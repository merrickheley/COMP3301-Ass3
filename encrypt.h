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
