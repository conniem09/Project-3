//Project 4
/* Student Information
 * Chia-Hua Lu              Cristian Martinez           Connie Chen
 * CL38755                  CJM4686                     CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com         conniem09@gmail.com
 * 52075                    52080                       52105
 */


#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */

   
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  
  //<Cris>
  struct inode *parent_inode = dir_traversal (name, true); 
  char *item_name = dir_token_last (name);
  struct dir *dir = dir_open (parent_inode);  
  //</Cris>
  bool success = (dir != NULL 
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false)
                  && dir_add (dir, item_name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{  
  //<Connie>
  struct inode *inode = NULL;
  inode = dir_traversal (name, false);
  return file_open (inode);
  //</Connie>
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  //<Cris>
  if(!strcmp(name, "/"))
  {
    return false;
  }
  struct inode *parent_inode = dir_traversal (name, true);
  struct dir *dir = dir_open (parent_inode);
  char* child_name = dir_token_last (name);
  bool success = dir != NULL && dir_remove (dir, child_name);
  dir_close (dir);
  //</Cris>
  return success;
}


/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16, ROOT_DIR_SECTOR))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/*Scans a given string to see if it contains the character '\', 
which would indicate that it is a path rather than a 
file or directory name. */
//<Chiahua>
bool 
is_path (const char *file_name)
{
  while (*file_name != '\0')
  {
    if (*file_name == '/')
      return true;
    file_name++;
  }
  return false;
} 
//</Chiahua>
