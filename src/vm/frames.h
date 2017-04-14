#include <inttypes.h>
#include <stdbool.h> 

#define TOTAL_PAGES 367

void frame_table_init (void);
uint8_t * frame_find_empty (void);
struct frame_table_elem frame_get_index_by_virtual(void);
void frame_free (uint8_t *kpage);
bool frame_install (uint8_t *upage, uint8_t *kpage, bool writable);


struct frame_table_elem
{
  uint8_t *kpage;
  uint8_t *upage;
  uint32_t *pagedir; 
};
