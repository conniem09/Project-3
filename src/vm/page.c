#include "page.h"

struct hash spt;

//<Documentation>
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct page *p = hash_entry (p_, struct page, hash_element);
  return hash_bytes (&p->upage, sizeof (p->upage));
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const struct page *a = hash_entry (a_, struct page, hash_element);
  const struct page *b = hash_entry (b_, struct page, hash_element);

  return a->upage < b->upage;
}
//</Documentation>

void
page_init ()
{
  hash_init (&spt, page_hash, page_less, NULL);
}


struct page * 
page_add (struct page *entry) 
{
  
  
  //<ChiaHua>
  hash_insert (&spt, entry->hash_element);
  return entry;
  //</ChiaHua>
}

void 
page_change_state (struct page *entry, int state)
{
  entry->location = state;
}

void
page_read_install (struct page *target)
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
  if (kpage == NULL)
  {
    //Evict page. Find again
  }
  // Load this page. 
  //page entry = Page_hash_find(upage virtual address);
  struct file *file = target->file;
  uint8_t *upage = target->upage;
  uint32_t page_read_bytes = target->page_read_bytes;
  uint32_t page_zero_bytes = target->page_zero_bytes;
  if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
  {
    frame_free (kpage);
    //return false; 
    system_exit_helper(-1);
  }
  memset (kpage + page_read_bytes, 0, page_zero_bytes);
  //</Connie>
  //<Chiahua>
  page_install_to_frame(upage, kpage);
  //</Chiahua>
}

void
page_install_to_frame (uint8_t *upage, uint8_t *kpage)
{
	
	
  // Add the page to the process's address space.
  //<Chiahua>
  bool writable = false; //TEMP
  if (!install_page (upage, kpage, writable)) 
  {
    page (kpage);
    return false; 
  }
  struct page srch;
  struct page *target;
  struct hash_elem *target_elem;
  srch.upage = upage;
  target_elem = hash_find(&spt, &srch.hash_element);
  target->location = IN_FRAME;
  //</Chiahua>
}

void page_fault_identifier (void *fault_addr) 
{
  //<Chiahua>
  struct page srch;
  struct page *target;
  struct hash_elem *target_elem;
  srch.upage = fault_addr;
  target_elem = hash_find(&spt, &srch.hash_element);
  //Not part of our virtual address space. Segmentation Fault
  if (target_elem == NULL) 
  {
    printf("There is crying in Pintos!\n");
    system_exit_helper(-1);
  }
  else 
  {
    target = hash_entry (target_elem, struct page, hash_element);
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
  }
  //</Chiahua>
  /*
   * Notes: Be careful of bringing in the wrong data, like, treating code as data or data as code.
   * In thread create, as long as not the main thread, initialise supplemental page table entry?
   */
  
}
