
#include "vm/frames.h"

struct lock lock;
/* Array of frame_table_elems */
struct frame_table_elem *frame_table[TOTAL_PAGES];
int occupancy;

//<cris>
//initiates the frame table
void 
frame_table_init () 
{ 
  occupancy = 0;
  int i = 0;
  struct frame_table_elem *x = malloc(sizeof (struct frame_table_elem));
  do
  {
    //initiate the frame_table_elem
    frame_table[i]->kpage = palloc_get_page(PAL_USER);
    frame_table[i]->upage = NULL;
    frame_table[i]->pagedir = NULL;
    
    //add the frame_table_elem to the array
    frame_table[i] = x;
    i++;
  } while (i < TOTAL_PAGES && x != NULL);
  
}
//</cris>

void
frame_free (uint8_t *kpage)
{
	int i;
	for (i = 0; i < TOTAL_PAGES; i++)
	{
	  if (frame_table[i]->kpage == NULL)
	  {
		frame_table[i]->upage = NULL;
	  }
	}
}

//<cris>
//find an unoccupied frame
uint8_t * 
frame_find_empty ()
{
  int i;
  if (occupancy < TOTAL_PAGES)
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
    //printf ("No Additional Available Frames\n");
    system_exit_helper(-1);
    return NULL;
    //i = evict();
  }
  return NULL;
}
//</cris>

//<chiahua>
/*struct frame_table_elem
frame_get_index_by_virtual()
{
  int index;
  for (index = 0; index < TOTAL_PAGES; index++)
  {
    
  }
  return NULL;
}*/
//</chiahua>


//Need a method for installing things into frame
