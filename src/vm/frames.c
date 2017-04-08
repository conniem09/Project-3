#include "threads/synch.h"
#include "threads/malloc.h"

#define TOTAL_PAGES 367

struct lock lock;
/* Array of frame_table_elems */
struct frame_table_elem frame_table[TOTAL_PAGES];

struct frame_table_elem
{
  uint8_t *kpage;
  uint8_t *upage;
  uint32_t *pagedir; 
};

//<cris>
//initiates the frame table
void 
frame_table_init () 
{ 
  int i;
  frame_table_elem *x;
  do
  {
    //initiate the frame_table_elem
    frame_table[i].kpage = palloc_get_page(PAL_USER);
    frame_table[i].upage = NULL;
    frame_table[i].pagedir = NULL;
    
    //add the frame_table_elem to the array
    frame_table[i] = (void*) x;
    i++;
  } while (i < TOTAL_PAGES && x != NULL);
  
}
//</cris>

//<cris>
//puts a upage into a kapage
void
frame_find_empty ()
{
  int i;
  if (space available)
  {
    for (i = 0; i < TOTAL_PAGES; i++)
    {
      if (frame_table[i]-> upage == NULL)
      {
        return frame_table[i]->kpage;
      }
    }
  }
  else
  {
    printf ("No Additional Available Frames\n");
    system_exit_helper(-1);
    //i = evict();
  }

}
//</cris>

//<chiahua>
struct frame_table_elem
frame_get_index_by_virtual()
{
  int index;
  for (index = 0; index < TOTAL_PAGES; index++)
  {
    
  }
}
//</chiahua>
