/* Student Information
 * Chia-Hua Lu              Cristian Martinez     Connie Chen
 * CL38755                  CJM4686               CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com   conniem09@gmail.com
 * 52075                    52080                 52105
 */
 
//<Chia-Hua, Cristian, Connie> 
#include <inttypes.h>
#include <stdbool.h> 
#include "vm/page.h"
#define TOTAL_PAGES 367

struct lock frame_lock;

void frame_table_init (void);
uint8_t * frame_find_empty (void);
struct frame_table_elem frame_get_index_by_virtual(void);
void frame_free (uint8_t *kpage);
bool frame_install (uint8_t *upage, uint8_t *kpage, uint32_t *pagedir,
                    bool writable, page *page_pointer);
uint8_t * frame_evict (void);

struct frame_table_elem
{
  uint8_t *kpage;
  uint8_t *upage;
  uint32_t *pagedir; 
  page *page_pointer;
};
//</Chia-Hua, Cristian, Connie>
