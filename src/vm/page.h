/* Student Information
 * Chia-Hua Lu              Cristian Martinez     Connie Chen
 * CL38755                  CJM4686               CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com   conniem09@gmail.com
 * 52075                    52080                 52105
 */
 
//<Chiahua, Cristian, Connie>
#ifndef INC_PAGEH

#define INC_PAGEH

#include <inttypes.h>
#include "devices/block.h"
#include "lib/kernel/hash.h"

#define IN_FRAME  1        /* Page is in Page Frame */
#define IN_SWAP 2          /* Page is in Swap */
#define IN_FILESYS  3      /* Page is in Filesys */

typedef struct page_t
{
  uint8_t *upage;
  uint8_t *kpage;
  struct file *file;
  bool writable;
  struct hash_elem hash_element;
  size_t page_read_bytes;
  size_t page_zero_bytes;    
  int location;
  block_sector_t swap_location;
  unsigned ofs;
  uint32_t *pagedir;
} page;

void page_init (struct hash *spt);
void page_clear_all (void);
unsigned page_hash (const struct hash_elem *p_, void *aux);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux);
page * page_add (page *entry);
page * page_build(uint8_t *upage, struct file *file, bool writable, 
                  size_t page_read_bytes, size_t page_zero_bytes, int location, 
                  unsigned ofs, uint32_t *pagedir);
void page_change_state (page *entry, int state);
void page_read_install (page *target);
bool page_install_to_frame (page *target, uint8_t *upage, uint8_t *kpage, 
                            uint32_t *pagedir);
void page_fault_identifier (void *fault_addr); 

#endif
//<//Chia-Hua, Cristian, Connie>
