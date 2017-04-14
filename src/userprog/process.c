/* Student Information
 * Chia-Hua Lu              Sabrina Herrero             Connie Chen
 * CL38755                  SH44786                     CMC5837
 * thegoldflute@gmail.com   sabrinaherrero123@gmail.com conniem09@gmail.com
 * 52075                    52105                       52105
 * 
 * Cristian Martinez
 * CJM4686
 * criscubed@gmail.com
 * 52080
 */

#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/string.h"
#include "threads/synch.h"
#include "vm/page.h"
#include "userprog/syscall.h"
#include "vm/frames.h"

#define ALIGN 4 /* align to multiple of this number */
#define AVAIL_STACK_SPACE 4084 /* Stack size - bytes needed for ret  
							address, int argc, and argv[] address */

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);
//our code
int getStatus (struct thread *parent, tid_t child_tid);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;
  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL) 
  {
    palloc_free_page (fn_copy);
    return TID_ERROR;
  }
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  //tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  tid = register_child_with_parent (file_name, PRI_DEFAULT, start_process, 
                                   fn_copy, thread_current ());
 
  if (tid != -1)
    return tid;
  else {
    //palloc_free_page (fn_copy); 
    return -1;
  }
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);
  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
  {
    thread_exit ();
  }
  //sema_up (&thread_current()->parent->forking);

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid) 
{
  //<chiahua>
  //search see if is our child
  struct list_elem *e;
  struct thread *parent = thread_current();
  struct child *child_node;
  struct thread *child_thread;
  int stat;

  stat = -1;
  for (e = list_begin (&parent->child_list); 
       e != list_end (&parent->child_list); e = list_next (e))
  {
    child_node = list_entry (e, struct child, elem);
    child_thread = child_node->kid;
    if (child_thread->tid == child_tid) 
    {
      //notify child that it is being waited on,
      //block ourself until child is ready to exit
      sema_down (&child_thread->block_parent);
      
      //parent woke up, child has exit status ready
      stat = (int) child_thread->exit_status;     
      
      //remove child node from our children list because it dying
      list_remove (&child_node->elem);
      palloc_free_page(child_node);
      
      //allow child to ded XP
      sema_up (&child_thread->block_child);
      return stat;
    }
  }
  return stat;
}


/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, const char *file_name);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;
  int numLeadingSpaces = 0;

  //<chiahua>
  char fileArg0[128];
  int copyIndex = 0;
  while (*(file_name + copyIndex) == ' ')
  {
    copyIndex++;
  }
  numLeadingSpaces = copyIndex;
  for ( ; *(file_name + copyIndex) != ' ' 
       && *(file_name + copyIndex) != '\0'; copyIndex++) 
  {
    fileArg0[copyIndex - numLeadingSpaces] = *(file_name + copyIndex);
  }
  fileArg0[copyIndex - numLeadingSpaces] = '\0';
  
  //initiate the supplemental page table
  t->spt = (struct hash*) malloc (sizeof (struct hash));
  page_init (t->spt);
  
  //</chiahua>

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /* Open executable file. */
  file = filesys_open (fileArg0);

  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", fileArg0);
      goto done;
    }
  //<sabrina>
  thread_current ()->command_line = file;
  strlcpy (thread_current ()->name, fileArg0, sizeof thread_current ()->name);
  //</sabrina>
  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", fileArg0);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp, file_name))
    goto done;
  
  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
 if (success)
   file_deny_write (file);
   
  else {
    thread_current ()->tid = -1;
  }
  /* We arrive here whether the load is successful or not. */
  //notify parent child finished loading
  sema_up (&thread_current ()->parent->wait_for_load);
  //file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs); 

  while (read_bytes > 0 || zero_bytes > 0) 
    {
   /* Calculate how to fill this page.
      We will read PAGE_READ_BYTES bytes from FILE
      and zero the final PAGE_ZERO_BYTES bytes. */
         
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;
      
      //<Connie, Chiahua>
      //build the page and add it to the frame table. 
      page *entry = page_build(upage, file, writable, page_read_bytes, 
                              page_zero_bytes, IN_FILESYS, ofs);
      page_add (entry);
      //</Connie, Chiahua>      
      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      //<cris>
      ofs += PGSIZE; 
	    //</cris>
      upage += PGSIZE;
    }   

  return true;
}

/* ********************************************************************
  
  //Chiahua>
  struct page *entry = malloc(sizeof(struct page));
  entry->upage = upage;
  entry->file = file;
  entry->page_read_bytes = page_read_bytes;
  entry->page_zero_bytes = page_zero_bytes;
  page_add (entry);
  page_read_install(upage);
  read_bytes -= page_read_bytes;
  zero_bytes -= page_zero_bytes;
  upage += PGSIZE;*/
  //</Chiahua>
  
  

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp,const char *file_name) 
{
  uint8_t *kpage;
  bool success = false;
  //<connie>
  char *fn_copy;
  int argc;
  char *dataAddresses[128];
  int index;
  char *my_esp = *esp;
  int x;
  char *temp_esp;
  int command_line_length;
  

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (PAL_ZERO);
  if (fn_copy == NULL) 
  {
    //palloc_free_page (fn_copy);
    return TID_ERROR;
  }
  //strlcpy (fn_copy, file_name, PGSIZE);
  //</connie>  

  //kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  kpage = frame_find_empty ();
  if (kpage != NULL) 
    {
      page *entry = page_build (((uint8_t *) PHYS_BASE) - PGSIZE, 0, true, 0,
                                0, IN_FRAME, 0);
      page_add(entry);
      success = page_install_to_frame (entry, entry->upage, kpage);
      //success = frame_install (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        my_esp = PHYS_BASE;
      else
      {
        frame_free (kpage);
        system_exit_helper(-102);
      }
    }
    else 
      system_exit_helper(-6);
  strlcpy (fn_copy, file_name, PGSIZE);
  //</connie>  
 
  //<chiahua, cris>
  //check if the commandline would overflow the stack
  command_line_length = strlen (fn_copy)+1;
  if (command_line_length > AVAIL_STACK_SPACE)
    return false;
  //move the stack pointer and copy the data into the stack
  my_esp = my_esp - command_line_length;
  strlcpy (my_esp, fn_copy, command_line_length);
  //</chiahua, cris>
  palloc_free_page(fn_copy);


  //<cris>
  //counting the number of arguments
  //null terminating the arguments
  //storing the addresses of the data
  x = 0;
  while (*(x + my_esp) == ' ')
  {
    x++;
  }
  argc = 1;
  dataAddresses[0] = my_esp;
  for ( ; (void*) (x + my_esp) < PHYS_BASE; x++)
  {
    if (*(x + my_esp) == ' ')
    {
      *(x + my_esp) = '\0';
      while (*(x + my_esp + 1) == ' ')
      {
        x++;
      }
      if (*(x + my_esp + 1) != ' ' && *(x + my_esp + 1) != '\0')
      {
        argc++;
      } 
      dataAddresses[argc - 1] = x + my_esp + 1;
    }
  }
  //</cris>
  
  // Check if pushing args onto stack would overflow it
  if (AVAIL_STACK_SPACE - command_line_length < argc * sizeof 
	  (dataAddresses[0]))
  {
    return false;
  }

  //<sabrina>  
  dataAddresses[argc] = NULL;
  
  //align addresses to multiples of 4
  while ((int) my_esp % ALIGN)
  {
    my_esp--;
    *my_esp = '\0';
  }

  //adding address to stack 
  for (index = argc; index >= 0; index--)
  {
    my_esp -= sizeof (dataAddresses[index]);
    //cast my_esp to an int* to expand to a 4 byte number 
    *((int*) my_esp) = (int) dataAddresses[index];
  }
  
  //push argv
  temp_esp = my_esp;
  my_esp -=  sizeof (temp_esp);
  *((int*) my_esp) = (int) temp_esp; 

  //push argc
  my_esp -=  sizeof (int);
  *((int*) my_esp) = argc;

  //push return address
  my_esp -=  sizeof (void*);
  *((int*) my_esp) = 0; 
  //</sabrina>
  
  *esp = my_esp;
  //hex_dump((int) *esp, *esp, PHYS_BASE-*esp, 1);
    
  return success;
}

bool
install_page_external (void *upage, void *kpage, bool writable)
{
  return install_page (upage, kpage, writable);
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
