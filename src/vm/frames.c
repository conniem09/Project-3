#include "vm/frames.h"
#include "userprog/syscall.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

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
  struct frame_table_elem *x = NULL;
  do
  {
	x = (struct frame_table_elem * ) malloc (sizeof (struct frame_table_elem));
    //initiate the frame_table_elem
    x->kpage = palloc_get_page(PAL_USER);
	x->upage = NULL;
    x->pagedir = NULL;
    
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
	  if (frame_table[i]->kpage == kpage)
	  {
		frame_table[i]->upage = NULL;
		occupancy--;
	  }
	}
}

bool
frame_install (uint8_t *upage, uint8_t *kpage, bool writable)
{
	bool success = install_page_external (upage, kpage, writable);
	if (success) 
	{
		int i;
		for (i = 0; i < TOTAL_PAGES; i++)
		{
		  if (frame_table[i]->kpage == kpage)
		  {
			if (frame_table[i]->upage != NULL)
			  system_exit_helper(-111);
				
			frame_table[i]->upage = upage;
			occupancy++;
		  }
		}
	}
		return success;		
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
    printf ("Evict. No Additional Available Frames\n");
    system_exit_helper(-17);
    ASSERT(false);
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
