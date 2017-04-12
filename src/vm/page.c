#include "page.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "vm/frames.h"
#include "userprog/syscall.h"
#include "lib/string.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

//struct hash *spt;

//<Documentation>
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  page *p = hash_entry (p_, page, hash_element);
  return hash_int (p->upage);
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

void
page_init (struct hash *spt)
{
  hash_init (spt, page_hash, page_less, NULL);
}


page * 
page_add (page *entry) 
{
  //<ChiaHua>
  hash_insert (thread_current ()->spt, &entry->hash_element);
  return entry;
  //</ChiaHua>
}

void 
page_change_state (page *entry, int state)
{
  entry->location = state;
}

void
page_read_install (page *target)
{
//<Connie>
 /* 
 * during a page fault,
 * find the faulting upage
 * 
 * load it and install it like below
 */
  //Get a page of memory.  
  uint8_t *kpage = frame_find_empty ();
  // Load this page. 
  //page entry = Page_hash_find(upage virtual address);
  struct file *file = target->file;
  uint8_t *upage = target->upage;
  uint32_t page_read_bytes = target->page_read_bytes;
  uint32_t page_zero_bytes = target->page_zero_bytes;
  file_seek (file, target->ofs);
  if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
  {
    frame_free (kpage);
    //return false;
    system_exit_helper(-31);
  }
  memset (kpage + page_read_bytes, 0, page_zero_bytes);
  //</Connie>
  //<Chiahua>
  page_install_to_frame(target, upage, kpage);
  //</Chiahua>
}

void
page_install_to_frame (page *target, uint8_t *upage, uint8_t *kpage)
{
  // Add the page to the process's address space.
  //<Chiahua>
  bool writable = target->writable;
  if (!frame_install (upage, kpage, writable)) 
  {
    frame_free (kpage);
    system_exit_helper(-11);
  }
  
  target->location = IN_FRAME;
  //</Chiahua>
}

void page_fault_identifier (void *fault_addr) 
{
  //<Chiahua>
  //printf("\nFault Address: %x\n",fault_addr);
  page srch;
  page *target;
  struct hash_elem *target_elem;
  srch.upage = pg_round_down (fault_addr) ;
  target_elem = hash_find(thread_current ()->spt, &srch.hash_element);
  //Not part of our virtual address space. Segmentation Fault
  if (target_elem == NULL) 
  {
    printf("Segmentation Fault\n");
    system_exit_helper(-91);
  }
  else 
  {
  
    target = hash_entry (target_elem, page, hash_element);
    printf("Location = %d\n", target->location); 
    if (target->location == IN_SWAP)
    {
      //Grab from Swap
      //Install Page
    }
    else if (target->location == IN_FILESYS)
    {
       //Read from filesys
       //Install Page 
       page_read_install (target);
    }
    else 
    {
      //Should not get here
      ASSERT(false);
    }
  }

  //</Chiahua>
  /*
   * Notes: Be careful of bringing in the wrong data, like, treating code as data or data as code.
   * In thread create, as long as not the main thread, initialise supplemental page table entry?
   * 
   * Writable: Make it so you can't overwrite your code pages. 
   */
  
}
