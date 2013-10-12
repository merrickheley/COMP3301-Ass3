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

/* Function for checking if dentry is a child of ENCRYPT_DIR */
int is_encrypt_ancestor(struct dentry *folder);

/* Function for encrypted write */
ssize_t do_sync_encrypt_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *ppos);

/* Function for encrypted read */
ssize_t do_sync_encrypt_read(struct file *filp, char __user *buf,
        size_t len, loff_t *ppos);

/* En/De-crypt the data stored in a file */
void crypt_block(struct dentry *dir);
