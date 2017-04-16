/* Student Information
 * Chia-Hua Lu              Cristian Martinez     Connie Chen
 * CL38755                  CJM4686               CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com   conniem09@gmail.com
 * 52075                    52080                 52105
 */
#include "devices/block.h"
#include "threads/vaddr.h"
#include "lib/kernel/bitmap.h"
#include "vm/page.h"

void swap_init (void);
block_sector_t swap_get_free (void);
void swap_read (const void *buffer, page *page_pointer);
void swap_write (const void *buffer, page *page_pointer);
