#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //<cris>
  int *stack_pointer = f->esp;
  
  if (*stack_pointer != NULL)
  {
    if (*stack_pointer >= PHYS_BASE)
    {
      printf ("Kernel Address Illegal SysCall!\n");
      thread_exit ();
    } 
  }
  else
  {
  //kill
  printf ("Null Pointer SysCall!\n");
  thread_exit ();
  }
  //</cris>
  printf ("Successful system call!\n");
  thread_exit ();
}
