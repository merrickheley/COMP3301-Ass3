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

    ext2_msg(filp->f_inode->i_sb, KERN_INFO, "Immediate write");


    return do_sync_write(filp, buf, len, ppos);
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

    ext2_msg(filp->f_inode->i_sb, KERN_INFO, "Immediate read");


    return do_sync_read(filp, buf, len, ppos);
}
