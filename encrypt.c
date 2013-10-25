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
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pagemap.h>

__u8 encrypt_key = 0;

/**
 * is_parent_encrypt
 *
 * Returns '1' if ENCRYPT_DIR is an ancestor of folder, '0' otherwise
 */
int is_encrypt_ancestor(struct dentry *dir) {

    /* Iterate up to folder below root */
//    while (strncmp(dir->d_parent->d_name.name, "/", 2) != 0) {
    while (!IS_ROOT(dir->d_parent)) {
        dir = dir->d_parent;
    }

    return !strncmp(dir->d_name.name, ENCRYPT_DIR, strlen(ENCRYPT_DIR)+1);
}

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
    char *encryptedBuf = 0;
    size_t i = 0;
    size_t ret;
    mm_segment_t oldFs;

    /* File is within the encrypt dir */
    if (is_encrypt_ancestor(filp->f_dentry)) {

        ext2_debug("%s resides in /encrypt. Encrypting.", filp->f_dentry->d_name.name);

        /* Allocate memory for the buffer */
        if ((encryptedBuf = kmalloc(len, GFP_NOFS)) == 0) {
            ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
        }

        /* Get the buffer */
        copy_from_user(encryptedBuf, __user buf, len);

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
    char *decryptedBuf;
    size_t i = 0;
    size_t ret;
    mm_segment_t oldFs;

    /* File is within the encrypt dir */
    if (is_encrypt_ancestor(filp->f_dentry)) {

        ext2_debug("%s resides in /encrypt. Decrypting.", filp->f_dentry->d_name.name);

        /* Allocate memory for the buffer */
        if ((decryptedBuf = kmalloc(len, GFP_NOFS)) == 0) {
            ext2_error(sb, KERN_CRIT, "Failed to allocate memory");
        }

        /* Move to kernel space to avoid read errors */
        /* encrpytedBuf will be outside threads segmentation limit */
        oldFs = get_fs();
        set_fs (KERNEL_DS);
        ret = do_sync_read(filp, decryptedBuf, len, ppos);
        set_fs(oldFs);

        /* Decrypt each char in the buffer */
        for (i = 0; i < len; i++) {
            *(decryptedBuf + i) ^= encrypt_key;
        }

        /* Set the buffer */
        copy_to_user(__user buf, decryptedBuf, len);

        kfree(decryptedBuf);
    } else {
        ret = do_sync_read(filp, buf, len, ppos);
    }

    return ret;
}

/**
 * Crypt block
 *
 * Toggle encryption for the data stored in a file. This is used if the file
 * is moved to/from ENCRYPT_DIR.
 *
 * inode : Directory of file to be encrypted/decrypted
 */
void crypt_block(struct inode *inode) {

    struct page *page;
    struct address_space *mapping = inode->i_mapping;
    char *pageAddr;
    pgoff_t i, j;

    /* For each page */
    for (i = 0; i < mapping->nrpages; i++) {

        /* Find the page, lock it, and get the page address */
        page = find_lock_page(mapping, i);
        pageAddr = (char *) page_address(page);

        /* Crypt each byte in the page, using j as an offset */
        for(j = 0; j < PAGE_CACHE_SIZE; j++) {
            *(pageAddr + j) ^= encrypt_key;
        }

        unlock_page(page);
        kunmap(page);
        page_cache_release(page);
    }

}
