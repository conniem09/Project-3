#include "lib/user/syscall.h"
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

//<sabrina, connie, cris>
void system_halt(void);
void system_exit(void *stack_pointer);
pid_t system_exec(void *stack_pointer);
int system_wait(void *stack_pointer);
bool system_create(void *stack_pointer);
bool system_remove(void *stack_pointer);
int system_open(void *stack_pointer);
int system_filesize(void *stack_pointer);
int system_read(void *stack_pointer);
int system_write(void *stack_pointer);
void system_seek(void *stack_pointer);
unsigned system_tell(void *stack_pointer);
void system_close(void *stack_pointer);

void check_pointer(void* pointer);
//</sabrina, connie, cris>

static void syscall_handler (struct intr_frame *);

struct lock filesys_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

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
      system_halt();
      break;
    case SYS_EXIT:
      system_exit(stack_pointer);
      break;
    case SYS_EXEC:
      f->eax = system_exec(stack_pointer);
      break;
    case SYS_WAIT:
      f->eax = system_wait(stack_pointer);
      break;
    case SYS_CREATE:
      f->eax = system_create(stack_pointer);
      break;
    case SYS_REMOVE:
      f->eax = system_remove(stack_pointer);
      break;
    case SYS_OPEN:
      f->eax = system_open(stack_pointer);
      break;
    case SYS_FILESIZE:
      f->eax = system_filesize(stack_pointer);
      break;
    case SYS_READ:
      f->eax = system_read(stack_pointer);
      break;
    case SYS_WRITE:
      f->eax = system_write(stack_pointer);
      break;
    case SYS_SEEK:
      system_seek(stack_pointer);
      break;
    case SYS_TELL:
      f->eax = system_tell(stack_pointer);
      break;
    case SYS_CLOSE:
      system_close(stack_pointer);
      break;
    //Invalid system call  
    default:              
      system_exit_helper(-1);
  }
  //</cris, connie, chiahua, sabrina>  
  
  //thread_exit ();
}

void 
system_halt()
{
  shutdown_power_off();
}

void 
system_exit(void *stack_pointer)
{
  //<chiahua, sabrina>
  int e_status;
  
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  e_status = *(int*) stack_pointer;
  //</sabrina>
  
  system_exit_helper(e_status);
  //</chiahua>
}

void
system_exit_helper(int e_status)
{
  //<chiahua, sabrina, cris>
  thread_current()->exit_status = e_status;
  printf("%s: exit(%d)\n",thread_current()->name, e_status);
  thread_exit();
  //</chiahua, sabrina, cris>
}


pid_t 
system_exec(void *stack_pointer)
{
  //<sabrina, cris>
  int file_name;
  pid_t pid;
  
  //get file_name and size from the stack
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer (stack_pointer);
  file_name = *(int*) stack_pointer;
  check_pointer((int*)file_name);
  
  pid = process_execute((char*)file_name);
  return pid; 
 //</sabrina, cris>
}

int 
system_wait(void *stack_pointer)
{
  //<chiahua, cris>
  int pid;
  
  //get file_name and size from the stack
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  pid = *(int*) stack_pointer;
  return process_wait((tid_t)pid);
  //</chiahua, cris>
}

bool 
system_create(void *stack_pointer)
{
  //<connie, cris>
  int file_name;
  int size;
  bool result;
  
  //get file_name and size from the stack
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer (stack_pointer);
  file_name = *(int*) stack_pointer;
  check_pointer((int*)file_name);
  
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  size = *(int*) stack_pointer;
  
  lock_acquire(&filesys_lock);
  result = filesys_create((char *)file_name, size);
  lock_release(&filesys_lock);
  return result;
  //</connie, cris>
}

bool  
system_remove(void *stack_pointer)
{

  //<connie, cris>
  int file_name;
  bool result;
  //get file_name from the stack
  stack_pointer =  (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  file_name = *(int*) stack_pointer;
  
  check_pointer((int*)file_name);

  lock_acquire(&filesys_lock);
  result = filesys_remove((char *)file_name);
  lock_release(&filesys_lock);
  return result;
  //</connie, cris>
}

int 
system_open(void *stack_pointer)
{
  //<sabrina, cris>
  int i;
  int file_name;
  struct thread *cur = thread_current();
  struct file *open_file;
  
  //check for available space
  if (cur-> num_open_files >= MAX_FILES)
    return -1;
  
  //store the file descriptor
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer (stack_pointer);
  file_name = *(int*) stack_pointer;
  check_pointer((int*)file_name);
  
  //check to see if file opened successfully 
  lock_acquire(&filesys_lock);
  open_file = filesys_open((char *)file_name);
  lock_release(&filesys_lock);
  if(open_file == NULL)
    return -1;
  
  //iterate through the array to find empty spots 
  for(i = 0; i < MAX_FILES; i++)
  {
    if(cur-> fd_pointers[i] == 0)
      break;
  }
  
  //if valid then put in array
  cur-> fd_pointers[i] = open_file;
  cur-> num_open_files++;
  
  //fd is equal to index plus offset of 2 to account for 0 and 1 not available
  return (i + 2);
  //</sabrina, cris>
}

//Returns the size, in bytes, of the file open as fd.
int 
system_filesize(void *stack_pointer)
{
  //<connie, cris>
  int fd;
  struct file* pointer;
  int result;
  
  //store the file descriptor
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  fd = *(int*) stack_pointer;
  
  //checks that fd is in range
  if (fd >= 2 && fd <= MAX_FILES + 2)
  {
    pointer = thread_current()->fd_pointers[fd-2];
    if((int)pointer)
    {
      lock_acquire(&filesys_lock);
      result = file_length(pointer);
      lock_release(&filesys_lock);
      return result;
    }
  }
  //</connie, cris>
  return -1;
}

int 
system_read(void *stack_pointer)
{
  //<connie, cris>
  //read paramaters off the stack 
  int fd;
  int buffer;
  int length;
  struct file* pointer;
  int result;
  
  //store the file descriptor
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  fd = *(int*) stack_pointer;

  //store the buffer parameter
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  buffer = *(int*) stack_pointer;
  check_pointer ((void*) buffer);  
  
  //store the length parameter
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  length = *(int*) stack_pointer;
  
  //if fd = 0, get input from keyboard
  if (fd == 0)
  {
    return input_getc();
  }
  
  //checks that fd is in range
  else if (fd >= 2 && fd <= MAX_FILES + 2)
  {
    pointer = thread_current()->fd_pointers[fd-2];
    if((int)pointer)
    {
      lock_acquire(&filesys_lock);
      result = file_read(pointer, (int*)buffer, length); 
      lock_release(&filesys_lock);
      return result;
    }
  }
  
  return -1;
  //</connie, cris>

}

//<cris, chiahua, connie, sabrina>
int 
system_write(void *stack_pointer)
{
  //read paramaters off the stack 
  int fd;
  int string;
  int length;
  int result;
  
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
    return (int) length; 
  }
  
  //if fd is anything else like a file or something
  else 
  {
    //write to file
    //<chiahua>
    if (fd >= 2 && fd <= MAX_FILES + 2)
    {
      if (thread_current()->fd_pointers[fd-2])
      {
        lock_acquire(&filesys_lock);
        result = file_write(thread_current()->fd_pointers[fd-2], (void*) string, length);
        lock_release(&filesys_lock);
        return result;
      }
    }
    //</chiahua>
  }
  return 0;
} 
//</cris, chiahua, connie, sabrina>


void 
system_seek(void *stack_pointer)
{
  //<connie, chiahua>
  int fd;
  int position;
  struct file* pointer;
  
  //get fd and position from the stack
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  fd = *(int*) stack_pointer;
  
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  position = *(int*) stack_pointer;
  
  //checks that fd is in range
  if (fd >= 2 && fd <= MAX_FILES + 2)
  {
    pointer = thread_current()->fd_pointers[fd-2];
    if((int)pointer)
    {
      lock_acquire(&filesys_lock);
      file_seek(pointer, position);
      lock_release(&filesys_lock);
    }
  }
  //</connie, chiahua>
}

unsigned system_tell(void *stack_pointer)
{
  //<connie, chiahua>
  int fd;
  struct file* pointer;
  unsigned result;
  
  //get fd from the stack
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  fd = *(int*) stack_pointer;
  
  //checks that fd is in range
  if (fd >= 2 && fd <= MAX_FILES + 2)
  {
    pointer = thread_current()->fd_pointers[fd-2];
    if((int)pointer)
    {
      lock_acquire(&filesys_lock);
      result = file_tell(pointer);
      lock_release(&filesys_lock);
      return result;
    }
  }
  //</connie, chiahua>
  return 0;
}

void system_close(void *stack_pointer)
{
  //<connie, chiahua>
  int fd;
  struct file* pointer;
  
  //get fd from the stack
  stack_pointer = (int*) stack_pointer + 1;
  check_pointer ((void*) stack_pointer);
  fd = *(int*) stack_pointer;
  
  //checks that fd is in range
  if (fd >= 2 && fd <= (MAX_FILES + 2))
  {
    pointer= thread_current()->fd_pointers[fd-2];
    if((int)pointer)
    {
      lock_acquire(&filesys_lock);
      file_close(pointer);
      lock_release(&filesys_lock);
      thread_current()->num_open_files --;
      thread_current()->fd_pointers[fd-2] = NULL;
    }
  }
  //</connie, chiahua>
}

//<cris>
void check_pointer(void* pointer)
{
  if(pointer == NULL || is_kernel_vaddr(pointer) || !pagedir_get_page (thread_current() -> pagedir, pointer))
  {
    system_exit_helper(-1);
  }
}
//</cris>
