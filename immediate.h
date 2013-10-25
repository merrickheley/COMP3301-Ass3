/*
 *  immediate.h
 *  Ext3301 File System
 *
 *  Merrick Heley (merrick.heley@uqconnect.edu.au)
 *
 *  Implements immediate files. This allows the file system to use the inode
 *  structure to store files less than or equal to 60 bytes.
 *
 *  Created on 2013-10-23
 */

#include <linux/fs.h>

/* Immediate file number */
#define DT_IM 9

#define S_IFIM   0110000
#define S_ISIM(m)      (((m) & S_IFMT) == S_IFIM)

/* Maximum size of immediate files */
#define IM_SIZE 60

/* Function for immediate write */
ssize_t do_sync_immediate_write(struct file *filp, const char __user *buf,
                size_t len, loff_t *ppos);

/* Function for immediate read */
ssize_t do_sync_immediate_read(struct file *filp, char __user *buf,
                size_t len, loff_t *ppos);

/* Function for wrapping generic write */
ssize_t do_sync_generic_write(struct file *filp, const char __user *buf,
                size_t len, loff_t *ppos);

/* Function for immediate crypt on inode */
void crypt_immediate(struct inode *inode);
