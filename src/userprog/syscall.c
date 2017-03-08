#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
//int write; 

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  //<cris, connie>
  void *stack_pointer =  f->esp;
  
  int syscall_number = *(int*) stack_pointer;
  printf("%d\n", syscall_number);
  printf("%d\n", SYS_HALT);
  
  switch (syscall_number)
  {
    case SYS_HALT:
      printf("Halt\n");
      shutdown_power_off();
      break;
    case SYS_EXIT:
      printf("Exit\n");
      break;
    case SYS_EXEC:
      printf("Exec\n");
      break;
    case SYS_WAIT:
      printf("wait\n");
      break;
    case SYS_CREATE:
      printf("Create\n");
      break;
    case SYS_REMOVE:
      printf("Remove\n");
      break;
    case SYS_OPEN:
      printf("Open\n");
     
      break;
    case SYS_FILESIZE:
      printf("Filesize\n");
      break;
    case SYS_READ:
      printf("Read\n");
      break;
    case SYS_WRITE:
      printf("Write\n");
      unsigned length;
      char *string;
      int x;
      
      stack_pointer += sizeof (int);
      //stack_pointer += sizeof (int);
      string = (char *) stack_pointer;
      stack_pointer += sizeof (const void *);
      length = (unsigned) stack_pointer;
      printf("%s\n", *string);
      /* for(x = 0; x <= length; x++)
      {
       printf("%c\n", *string);
       string++;
      } */
      break;
    case SYS_SEEK:
      printf("Seek\n");
      break;
    case SYS_TELL:
      printf("Tell\n");
      break;
    case SYS_CLOSE:
      printf("Close\n");
      break;
    //Invalid system call  
    default:              
      printf("I cannot believe you've done this :( \n");
    //exit(-1);
  }
  //</cris>  
  
  printf ("Successful system call!\n");
  
  thread_exit ();
}
