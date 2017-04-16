/* Student Information
 * Chia-Hua Lu              Cristian Martinez     Connie Chen
 * CL38755                  CJM4686               CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com   conniem09@gmail.com
 * 52075                    52080                 52105
 */
#include "page.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "vm/frames.h"
#include "userprog/syscall.h"
#include "lib/string.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

//<Documentation>
/* Returns a hash value for page p.*/
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  page *p = hash_entry (p_, page, hash_element);
  return hash_int ((int)p->upage);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const page *a = hash_entry (a_, page, hash_element);
  const page *b = hash_entry (b_, page, hash_element);

  return a->upage < b->upage;
}
//</Documentation>

//<Chiahua>
//initiate the supplemental page table. 
void
page_init (struct hash *spt)
{
  //(the SPT is actually just a hashtable, thats it)
  hash_init (spt, page_hash, page_less, NULL);
}

//add a page to the supplemental page table. 
page * 
page_add (page *entry) 
{
  hash_insert (thread_current ()->spt, &entry->hash_element);
  return entry;
}

void 
page_clear_all ()
{
  struct hash_iterator i;
  struct hash *spt = thread_current ()->spt;
  page *p;
  hash_first (&i, spt);
  while (hash_next (&i))
  {
    p = hash_entry (hash_cur (&i), page, hash_element);
    if (p->location == IN_FRAME)
    {
      frame_free (p->kpage);
      p->kpage = NULL;
      p->location = 0;
    }
  }
  hash_destroy(spt, NULL);
}
//</Chiahua>

//<Cris>
page * 
page_build(uint8_t *upage, struct file *file, bool writable, 
           size_t page_read_bytes, size_t page_zero_bytes, int location, 
           unsigned ofs, uint32_t *pagedir)
{
      page *entry = (page*) malloc (sizeof (page));
      ASSERT(entry != NULL);
      entry->upage = upage;
      entry->file = file;
      entry->page_read_bytes = page_read_bytes;
      entry->page_zero_bytes = page_zero_bytes;
      entry->writable = writable;
      entry->ofs = ofs;
      entry->location = location;
      entry->kpage = NULL;
      entry->pagedir = pagedir;
      entry->swap_location = 0;
      return entry; 
}
//</Cris>

void
page_read_install (page *target)
{
  //<Connie>
  
  //Find an empty frame to intall to
  uint8_t *kpage = frame_find_empty ();
  
  //page entry = Page_hash_find(upage virtual address);
  struct file *file = target->file;
  uint8_t *upage = target->upage;
  uint32_t *pagedir = target->pagedir;
  uint32_t page_read_bytes = target->page_read_bytes;
  uint32_t page_zero_bytes = target->page_zero_bytes;
  file_seek (file, target->ofs);
  if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
  {
    frame_free (kpage);
    system_exit_helper(-1);
  }
  memset (kpage + page_read_bytes, 0, page_zero_bytes);
  //</Connie>
  //<Chiahua>
  page_install_to_frame(target, upage, kpage, pagedir);
  //</Chiahua>
}

bool
page_install_to_frame (page *target, uint8_t *upage, uint8_t *kpage, 
                      uint32_t *pagedir)
{
  // Add the page to physical address space.
  //<Chiahua>
  bool writable = target->writable;
  bool success = frame_install (upage, kpage, pagedir, writable, target);
  if (!success) 
  {
    frame_free (kpage);
    system_exit_helper(-1);
  }
  
  target->location = IN_FRAME;
  target->kpage = kpage;
  return success;
  //</Chiahua>
}

void page_fault_identifier (void *fault_addr) 
{
  //<Chiahua>
  page srch;
  page *target;
  struct hash_elem *target_elem;
  srch.upage = pg_round_down (fault_addr);
  target_elem = hash_find(thread_current ()->spt, &srch.hash_element);
  //Not part of our virtual address space.
  if (target_elem == NULL) 
  {
    system_exit_helper(-1);
  }
  else 
  {
    target = hash_entry (target_elem, page, hash_element);
    if (target->location == IN_SWAP)
    {
      //Grab from Swap
      page_install_to_frame (target, target->upage, target->kpage, 
                      target->pagedir);
      swap_read (target->kpage, target);
    }
    else if (target->location == IN_FILESYS)
    {
       //Read from filesys
       page_read_install (target);
    }
    else 
    {
      //Should not get here
      ASSERT(false);
    }
  }  
}
//</Chiahua>
