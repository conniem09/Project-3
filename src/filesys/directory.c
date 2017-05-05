//Project 4
/* Student Information
 * Chia-Hua Lu              Cristian Martinez           Connie Chen
 * CL38755                  CJM4686                     CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com         conniem09@gmail.com
 * 52075                    52080                       52105
 */

#include "filesys/directory.h"

/* A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */    
    bool in_use;                        /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt, block_sector_t parent)
{
  //<Cris>
  bool success;
  success = inode_create (sector, entry_cnt * sizeof (struct dir_entry), true);
    
  struct inode *this_inode = inode_open (sector);
  struct dir *this_dir = dir_open (this_inode);
  dir_add (this_dir, ".", sector);    //add ourselves to the directory
  dir_add (this_dir, "..", parent);   //add our parent to the directory
  dir_close (this_dir);
  //</Cris>
  return success;
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
  {
    dir->inode = inode;
    dir->pos = 0;
    return dir;
  }
  else
  {
    inode_close (inode);
    free (dir);
    return NULL; 
  }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
  {
    inode_close (dir->inode);
    free (dir);
  }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
  {
    if (e.in_use && !strcmp (name, e.name)) 
    {
      if (ep != NULL)
        *ep = e;
      if (ofsp != NULL)
        *ofsp = ofs;
      return true;
    }
  }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;
  ASSERT (dir != NULL);
  ASSERT (name != NULL);
  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;
  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
  {
    return false;
  }
  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
  {
    goto done;
  }

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  //<Chiahua>
  if (inode->data.is_dir)
  {
    char buf[NAME_MAX + 1];
    struct dir *toBeRemoved = dir_open (inode);
    bool temp = dir_readdir (toBeRemoved, buf);
    if (temp)
    {
       return false;
    } 
    dir_close (toBeRemoved);
  }
  //</Chiahua>
  
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;
  
  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
  {
    dir->pos += sizeof e;
    //<Cris>
    if (e.in_use && (strcmp (e.name, ".") && strcmp (e.name, "..")))
    {
      strlcpy (name, e.name, NAME_MAX + 1);
      return true;
    } 
    //</Cris>
  }
  return false;
}

/*
Counts number of tokens in a path
*/
//<Chiahua>
int 
path_tokens (char* name) 
{
	char *path = (char*) malloc (strlen (name) + 1);
	strlcpy (path, name, strlen (name) + 1);
	char *token;
  char *save_ptr;
  int num_tokens = 0;
  
	for(token = strtok_r (path, "/", &save_ptr); token != NULL; 
      token = strtok_r (NULL, "/", &save_ptr))
  {
    num_tokens++;
  }
  free (path);
  return num_tokens;
}
//</Chiahua>

/*
 Navigates to location in file system specified by a given path. 
 If ingore_last is true, it will navigate to the 2nd to last item in path.
*/
//<Chiahua>
struct inode *
dir_traversal (char *name, bool ignore_last)
{
  struct dir *temp_dir = NULL;
  struct inode *inode = NULL;
  struct dir *dir = NULL;
  char *path = (char*) malloc (strlen (name) + 1);
  strlcpy (path, name, strlen (name) + 1);
  bool absolute = (*name == '/' || thread_current ()->pwd == NULL);
  char *token;
  char *save_ptr;
  int num_tokens = path_tokens (name) + !ignore_last;
  
	if (absolute)
  {
		//if the path is absolute, start from root
		dir = dir_open_root ();
	} else 
  {
		//if the path is relative, start from pwd
		dir = dir_reopen (thread_current ()->pwd);
	}
  inode = dir->inode;
  if(inode->removed)
  {
	  return NULL;
  }
  //make sure the dir isnt NULL
  if (dir != NULL)
  {
    //iterate through each subdirectory
    for (token = strtok_r (path, "/", &save_ptr); 
         token != NULL && num_tokens > 1; 
         token = strtok_r (NULL, "/", &save_ptr))
    {
      num_tokens--;
      //look to see if the directory exists
      dir_lookup (dir, token, &inode);
      //check if the next token is a directory or a file
      //<Cris>
      if (inode != NULL)
      {
        if (inode->data.is_dir)
        {
          //is a dir
          temp_dir = dir;
          dir = dir_open (inode);
          dir_close (temp_dir);
        }
        else
        {
          //is a file
          break;
        }
      }
      //</Cris>
    }
    inode = inode_reopen (inode);
    dir_close (dir);
  }
  return inode;
}
//</Chiahua>

/*
Takes a path name and returns the last item
*/
//<Chiahua>
char *
dir_token_last (char * path)
{
  char last[strlen (path) + 1];
	strlcpy (&last, path, strlen (path) + 1);
  char *token = NULL;
  char *save_ptr= NULL;
  char *prev = NULL;
  
  //<Cris>
  if (strlen (path) == 0)
  {
    //<Chiahua>
    char *result = (char*) malloc (1);
    *result = '\0';
    //</Chiahua>
    return result;
  }
  //</Cris>
  for (token = strtok_r (&last, "/", &save_ptr); token != NULL; 
       token = strtok_r (NULL, "/", &save_ptr))
  {
    prev = token;
  }
  char *result = (char*) malloc (strlen (prev) + 1);
	strlcpy (result, prev, strlen (prev) + 1);
  return result;
}
//</Chiahua>





