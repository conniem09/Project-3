/* Student Information
 * Chia-Hua Lu              Cristian Martinez     Connie Chen
 * CL38755                  CJM4686               CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com   conniem09@gmail.com
 * 52075                    52080                 52105
 */
#include "vm/frames.h"
#include "userprog/syscall.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

struct lock lock;
/* Array of frame_table_elems */
struct frame_table_elem *frame_table[TOTAL_PAGES];
int occupancy;
int clock_pointer;

//<Cris>
//initiates the frame table
void 
frame_table_init () 
{ 
  occupancy = 0;
  clock_pointer = 0;
  int i = 0;
  lock_init (&lock);
  struct frame_table_elem *x = NULL;
  do
  {
    //initiate the frame_table_elem
    x = (struct frame_table_elem *) malloc (sizeof (struct frame_table_elem));
    x->kpage = palloc_get_page(PAL_USER);
    x->upage = NULL;
    x->pagedir = NULL;
    x->page_pointer = NULL;
    
    //add the frame_table_elem to the array
    frame_table[i] = x;
    i++;
  } while (i < TOTAL_PAGES && x != NULL);
  
}
//</Cris>

//<Chiahua, Cris>
//labels a frame as available 
void
frame_free (uint8_t *kpage)
{
  lock_acquire (&lock);
	int i;
  //iterate through all pages
	for (i = 0; i < TOTAL_PAGES; i++)
	{
    //label the current kpage as available
	  if (frame_table[i]->kpage == kpage)
	  {
      frame_table[i]->upage = NULL;
      frame_table[i]->pagedir = NULL;
      frame_table[i]->page_pointer = NULL;
      occupancy--;
	  }
	}
  lock_release (&lock);
}

//install a virtual page into a physical page
bool
frame_install (uint8_t *upage, uint8_t *kpage, uint32_t *pagedir, bool writable,
                page *page_pointer)
{
  lock_acquire (&lock);
  //Move the upage to the kpage
	bool success = install_page_external (upage, kpage, writable);
	if (success) 
	{
		int i;
    //find the kpage that we installed to
		for (i = 0; i < TOTAL_PAGES; i++)
		{
		  if (frame_table[i]->kpage == kpage)
		  {
        //upage at this point should be null
        if (frame_table[i]->upage != NULL)
        {
          lock_release (&lock);
          system_exit_helper(-1);
        }
        //update frame table metadata
        frame_table[i]->upage = upage;
        frame_table[i]->pagedir = pagedir;
        frame_table[i]->page_pointer = page_pointer;
        occupancy++;
		  }
		}
	}
  lock_release (&lock);
  return success;		
}
//</Chiahua, Cris>

//<Cris>
//find an unoccupied frame
uint8_t * 
frame_find_empty ()
{
  lock_acquire (&lock);
  int i;
  if (occupancy < TOTAL_PAGES)
  {
    for (i = 0; i < TOTAL_PAGES; i++)
    {
      if (frame_table[i]->upage == NULL)
      {
        lock_release (&lock);
        return frame_table[i]->kpage;
      }
    }
  }
  else
  {
    //printf ("Evict. No Additional Available Frames\n");
        lock_release (&lock);

    uint8_t *result = frame_evict();
    return result;
  }
  lock_release (&lock);
  return NULL;
}

uint8_t *
frame_evict ()
{
  uint8_t * result = NULL;
  bool found = false;
  while(!found){
    if(pagedir_is_accessed (frame_table[clock_pointer]->pagedir, 
       frame_table[clock_pointer]->upage) == 0)
    {
      found = true;
      swap_write(frame_table[clock_pointer]->upage, 
                 frame_table[clock_pointer]->page_pointer);
      frame_table[clock_pointer]->upage = NULL;
      frame_table[clock_pointer]->pagedir = NULL;
      frame_table[clock_pointer]->page_pointer->location = IN_SWAP;
      frame_table[clock_pointer]->page_pointer = NULL;
      occupancy--;
      result = frame_table[clock_pointer]->kpage;
    }
    else
    {
      pagedir_set_accessed (frame_table[clock_pointer]->pagedir, 
       frame_table[clock_pointer]->upage, 0); 
    }
    clock_pointer++;
    if(clock_pointer == TOTAL_PAGES)
    {
      clock_pointer = 0;
    }
  }
  return result;
}
//</Cris>
