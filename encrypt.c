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

#include <linux/fs.h>

/**
 * Encrypted write
 *
 *
 */
ssize_t do_sync_encrypt_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *ppos) {

	return do_sync_write(filp, buf, len, ppos);
}
