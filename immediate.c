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

#include <linux/slab.h>

#include "ext2.h"

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

    struct super_block *sb = filp->f_dentry->d_inode->i_sb;
    struct inode *inode = filp->f_dentry->d_inode;
    char *sink = (char *) EXT2_I(inode)->i_data;
    loff_t writePos;

    /* Find the write position */
    if (filp->f_flags & O_APPEND) {
        writePos = inode->i_size;
    } else {
        writePos = *ppos;
    }

    /* Copy the data into the inode sink */
    copy_from_user(sink + writePos, __user buf, len);

    /* Track the size of the inode */
    /* Look at using i_size_read and i_size_write?? */
    inode->i_size += len;

    /* Increment file pointer */
    *ppos += len;

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
ssize_t do_sync_immediate_read(struct file *filp, const char __user *buf,
                size_t len, loff_t *ppos) {

    struct super_block *sb = filp->f_dentry->d_inode->i_sb;
    struct inode *inode = filp->f_dentry->d_inode;
    char *sink = (char *) EXT2_I(inode)->i_data;
    char *sinkBuf;
    size_t rlen = 0;

    /* Invalid read */
    if (*ppos >= inode->i_size)
        return 0;

    /* Ensure we don't try and read outside the inode */
    if (len < inode->i_size - *ppos) {
        rlen = len;
    } else {
        rlen = inode->i_size - *ppos;
    }

    /* Allocate memory for the buffer */
    if ((sinkBuf = kmalloc(rlen + 1, GFP_NOFS)) == 0) {
        ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
    }

    /* Copy the data out and append a terminator */
    memcpy(sinkBuf, sink + *ppos, rlen);
    sinkBuf[rlen] = 0;

    copy_to_user((void *) buf, sinkBuf, rlen + 1);

    /* Increment file pointer */
    *ppos += rlen;

    kfree(sinkBuf);

    return rlen;
}
