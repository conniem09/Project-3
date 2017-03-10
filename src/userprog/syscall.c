#include "lib/user/syscall.h"
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "devices/shutdown.h"

//<sabrina, connie>
void check_pointer(void* pointer);

void system_halt(void);
void system_exit(int);
pid_t system_exec(const char *);
int system_wait(pid_t);
bool system_create(const char *, unsigned);
bool system_remove(const char *);
int system_open(const char *);
int system_filesize(int);
int system_read(int, void *, unsigned);
int system_write(void *stack_pointer);
void system_seek(int, unsigned);
unsigned system_tell(int);
void system_close(int);
//</sabrina, connie>

static void syscall_handler (struct intr_frame *);
//int write; 
// Debuging Method  void debug_print(char* pointer, int bytes);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// DEBUG TEMP CODE

/*
void 
debug_print(char* pointer, int bytes)
{
  int outsideIndex;
  int insideIndex;
  char *reader = pointer;
  while ((int)reader&1 != 0)
  {
   // (char*) reader = (int) reader - 1;
  }
  for (outsideIndex = 0; outsideIndex+(unsigned)(char*) pointer<0xc0000010; outsideIndex++) {
    printf("%8x:   ", (int)(char*) pointer);
    for (insideIndex = 0; insideIndex < 16; insideIndex++) {
      int byteHere = *((char*)(unsigned)pointer);
      if (byteHere<0)
        byteHere = -byteHere;
      if (pointer<0xc0000000)
      printf("%02x ", byteHere) ;
      (char*) pointer++;
    }
    printf(" |\n");
  } 
}
*/

static void
syscall_handler (struct intr_frame *f) 
{
  //<cris, connie, chiahua, sabrina>
  void *stack_pointer =  (void*) f->esp;
  
  //check the stack pointer to make sure it is not NULL and in user memory
  check_pointer(stack_pointer);
  
  //save syscall_number which is at the top of the stack
  int syscall_number = *(int*) stack_pointer;
  //debug_print((char*)stack_pointer, 40);
  switch (syscall_number)
  {
    case SYS_HALT:
      printf("Halt\n");
      system_halt();
      break;
    case SYS_EXIT:
      printf("Exit\n");
      //system_exit();
      break;
    case SYS_EXEC:
      printf("Exec\n");
      //system_exec();
      break;
    case SYS_WAIT:
      printf("wait\n");
      //system_wait();
      break;
    case SYS_CREATE:
      printf("Create\n");
      //system_create();
      break;
    case SYS_REMOVE:
      printf("Remove\n");
      //system_remove();
      break;
    case SYS_OPEN:
      printf("Open\n");
      //system_open();
      break;
    case SYS_FILESIZE:
      printf("Filesize\n");
      //system_filesize();
      break;
    case SYS_READ:
      printf("Read\n");
      //system_read();
      break;
    case SYS_WRITE:
      printf("Write\n");
      f->eax = system_write(stack_pointer);
      break;
    case SYS_SEEK:
      printf("Seek\n");
      //system_seek();
      break;
    case SYS_TELL:
      printf("Tell\n");
      //system_tell();
      break;
    case SYS_CLOSE:
      printf("Close\n");
      //system_close();
      break;
    //Invalid system call  
    default:              
      printf("I cannot believe you've done this :( \n");
    //exit(-1);
  }
  //</cris, connie, chiahua, sabrina>  
  
  printf ("Successful system call!\n");
  
  //thread_exit ();
}

void 
system_halt()
{
  shutdown_power_off();
}

void 
system_exit(int status)
{
    
}

pid_t 
system_exec(const char *cmd_line)
{
 //possibly one line long(?)
 
}

int 
system_wait(pid_t pid)
{
 //possibly one line long(?)
  
}

bool 
system_create(const char *file, unsigned initial_size)
{
  
}

bool  
system_remove(const char *file)
{
  
}

int 
system_open(const char *file)
{
  
}

//Returns the size, in bytes, of the file open as fd.
int 
system_filesize(int fd)
{
  
}

int 
system_read(int fd, void *buffer, unsigned size)
{
  
}

//<cris>
int 
system_write(void *stack_pointer)
{
  //read paramaters off the stack 
  int fd;
  int string;
  int length;
  
  //store the file descriptor
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  fd = *(int*) stack_pointer;
  
  //store the string parameter
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  string = *(int*) stack_pointer;
  check_pointer ((void*) string);  
  
  //store the length parameter
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  length = *(int*) stack_pointer;

  /*
  printf("FD = %d\n", fd);
  printf("String = %x\n", (int)string);
  printf("Len = %d\n", length);
  */
  
  //if fd is stdin
  if (fd == 0)  
  {
    //DONT DO ANYTHING >:c
  } 
  
  //if fd is stdout
  else if (fd == 1) 
  {
    //write the string to the console
    putbuf ((char*)string, (size_t) length); 
  }
  
  //if fd is anything else like a file or something
  else 
  {
    //write to file
  }
  return (int) length;
}
//</cris>


void 
system_seek(int fd, unsigned position)
{
  
}

unsigned system_tell(int fd)
{

}

void system_close(int fd)
{

}

void check_pointer(void* pointer)
{
  if(pointer == NULL || is_kernel_vaddr(pointer))
  {
    printf("I'd rather you didn't do that: WRONG POINTER\n");
    thread_exit ();
  }
}
