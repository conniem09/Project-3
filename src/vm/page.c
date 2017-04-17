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
#include "vm/swap.h"

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

//
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
  free(spt);
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
      entry->swap_location = -1;
      return entry; 
}
//</Cris>

void
page_read_install (page *target)
{
  //<Connie>
  //Find an empty frame to intall to
  uint8_t *kpage = frame_find_empty ();
  
  struct file *file = target->file;
  uint8_t *upage = target->upage;
  uint32_t *pagedir = target->pagedir;
  uint32_t page_read_bytes = target->page_read_bytes;
  uint32_t page_zero_bytes = target->page_zero_bytes;
  file_seek (file, target->ofs);
  if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
  {
    frame_free (kpage);
    system_exit_helper(-35);
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
    system_exit_helper(-532);
  }
  
  target->location = IN_FRAME;
  target->kpage = kpage;
  return success;
  //</Chiahua>
}

void page_fault_identifier (void *fault_addr, struct intr_frame *f, bool user) 
{
  //<Chiahua>
  page srch;
  page *target;
  uint8_t * kpage;
  struct hash_elem *target_elem;
  if(lock_held_by_current_thread (&frame_lock))
  {
      lock_release(&frame_lock);
  }
  srch.upage = pg_round_down (fault_addr);
  target_elem = hash_find(thread_current ()->spt, &srch.hash_element);
  //Not part of our virtual address space.
  if (target_elem == NULL) 
  {
      //if user or if kernel
      void *stack_pointer = f->esp;
      if(!user)
      {
        stack_pointer = thread_current()->esp;
      }
      if(fault_addr >= (stack_pointer - 32))
      {
        //is stack growth
        kpage = frame_find_empty ();
        if (kpage != NULL) 
        {
          page *entry = page_build ((uint8_t *)(pg_round_down(fault_addr)), 0, 
                                      true, 0, 0, IN_FRAME, 0, 
                                      thread_current()->pagedir);
          page_add(entry);
          page_install_to_frame (entry, entry->upage, kpage, entry->pagedir);
        }
      }
      else
      {
        system_exit_helper(-1);
      }
  }
  else 
  {
    target = hash_entry (target_elem, page, hash_element);
    if (target->location == IN_SWAP)
    {
      //Grab from Swap
      kpage = frame_find_empty();
      lock_acquire(&frame_lock);
      swap_read (kpage, target);
      lock_release(&frame_lock);
      page_install_to_frame (target, target->upage, kpage, 
                      target->pagedir);
      
    }
    else if (target->location == IN_FILESYS)
    {
       //Read from filesys
       page_read_install (target);
    }
  }  
}
//</Chiahua>
