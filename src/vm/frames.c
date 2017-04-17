/* Student Information
 * Chia-Hua Lu              Cristian Martinez     Connie Chen
 * CL38755                  CJM4686               CMC5837
 * thegoldflute@gmail.com   criscubed@gmail.com   conniem09@gmail.com
 * 52075                    52080                 52105
 */
#include <stdio.h>
#include "vm/frames.h"
#include "vm/swap.h"
#include "userprog/syscall.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"



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
  clock_pointer = 25;
  int i = 0;
  lock_init (&frame_lock);
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
    lock_acquire(&frame_lock);
    frame_table[i] = x;
    lock_release(&frame_lock);
    i++;
  } while (i < TOTAL_PAGES && x != NULL);
  
}
//</Cris>

//<Chiahua, Cris>
//labels a frame as available 
void
frame_free (uint8_t *kpage)
{
  lock_acquire (&frame_lock);
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
  lock_release (&frame_lock);
}

//install a virtual page into a physical page
bool
frame_install (uint8_t *upage, uint8_t *kpage, uint32_t *pagedir, bool writable,
                page *page_pointer)
{
  lock_acquire (&frame_lock);
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
          lock_release (&frame_lock);
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
  lock_release (&frame_lock);
  return success;		
}
//</Chiahua, Cris>

//<Cris>
//find an unoccupied frame
uint8_t * 
frame_find_empty ()
{
  lock_acquire (&frame_lock);
  int i;
  //check if all pages are occupied or not
  if (occupancy < TOTAL_PAGES)
  {
    //iterate through all the frames
    for (i = 0; i < TOTAL_PAGES; i++)
    {
      //and find one that is available
      if (frame_table[i]->upage == NULL)
      {
        lock_release (&frame_lock);
        return frame_table[i]->kpage;
      }
    }
  }
  else
  {
    lock_release (&frame_lock);
    uint8_t *result = frame_evict();
    return result;
  }
  //should not get here
  lock_release (&frame_lock);
  printf("No available page was found\n");
  return NULL;
}

//find and return a page to evict
uint8_t *
frame_evict ()
{
  uint8_t * result = NULL;
  bool found = false;
  //find a page to evict using clock algorithm
  while(!found){
    //if the accessed bit is 0
    if(pagedir_is_accessed (frame_table[clock_pointer]->pagedir, 
       frame_table[clock_pointer]->upage) == 0)
    {
      found = true;
      //Write the current page into swap
      pagedir_clear_page(frame_table[clock_pointer]->pagedir, 
                       frame_table[clock_pointer]->upage); 
      swap_write(frame_table[clock_pointer]->kpage, 
                 frame_table[clock_pointer]->page_pointer);
      //clear the metadata for the current frame table element
      frame_table[clock_pointer]->upage = NULL;
      frame_table[clock_pointer]->pagedir = NULL;
      frame_table[clock_pointer]->page_pointer->location = IN_SWAP;
      frame_table[clock_pointer]->page_pointer->kpage = NULL;
      frame_table[clock_pointer]->page_pointer = NULL;
      occupancy--;
      result = frame_table[clock_pointer]->kpage;
    }
    else
    {
      //if the accessed bit is not 0 then set it to 0
      pagedir_set_accessed (frame_table[clock_pointer]->pagedir, 
       frame_table[clock_pointer]->upage, 0); 
    }
    //increment the clock pointer
    clock_pointer++;
    //reset the clock pointer
    if(clock_pointer == TOTAL_PAGES)
    {
      clock_pointer = 25;
    }
  }
  //return the evicted page
  return result;
}
//</Cris>
