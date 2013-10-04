/*
 *  encrypt.h
 *  Ext3301 File System
 *
 *  Merrick Heley (merrick.heley@uqconnect.edu.au)
 *
 *  Implements a naive encryption scheme for folders stored in the /encrypt
 *  directory of the ext3301 file system.
 */

#include <linux/fs.h>

/* Folder that will be encrypted */
#define ENCRYPT_DIR "encrypt"

/* Encryption key (8 bits only) */
extern __u8 encrypt_key;


/* Function for encrypted write */
ssize_t do_sync_encrypt_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *ppos);
