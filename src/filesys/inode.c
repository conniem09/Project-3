#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

//ALERT!!!
// TODO: Convert Bitmap usage to free-map, since freemap is on disk and bitmap is in memory

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define SECTORSIZE 512		        //Bytes per sector
#define POINTERS_PER_SECTOR 128	//Pointers that fit on a sector
#define num_IBs 16               //Number of IBs pointed by iNode
#define secs_per_chunk 8         //Sectors per datablock

struct bitmap *bitmap;

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
  //<Chiahua>
  block_sector_t ib_ptr[num_IBs];         /* 16 Level 1 Indirect Pointer */
  unsigned magic;                    /* Magic number. */
  unsigned length;                   /* Length of File */
  unsigned unused[110];              /* Unused space occupier */
  //</Chiahua>
};

/* On-disk Indirect Block.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
   /*
struct ib_disk
{
  //<Chiahua>
  block_sector_t ptr[POINTERS_PER_SECTOR];            128 direct Pointers to data blocks 
  //</Chiahua>
};
*/

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

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

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos, bool extended) 
{
  ASSERT (inode != NULL);
  //if it is just reads, it will not extend
  //if (pos < inode->data.length)
  //{
  //<Chiahua>
  //temp buffer to store sector
  block_sector_t buffer[POINTERS_PER_SECTOR];
  block_sector_t empty_sector[POINTERS_PER_SECTOR];
  //
  int inode_index = pos/(SECTORSIZE*secs_per_chunk*POINTERS_PER_SECTOR);
  int IB_index = (pos%(SECTORSIZE*secs_per_chunk*POINTERS_PER_SECTOR))/(SECTORSIZE*secs_per_chunk);
  int sector_index = (pos%(SECTORSIZE*secs_per_chunk*POINTERS_PER_SECTOR))%(SECTORSIZE*secs_per_chunk)/
                     (SECTORSIZE);
                     
  // if file is larger than max supported size, fail the operation
  ASSERT (inode_index < num_IBs);
  
  //Read inode from disk into buffer
  block_read (fs_device, inode->sector, &buffer);
  //inode memory also contains its data.
  //can make the buffer into a inode disk
  //</Chiahua>
  
  //<Cris>
  //Get IB address from inode
  block_sector_t sector = buffer[inode_index];
  if (sector == 0)
  {
    if (!extended)
      return -1;
    else 
    {
      //<Chiahua>
      //Request new IB
      //sector = bitmap_scan (bitmap, 0, 1, false);
      free_map_allocate (1, &sector);
      buffer[inode_index] = sector;
      //bitmap_set_multiple (bitmap, sector, 1, true);
      block_write(fs_device, sector, &empty_sector); //Clear the IB on disk
      block_write(fs_device, inode->sector, &buffer);
      //</Chiahua>
    }
  }
  
  //Read IB from disk into buffer
  block_read (fs_device, sector, &buffer);
  //Save the IB address
  block_sector_t ib_address = sector;
  
  //Get Data block address from IB
  sector = buffer[IB_index];
  if (sector == 0)
  {
    if (!extended)
      return -1;
    else 
    {
      //<Connie>
      //Request new Data area
      //sector = bitmap_scan (bitmap, 0, secs_per_chunk, false);
      free_map_allocate (secs_per_chunk, &sector);
      buffer[IB_index] = sector;
      //bitmap_set_multiple (bitmap, sector, secs_per_chunk, true);
      int index;
      for (index = 0; index < secs_per_chunk; index++)
      {
        block_write(fs_device, sector+index, &empty_sector); //Clear the DB on disk
      }
      block_write(fs_device, ib_address, &buffer);
      //</Connie>
    }
  }
  
  //Recursively fill in gap if file has gap
  //<Chiahua>
  while (byte_to_sector (inode, pos - (secs_per_chunk * SECTORSIZE), false == -1)) 
  {
    byte_to_sector (inode, pos - (secs_per_chunk * SECTORSIZE), true);
    pos = pos - (secs_per_chunk * SECTORSIZE);
  } 
  //</Chiahua>
  //Return the correct datablock sector number
  return sector + sector_index; 
}
//</Cris> 
    

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
  bitmap = bitmap_create (block_size(fs_device)); 
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  //TODO: Edit this method extensively
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
  {
    disk_inode->magic = INODE_MAGIC;
    //Write inode disk to disk
    block_write (fs_device, sector, disk_inode);
    struct inode *inode = inode_open (sector);
    //Allocate datablocks for file of length length
    byte_to_sector (inode, length, true);  
    inode_close (inode);
    success = true;
    /*
    size_t sectors = bytes_to_sectors (length);
    disk_inode->length = length;
    disk_inode->magic = INODE_MAGIC;
    if (free_map_allocate (sectors, &disk_inode->start)) 
    {
      block_write (fs_device, sector, disk_inode);
      if (sectors > 0) 
      {
        static char zeros[BLOCK_SECTOR_SIZE];
        size_t i;
        
        for (i = 0; i < sectors; i++) 
          block_write (fs_device, disk_inode->start + i, zeros);
      }
      success = true; 
    } 
    */
    free (disk_inode);
  }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
  {
    inode = list_entry (e, struct inode, elem);
    if (inode->sector == sector) 
    {
      inode_reopen (inode);
      return inode; 
    }
}

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
  {
    /* Remove from inode list and release lock. */
    list_remove (&inode->elem);

    /* Deallocate blocks if removed. */
    //TODO: Edit how to deallocate 
    if (inode->removed) 
    {
      //<Connie>
      block_sector_t inode_buffer[POINTERS_PER_SECTOR];
      block_sector_t IB_buffer[POINTERS_PER_SECTOR];
      //Read inode from disk into buffer
      block_read (fs_device, inode->sector, &inode_buffer);
    
      int inode_index = 0;
      
      while (inode_buffer[inode_index] != 0 && inode_index < num_IBs)
      {
        //Read IB from inode into memory buffer
        block_read (fs_device, inode_buffer[inode_index], &IB_buffer);
        int ib_index = 0;
        while (IB_buffer[ib_index] != 0 && ib_index < POINTERS_PER_SECTOR)
        {
          //deallocating dataBlock
          free_map_release (IB_buffer[ib_index], secs_per_chunk);
          ib_index++;
        }
        //deallocating IB
        free_map_release (inode_buffer[inode_index], 1);
        inode_index++;
      }
      //deallocating inode
      free_map_release (inode->sector, 1);
      
      //</Connie>
      /*
      free_map_release (inode->sector, 1);
      free_map_release (inode->data.start,
                        bytes_to_sectors (inode->data.length)); 
                        */
    }

    free (inode); 
  }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
  {
    /* Disk sector to read, starting byte offset within sector. */
    block_sector_t sector_idx = byte_to_sector (inode, offset, false);
    int sector_ofs = offset % BLOCK_SECTOR_SIZE;

    /* Bytes left in inode, bytes left in sector, lesser of the two. */
    off_t inode_left = inode_length (inode) - offset;
    int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    int min_left = inode_left < sector_left ? inode_left : sector_left;

    /* Number of bytes to actually copy out of this sector. */
    int chunk_size = size < min_left ? size : min_left;
    if (chunk_size <= 0)
      break;

    if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
    {
      /* Read full sector directly into caller's buffer. */
      block_read (fs_device, sector_idx, buffer + bytes_read);
    }
    else 
    {
      /* Read sector into bounce buffer, then partially copy
         into caller's buffer. */
      if (bounce == NULL) 
      {
        bounce = malloc (BLOCK_SECTOR_SIZE);
        if (bounce == NULL)
          break;
      }
      block_read (fs_device, sector_idx, bounce);
      memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
    }
    
    /* Advance. */
    size -= chunk_size;
    offset += chunk_size;
    bytes_read += chunk_size;
  }
  free (bounce);

  return bytes_read;
}             


/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
  {
    //TODO: Need to allocate space and create IBs and datablocks
    
    /* Sector to write, starting byte offset within sector. */
    block_sector_t sector_idx = byte_to_sector (inode, offset, true);
    int sector_ofs = offset % BLOCK_SECTOR_SIZE;

    /* Bytes left in inode, bytes left in sector, lesser of the two. */
    off_t inode_left = inode_length (inode) - offset;
    int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    int min_left = inode_left < sector_left ? inode_left : sector_left;

    /* Number of bytes to actually write into this sector. */
    int chunk_size = size < min_left ? size : min_left;
    if (chunk_size <= 0)
      break;

    if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
    {
      /* Write full sector directly to disk. */
      block_write (fs_device, sector_idx, buffer + bytes_written);
    }
    else 
    {
      /* We need a bounce buffer. */
      if (bounce == NULL) 
      {
        bounce = malloc (BLOCK_SECTOR_SIZE);
        if (bounce == NULL)
          break;
      }

      /* If the sector contains data before or after the chunk
         we're writing, then we need to read in the sector
         first.  Otherwise we start with a sector of all zeros. */
      if (sector_ofs > 0 || chunk_size < sector_left) 
        block_read (fs_device, sector_idx, bounce);
      else
        memset (bounce, 0, BLOCK_SECTOR_SIZE);
      memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
      block_write (fs_device, sector_idx, bounce);
    }

    /* Advance. */
    size -= chunk_size;
    offset += chunk_size;
    bytes_written += chunk_size;
  }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
