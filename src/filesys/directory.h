//Project 4
/* Student Information
 * Chia-Hua Lu              Cristian Martinez           Connie Chen
 * CL38755                  CJM4686                     CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com         conniem09@gmail.com
 * 52075                    52080                       52105
 */

#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include <stddef.h>
#include "devices/block.h"
#include "threads/thread.h"

/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 15

/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

struct inode;

/* Opening and closing directories. */
bool dir_create (block_sector_t sector, size_t entry_cnt, block_sector_t parent);
struct dir *dir_open (struct inode *);
struct dir *dir_open_root (void);
struct dir *dir_reopen (struct dir *);
void dir_close (struct dir *);
struct inode *dir_get_inode (struct dir *);

/* Reading and writing. */
bool dir_lookup (const struct dir *, const char *name, struct inode **);
bool dir_add (struct dir *, const char *name, block_sector_t);
bool dir_remove (struct dir *, const char *name);
bool dir_readdir (struct dir *, char name[NAME_MAX + 1]);

struct inode *dir_traversal(char *name, bool ignore_last);
char *dir_token_last (char * path);
int path_tokens (char* name);
#endif /* filesys/directory.h */
