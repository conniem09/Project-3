#include "vm/swap.h"

struct block * swap;
struct bitmap * free_bitmap;

void
swap_init () 
{
  swap = block_get_role (BLOCK_SWAP);
  //bitmap is indexed by number of pages
  free_bitmap = bitmap_create (block_size (swap) * BLOCK_SECTOR_SIZE / PGSIZE);
  
}

block_sector_t 
swap_get_free ()
{
  int i;
  i = bitmap_scan(free_bitmap, 0, 1, false);
  bitmap_set (free_bitmap, i, true);
  bitmap_dump(free_bitmap);
  return (block_sector_t) i * 8;
}

void
swap_read (const void *buffer, page *page_pointer)
{
  int i;
  block_sector_t sector = page_pointer->swap_location;
  for (i = 0; i < 8; i++)
  {
    block_write (swap, sector, buffer);
    sector++;
    buffer += BLOCK_SECTOR_SIZE;
  }
}

void
swap_write (const void *buffer, page *page_pointer)
{
  int i;
  block_sector_t sector = swap_get_free();
  page_pointer->swap_location = sector;
  for (i = 0; i < 8; i++)
  {
    block_write (swap, sector, buffer);
    sector++;
    buffer += BLOCK_SECTOR_SIZE;
  }
}
