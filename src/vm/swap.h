#include "devices/block.h"
#include "threads/vaddr.h"
#include "lib/kernel/bitmap.h"
#include "vm/page.h"

void swap_init (void);
block_sector_t swap_get_free (void);
void swap_read (const void *buffer, page page_pointer);
void swap_write (const void *buffer, page page_pointer);
