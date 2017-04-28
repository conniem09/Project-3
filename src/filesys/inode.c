#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define SECTORSIZE 512		       //Bytes per sector
#define POINTERS_PER_SECTOR 128	 //Pointers that fit on a sector
#define NUM_IBs 20               //Number of IBs pointed by iNode
#define SECS_PER_CHUNK 8         //Sectors per datablock

block_sector_t empty_sector[POINTERS_PER_SECTOR];

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
  //<Chiahua>
  block_sector_t ib_ptr[NUM_IBs];         /* 32 Level 1 Indirect Pointer */
  unsigned length;                   /* Length of File */
  unsigned magic;                    /* Magic number. */
  unsigned unused[POINTERS_PER_SECTOR-NUM_IBs-2];   /* Unused space occupier */
  //</Chiahua>
};

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
byte_to_sector (struct inode *inode, off_t pos, bool extended, bool fill_hole) 
{
  int tempOffset = pos;
  ASSERT (inode != NULL);

  //<Chiahua>
  //temp buffer to store sector
  block_sector_t buffer[POINTERS_PER_SECTOR];
  
  //Calculate coordinates to datablock
  int inode_index = pos/(SECTORSIZE*SECS_PER_CHUNK*POINTERS_PER_SECTOR);
  int IB_index = (pos%(SECTORSIZE*SECS_PER_CHUNK*POINTERS_PER_SECTOR))/
                 (SECTORSIZE*SECS_PER_CHUNK);
  int sector_index = (pos%(SECTORSIZE*SECS_PER_CHUNK*POINTERS_PER_SECTOR))%
                     (SECTORSIZE*SECS_PER_CHUNK)/(SECTORSIZE);
                   
  // if file is larger than max supported size, fail the operation
  ASSERT (inode_index < NUM_IBs);
  //Read inode from disk into buffer
  block_read (fs_device, inode->sector, &buffer);
  //</Chiahua>
  
  //<Cris>
  //Get IB address from inode
  block_sector_t temp_sector = buffer[inode_index];
  //If Indirect Block does not exist
  if (temp_sector == 0)
  {
    //return -1 if we dont care about extending
    if (!extended)
    {
      return -1;
    }
    else //extend the file by allocating a new indirect block
    {
      //<Chiahua>
      //Request new IB
      free_map_allocate (1, &temp_sector);
      buffer[inode_index] = temp_sector;

      //inode has been changed, save it to disk.
      block_write(fs_device, inode->sector, &buffer);
      //Clear the garbage in IB on the disk 
      block_write(fs_device, temp_sector, &empty_sector); 
      //</Chiahua>
    }
  }

  //Read IB from disk into buffer
  block_read (fs_device, temp_sector, &buffer);
  //Save the IB address
  block_sector_t ib_address = temp_sector;
  //Get Data block address from IB
  temp_sector = buffer[IB_index];
  //if the data block doesnt exist
  if (temp_sector == 0)
  {
    if (!extended)
      return -1;
    else 
    {
      //<Connie>
      //Allocate 8 sectors to use as the data block
      free_map_allocate (SECS_PER_CHUNK, &temp_sector);
  
      buffer[IB_index] = temp_sector;
      //Indirect block has been changed, save it to disk
      block_write(fs_device, ib_address, &buffer);
      
      //update all 8 sectors in the datablock to 0
      int index;
      for (index = 0; index < SECS_PER_CHUNK; index++)
      {
        block_write(fs_device, temp_sector + index, &empty_sector); 
      }
      //</Connie>
    }
  }
  
  //<Chiahua>
  //fill in data that hasn't been allocated yet.
  if (extended && fill_hole) 
  {
    //iterate backwards through unallocated data.
    for (pos -= (SECTORSIZE*SECS_PER_CHUNK); pos >= 0;
         pos -= (SECTORSIZE*SECS_PER_CHUNK))
    {
      //if already allocated, no need to continue with the loop
      if (byte_to_sector (inode, pos, false, false) != -1)
      {
        break;
      }
      else
      {
        //if unallocated, then allocate
        byte_to_sector (inode, pos, true, false);
      }
    }
  }
  //</Chiahua>
  //temp_sector is the index of the first sector in the data block
  //adding sector index gives us which sector the byte is at.
  return temp_sector + sector_index; 
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
  int index;
  for (index = 0; index < 128; index++)
  {
    empty_sector[index] = 0;
  }
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
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
    //Before calling block write, need to update length
    disk_inode->length = length;
    block_write (fs_device, sector, disk_inode);
    struct inode *inode = inode_open (sector);
    //Allocate datablocks for file of length length
    byte_to_sector (inode, length, true, true);  
    inode_close (inode);
    success = true;
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
    if (inode->removed) 
    {
      //<Connie>
      block_sector_t inode_buffer[POINTERS_PER_SECTOR];
      block_sector_t IB_buffer[POINTERS_PER_SECTOR];
      //Read inode from disk into buffer
      block_read (fs_device, inode->sector, &inode_buffer);
    
      int inode_index = 0;
      
      while (inode_buffer[inode_index] != 0 && inode_index < NUM_IBs)
      {
        //Read IB from inode into memory buffer
        block_read (fs_device, inode_buffer[inode_index], &IB_buffer);
        int ib_index = 0;
        while (IB_buffer[ib_index] != 0 && ib_index < POINTERS_PER_SECTOR)
        {
          //deallocating dataBlock
          free_map_release (IB_buffer[ib_index], SECS_PER_CHUNK);
          IB_buffer[ib_index] = 0;
          ib_index++;
        }
        block_write (fs_device, inode_buffer[inode_index], &IB_buffer);
        //deallocating IB
        free_map_release (inode_buffer[inode_index], 1);
        inode_buffer[inode_index] = 0;
        inode_index++;
      }
      block_write (fs_device, inode->sector, &inode_buffer);
      //deallocating inode
      free_map_release (inode->sector, 1);
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
  if (size+offset > inode->data.length)
    return 0;
  while (size > 0) 
  {
    /* Disk sector to read, starting byte offset within sector. */
    block_sector_t sector_idx = byte_to_sector (inode, offset, false, false);
    if (sector_idx == -1)
    {
      return 0; 
    }
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
  {
    return 0;
  }
  byte_to_sector (inode, offset+size, true, true);
  if (inode->data.length < size+offset)
  {
    inode->data.length = size+offset;
	block_write(fs_device, inode->sector, &inode->data);
  }
  while (size > 0) 
  {        
    /* Sector to write, starting byte offset within sector. */
    block_sector_t sector_idx = byte_to_sector (inode, offset, false, false); 
    int sector_ofs = offset % BLOCK_SECTOR_SIZE;

    /* Bytes left in inode, bytes left in sector, lesser of the two. */
    off_t inode_left = inode_length (inode) - offset;
    int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    int min_left = inode_left < sector_left ? inode_left : sector_left;
    /* Number of bytes to actually write into this sector. */
    int chunk_size = size < min_left ? size : min_left;
    if (chunk_size <= 0)
    {
      break;
    }
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

//Length Is critical
/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
