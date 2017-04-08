

void page_init (void);
page_hash (const struct hash_elem *p_, void *aux UNUSED);
page_less (const struct hash_elem *a_, const struct hash_elem *b_, 
           void *aux UNUSED);
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
