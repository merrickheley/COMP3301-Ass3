/*
 *  immediate.c
 *  Ext3301 File System
 *
 *  Merrick Heley (merrick.heley@uqconnect.edu.au)
 *
 *  Implements immediate files. This allows the file system to use the inode
 *  structure to store files less than or equal to 60 bytes.
 */

#include "immediate.h"

#include <asm/uaccess.h>
#include <linux/slab.h>

#include "encrypt.h"
#include "ext2.h"
#include "xip.h"

/**
 * Grow immediate
 *
 * Grow an immediate file to a regular file if the immediate file no longer
 * has enough storage space for the data.
 *
 * filp : File to be written to
 * buf  : Buffer to write to the file
 * len  : Length of write to the file
 * ppos : Position of write
 */
ssize_t grow_immediate(struct file *filp, const char __user *buf,
        size_t len, loff_t *ppos) {

    struct super_block *sb = filp->f_dentry->d_inode->i_sb;
    struct inode *inode = filp->f_dentry->d_inode;
    char *moveBuf;
    loff_t moveSize = i_size_read(inode);
    int errp = 0;
    mm_segment_t oldFs;

    /* Allocate memory for the buffer */
    if ((moveBuf = kmalloc(moveSize, GFP_NOFS)) == 0) {
        ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
    }

    // Tried copy to user here
    memcpy(moveBuf, EXT2_I(inode)->i_data, moveSize);

    /* Reset the inode data to 0 */
    memset(EXT2_I(inode)->i_data, 0, moveSize);

    /* Set the mode */
    inode->i_mode ^= S_IFIM;
    inode->i_mode |= S_IFREG;

    /* Set the inode operations, taken straight from original ext2_create */
    if (ext2_use_xip(ino.de->i_sb)) {
      inode->i_mapping->a_ops = &ext2_aops_xip;
      inode->i_fop = &ext2_xip_file_operations;
    } else if (test_opt(inode->i_sb, NOBH)) {
      inode->i_mapping->a_ops = &ext2_nobh_aops;
      inode->i_fop = &ext2_file_operations;
    } else {
      inode->i_mapping->a_ops = &ext2_aops;
      inode->i_fop = &ext2_file_operations;
    }

    /* Mark the inode as dirty */
    mark_inode_dirty(inode);

    /* Reset the file operations */
    filp->f_op = inode->i_fop;

    /* Initialise a new block */
    EXT2_I(inode)->i_data[0] = ext2_new_block(inode, 0, &errp);
    inode->i_blocks = 1;

    /* Write the old data without any encryption
     * Ensure the old data is written at the start of the file */
    if(filp->f_flags & O_APPEND) {
        filp->f_flags ^= O_APPEND;
        filp->f_flags |= O_TRUNC;

        oldFs = get_fs();
        set_fs(KERNEL_DS);
        do_sync_write(filp, moveBuf, moveSize, ppos);
        set_fs(oldFs);

        filp->f_flags ^= O_TRUNC;
        filp->f_flags |= O_APPEND;
    }

    kfree(moveBuf);

    /* Write the old data at the start of the file */
    return do_sync_encrypt_write(filp, buf, len, ppos);
}

/**
 * Immediate write
 *
 * Write data directly into inode if the total data stored in the inode will
 * be less than or equal to 60 bytes.
 *
 * filp : File to be written to
 * buf  : Buffer to write to the file
 * len  : Length of write to the file
 * ppos : Position of write
 */
ssize_t do_sync_immediate_write(struct file *filp, const char __user *buf,
                size_t len, loff_t *ppos) {

    struct inode *inode = filp->f_dentry->d_inode;
    char *sink = (char *) EXT2_I(inode)->i_data;
    loff_t writePos;
    char i = 0;

    /* Grow the file if the new data is too large */
    if (((filp->f_flags & O_APPEND) && (i_size_read(inode) + len > IM_SIZE))
            || (len > IM_SIZE)) {

        return grow_immediate(filp, buf, len, ppos);
    }

    /* Find the write position */
    if (filp->f_flags & O_APPEND) {
        writePos = i_size_read(inode);
    } else {
        writePos = *ppos;
    }

    /* Copy the data into the inode sink */
    copy_from_user(sink + writePos, __user buf, len);

    /* Encrypt each char in the buffer if in ENCRYPT_DIR */
    if (is_encrypt_ancestor(filp->f_dentry)) {
        for (i = 0; i < len; i++) {
            *(sink + writePos + i) ^= encrypt_key;
        }
    }

    /* Increment file pointer */
    *ppos = writePos;
    *ppos += len;

    /* Track the size of the inode */
    i_size_write(inode, *ppos);

    /* Mark the inode as dirty */
    mark_inode_dirty(inode);

    return len;
}

/**
 * Immediate read
 *
 * Read data directly from the inode.
 *
 * filp : File to be written to
 * buf  : Buffer to write to the file
 * len  : Length of write to the file
 * ppos : Position of write
 */
ssize_t do_sync_immediate_read(struct file *filp, char __user *buf,
                size_t len, loff_t *ppos) {

    struct super_block *sb = filp->f_dentry->d_inode->i_sb;
    struct inode *inode = filp->f_dentry->d_inode;
    char *sink = (char *) EXT2_I(inode)->i_data;
    char *sinkBuf;
    size_t rlen = 0;
    loff_t size = i_size_read(inode);
    char i = 0;

    /* Invalid read */
    if (*ppos >= size) {
        return 0;
    }

    /* Ensure we don't try and read outside the inode */
    if (len < size - *ppos) {
        rlen = len;
    } else {
        rlen = size - *ppos;
    }

    /* Allocate memory for the buffer */
    if ((sinkBuf = kmalloc(rlen + 1, GFP_NOFS)) == 0) {
        ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
    }

    /* Copy the data out and append a terminator */
    memcpy(sinkBuf, sink + *ppos, rlen);
    sinkBuf[rlen] = 0;

    /* Decrypt each char in the buffer if in ENCRYPT_DIR */
    if (is_encrypt_ancestor(filp->f_dentry)) {
        for (i = 0; i < rlen; i++) {
            *(sinkBuf + i) ^= encrypt_key;
        }
    }

    copy_to_user(__user buf, sinkBuf, rlen + 1);

    /* Increment file pointer */
    *ppos += rlen;

    kfree(sinkBuf);

    return rlen;
}

/**
 * Generic write
 *
 * Check if an immediate file is getting a sync_write command. If so, fix the
 * file operations and perform an immediate write. Otherwise, perform an
 * encrypt write.
 *
 * filp : File to be written to
 * buf  : Buffer to write to the file
 * len  : Length of write to the file
 * ppos : Position of write
 */
ssize_t do_sync_generic_write(struct file *filp, const char __user *buf,
                size_t len, loff_t *ppos) {

    /* If an immediate file finds its way here, change the file operations
     * and do an immediate write */
    if (S_ISIM(filp->f_dentry->d_inode->i_mode)) {

        filp->f_op = filp->f_dentry->d_inode->i_fop;
        return do_sync_immediate_write(filp, buf, len, ppos);
    } else {
        return do_sync_encrypt_write(filp, buf, len, ppos);
    }

}

/**
 * Crypt immediate
 *
 * Toggle encryption for the data stored in on the inode. This is used if the
 * file is moved to/from ENCRYPT_DIR.
 *
 * inode : Directory of file to be encrypted/decrypted
 */
void crypt_immediate(struct inode *inode) {
    char *sink = (char *) EXT2_I(inode)->i_data;
    loff_t size = i_size_read(inode);
    char i = 0;

    for (i = 0; i < size; i++) {
        *(sink + i) ^= encrypt_key;
    }

    mark_inode_dirty(inode);
}
