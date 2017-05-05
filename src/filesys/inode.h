//Project 4
/* Student Information
 * Chia-Hua Lu              Cristian Martinez           Connie Chen
 * CL38755                  CJM4686                     CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com         conniem09@gmail.com
 * 52075                    52080                       52105
 */
 
 
#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <list.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define SECTORSIZE 512		       //Bytes per sector
#define POINTERS_PER_SECTOR 128	 //Pointers that fit on a sector
#define NUM_IBs 20               //Number of IBs pointed by iNode
#define SECS_PER_CHUNK 8         //Sectors per datablock

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
  //<Chiahua>
  block_sector_t ib_ptr[NUM_IBs];    /* 32 Level 1 Indirect Pointer */
  bool is_dir;                       /* Is this inode a directory or file */
  unsigned length;                   /* Length of File */
  unsigned magic;                    /* Magic number. */
  unsigned unused[POINTERS_PER_SECTOR-NUM_IBs-3];   /* Unused space occupier */
  //</Chiahua>
};

/* In-memory inode. */
struct inode 
{
  struct list_elem elem;              /* Element in inode list. */
  block_sector_t sector;              /* Sector number of disk location. */
  int open_cnt;                       /* Number of openers. */
  bool removed;                       /* True if deleted, false otherwise. */
  int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
  struct inode_disk data;             /* Inode content. */
};

void inode_init (void);
bool inode_create (block_sector_t, off_t, bool);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

#endif /* filesys/inode.h */
