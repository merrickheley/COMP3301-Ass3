/*
 *  encrypt.h
 *  Ext3301 File System
 *
 *  Merrick Heley (merrick.heley@uqconnect.edu.au)
 *
 *  Implements a naive encryption scheme for folders stored in the /encrypt
 *  directory of the ext3301 file system.
 */

#include "encrypt.h"

#include "ext2.h"

#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>

__u8 encrypt_key = 0;

/**
 * Encrypted write
 *
 * Calls do_sync_write, but if a key is set and the file is within
 * ENCRYPT_DIR the buffer is encrypted with encrypt_key before write.
 *
 * filp : File to be written to
 * buf  : Buffer to write to the file
 * len  : Length of write to the file
 * ppos : Position of write
 */
ssize_t do_sync_encrypt_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *ppos) {

    struct super_block *sb = filp->f_inode->i_sb;
    struct dentry *folder = filp->f_dentry;
    char *encryptedBuf = 0;
    size_t i = 0;
    size_t ret;
    mm_segment_t oldFs;

    /* Iterate up to folder below root */
    while (strncmp(folder->d_parent->d_name.name, "/", 2) != 0) {
        folder = folder->d_parent;
    }

    /* File is within the encrypt dir */
    if (strncmp(folder->d_name.name, ENCRYPT_DIR, strlen(ENCRYPT_DIR)+1) == 0) {

        /* Allocate memory for the buffer */
        if ((encryptedBuf = kmalloc(len, GFP_NOFS)) == 0) {
            ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
        }

        /* Get the buffer */
        copy_from_user(encryptedBuf, __user buf, len);

        ext2_msg(sb, KERN_DEBUG, "%s resides in /encrypt. Encrypting.", filp->f_dentry->d_name.name);

        /* Encrypt each char in the buffer */
        for (i = 0; i < len; i++) {
            encryptedBuf[i] ^= encrypt_key;
        }

        /* Move to kernel space to avoid write errors */
        /* encrpytedBuf will be outside threads segmentation limit */
        oldFs = get_fs();
        set_fs (KERNEL_DS);
        ret = do_sync_write(filp, encryptedBuf, len, ppos);
        set_fs(oldFs);

        kfree(encryptedBuf);

    } else {
        ret = do_sync_write(filp, buf, len, ppos);
    }

    return ret;
}

/**
 * Encrypted read
 *
 * Calls do_sync_read, but if a key is set and the file is within
 * ENCRYPT_DIR the buffer is decrypted with encrypt_key before read.
 *
 * filp : File to be written to
 * buf  : Buffer to write to the file
 * len  : Length of write to the file
 * ppos : Position of write
 */
ssize_t do_sync_encrypt_read(struct file *filp, char __user *buf,
        size_t len, loff_t *ppos) {

    struct super_block *sb = filp->f_inode->i_sb;
    struct dentry *folder = filp->f_dentry;
    char *decryptedBuf;
    size_t i = 0;
    size_t ret;
    mm_segment_t oldFs;

    /* Allocate memory for the buffer */
    if ((decryptedBuf = kmalloc(len, GFP_NOFS)) == 0) {
        ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
    }

    /* Move to kernel space to avoid read errors */
    /* decrpytedBuf will be outside threads segmentation limit */
    oldFs = get_fs();
    set_fs (KERNEL_DS);
    ret = do_sync_read(filp, decryptedBuf, len, ppos);
    set_fs(oldFs);

    /* Iterate up to folder below root */
    while (strncmp(folder->d_parent->d_name.name, "/", 2) != 0) {
        folder = folder->d_parent;
    }

    /* File is within the encrypt dir */
    if (strncmp(folder->d_name.name, ENCRYPT_DIR, strlen(ENCRYPT_DIR)+1) == 0) {

        ext2_msg(sb, KERN_DEBUG, "%s resides in /encrypt. Decrypting.", filp->f_dentry->d_name.name);

        /* Decrypt each char in the buffer */
        for (i = 0; i < len; i++) {
            decryptedBuf[i] ^= encrypt_key;
        }
    }

    /* Set the buffer */
    copy_to_user(__user buf, decryptedBuf, len);

    kfree(decryptedBuf);

    return ret;
}
