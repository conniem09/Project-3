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
  
  struct inode *parent_inode = dir_traversal(name, true); 
  if(parent_inode->removed)
  {
    return false;
  }
  char *item_name = dir_token_last (name);
  struct dir *dir = dir_open(parent_inode);  //Change this
  bool a = dir != NULL ;
  bool b = free_map_allocate (1, &inode_sector);
  bool c = inode_create (inode_sector, initial_size, false);
  bool d = dir_add (dir, item_name, inode_sector);
  bool success = (a && b && c && d);
  /*
  bool success = (dir != NULL 
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false)
                  && dir_add (dir, item_name, inode_sector));
                  * */
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
  struct inode *inode = NULL;
  inode = dir_traversal(name, false);

  return file_open(inode);
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
/*bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}*/

bool
filesys_remove (const char *name) 
{
  if(!strcmp(name, "/")){
    return false;
  }
  struct inode *parent_inode = dir_traversal(name, true);
  struct dir *dir = dir_open (parent_inode);
  char* child_name = dir_token_last(name);
  bool success = dir != NULL && dir_remove (dir, child_name);
  dir_close (dir);
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





  /*
  char *path = (char *) malloc(strlen(name)+1);
  strlcpy(path, name, strlen(name)+1);
  int index;
  bool valid_token = false;
  bool replaced_slash = false;
  for (index = strlen(path)-1; index >=0 && !valid_token && !replaced_slash; index--)
  {

    if (*(path+index) != '/')
      valid_token = true;
    if (*(path+index) == '/')
    {
      replaced_slash = true;
      *(path+index) = '\0';
    }
  }

  printf("%s\n", path);
  //delete last token (name); 
  struct dir *dir = dir_open (fs_traversal(name));*/
  //struct dir *dir = navigate;
