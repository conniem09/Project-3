//Project 4
/* Student Information
 * Chia-Hua Lu              Cristian Martinez           Connie Chen
 * CL38755                  CJM4686                     CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com         conniem09@gmail.com
 * 52075                    52080                       52105
 */

#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);
bool is_path (const char *file_name);

#endif /* filesys/filesys.h */
