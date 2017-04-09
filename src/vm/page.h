#include "lib/kernel/hash.h"
#include "filesys/file.h"

void page_init (void);
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED)
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED);
struct page * page_add (struct page *entry);
void page_change_state (struct page *entry, int state);
void page_read_install (struct page *target);
void page_install_to_frame (uint8_t *upage, uint8_t *kpage);
void page_fault_identifier (void *fault_addr); 
#define FRAME  1        /* Page is in Page Frame */
#define SWAP 2          /* Page is in Swap */
#define FILESYS  3      /* Page is in Filesys */


struct page
{
  uint8_t *upage;
  struct file *file;
  struct hash_elem *hash_element;
  size_t page_read_bytes;
  size_t page_zero_bytes;    
  int location;
  
}
