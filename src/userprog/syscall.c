#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

static struct lock files_sys_lock;               /* lock for syschronization between files */

static void syscall_handler (struct intr_frame *);

struct open_file* get_file(int fd);

void exit_handler(struct intr_frame *cur_frame);
void exec_handler(struct intr_frame *cur_frame);
void wait_handler(struct intr_frame *cur_frame);
void create_handler(struct intr_frame *cur_frame);
void remove_handler(struct intr_frame *cur_frame);
void open_handler(struct intr_frame *cur_frame);
void filesize_handler(struct intr_frame *cur_frame);
void read_handler(struct intr_frame *cur_frame);
void write_handler(struct intr_frame *cur_frame);
void seek_handler(struct intr_frame *cur_frame);
void tell_handler(struct intr_frame *cur_frame);
void close_handler(struct intr_frame *cur_frame);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  lock_init(&files_sys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{ 
  struct intr_frame *cur_frame = f;

  if (validate_void_ptr(cur_frame->esp))
    syscall_exit(-1);
  
  switch (*(int*)cur_frame->esp)
  {
  case SYS_HALT:
    shutdown_power_off();
    break;
  
  case SYS_EXIT:
    exit_handler(cur_frame);
    break;

  case SYS_EXEC:
    exec_handler(cur_frame);
    break;

  case SYS_WAIT:
    wait_handler(cur_frame);
    break;

  case SYS_CREATE:
    create_handler(cur_frame);
    break;

  case SYS_REMOVE:
    remove_handler(cur_frame);
    break;

  case SYS_OPEN:
    open_handler(cur_frame);
    break;

  case SYS_FILESIZE:
    filesize_handler(cur_frame);
    break;

  case SYS_READ:
    read_handler(cur_frame);
    break;

  case SYS_WRITE:
    write_handler(cur_frame);
    break;

  case SYS_SEEK:
    seek_handler(cur_frame);
    break;

  case SYS_TELL:
    tell_handler(cur_frame);
    break;

  case SYS_CLOSE:
    close_handler(cur_frame);
    break;

  default:
    // negative area
    break;
  }

}

bool 
validate_void_ptr(const void* pt)
{
  if (pt == NULL || !is_user_vaddr(pt) || pagedir_get_page(thread_current()->pagedir, pt) == NULL) 
  {
    return true;
  }
  return false;
}

void
syscall_exit(int status)
{
  struct thread* parent = thread_current()->parent;
  printf("%s: exit(%d)\n", thread_current()->name, status);
  if(parent) parent->child_status = status;
  thread_exit();
}

void
exit_handler(struct intr_frame *cur_frame)
{
  if (validate_void_ptr(cur_frame->esp+4))
    syscall_exit(-1);
  int status = *((int*)cur_frame->esp + 1);
  syscall_exit(status);
}

int
sys_exec(char* file_name)
{
  return process_execute(file_name);
}

void
exec_handler(struct intr_frame *cur_frame)
{ 
  if (validate_void_ptr(cur_frame->esp+4))
    syscall_exit(-1);
  char* name = (char*)(*((int*)cur_frame->esp + 1));

  if (name == NULL) syscall_exit(-1);
  lock_acquire(&files_sys_lock);
  cur_frame->eax = sys_exec(name);
  lock_release(&files_sys_lock);
}

int
sys_wait(int pid)
{
  return process_wait(pid);
}

void
wait_handler(struct intr_frame *cur_frame)
{
  if (validate_void_ptr(cur_frame->esp+4))
    syscall_exit(-1);
  int tid = *((int*)cur_frame->esp + 1);

  cur_frame->eax = sys_wait(tid);
}

bool
sys_create(char* name, size_t size)
{
  bool res;
  lock_acquire(&files_sys_lock);

  res = filesys_create(name,size);

  lock_release(&files_sys_lock);
  return res;
}

void
create_handler(struct intr_frame *cur_frame)
{
  for (int i = 4; i <= 8; i += 4)
  {
    if (validate_void_ptr(cur_frame->esp+i))
      syscall_exit(-1);
  }
  
  char* name = (char*)(*((int*)cur_frame->esp + 1));
  size_t size = *((int*)cur_frame->esp + 2);

  if (name == NULL) syscall_exit(-1);

  cur_frame->eax = sys_create(name,size);
}

bool
sys_remove(char* name)
{
  bool res;
  lock_acquire(&files_sys_lock);

  res = filesys_remove(name);

  lock_release(&files_sys_lock);
  return res;
}

void
remove_handler(struct intr_frame *cur_frame)
{

  if (validate_void_ptr(cur_frame->esp+4))
    syscall_exit(-1);

  char* name = (char*)(*((int*)cur_frame->esp + 1));

  if (name == NULL) syscall_exit(-1);

  cur_frame->eax = sys_remove(name);
}

int
sys_open(char* name)
{
  struct open_file* open = palloc_get_page(0);
  if (open == NULL) 
  {
    palloc_free_page(open);
    return -1;
  }
  lock_acquire(&files_sys_lock);
  open->ptr = filesys_open(name);
  lock_release(&files_sys_lock);
  if (open->ptr == NULL)
  {
    return -1;
  }
  open->fd = ++thread_current()->fd_last;
  list_push_back(&thread_current()->open_file_list,&open->elem);
  return open->fd;
}

void
open_handler(struct intr_frame *cur_frame)
{
  if (validate_void_ptr(cur_frame->esp+4))
    syscall_exit(-1);

  char* name = (char*)(*((int*)cur_frame->esp + 1));

  if (name == NULL) syscall_exit(-1);

  cur_frame->eax = sys_open(name);
}

int
sys_filesize(int fd)
{
  struct thread* t = thread_current();
  struct file* my_file = get_file(fd)->ptr;

  if (my_file == NULL)
  {
    return -1;
  }
  int res;
  lock_acquire(&files_sys_lock);
  res = file_length(my_file);
  lock_release(&files_sys_lock);
  return res;
}

void
filesize_handler(struct intr_frame *cur_frame)
{
  if (validate_void_ptr(cur_frame->esp+4))
    syscall_exit(-1);
  int fd = *((int*)cur_frame->esp + 1);

  cur_frame->eax = sys_filesize(fd);
}

int
sys_read(int fd,void* buffer, int size)
{
  if (fd == 0)
  {
    
    for (size_t i = 0; i < size; i++)
    {
      lock_acquire(&files_sys_lock);
      ((char*)buffer)[i] = input_getc();
      lock_release(&files_sys_lock);
    }
    return size;
    
  } else {

    struct thread* t = thread_current();
    struct file* my_file = get_file(fd)->ptr;

    if (my_file == NULL)
    {
      return -1;
    }
    int res;
    lock_acquire(&files_sys_lock);
    res = file_read(my_file,buffer,size);
    lock_release(&files_sys_lock);
    return res;
    
  }
}

void
read_handler(struct intr_frame *cur_frame)
{
  for (int i = 4; i <= 12; i += 4)
  {
    if (validate_void_ptr(cur_frame->esp+i))
      syscall_exit(-1);
  }

  int fd, size;
  void* buffer;
  fd = *((int*)cur_frame->esp + 1);
  buffer = (void*)(*((int*)cur_frame->esp + 2));
  size = *((int*)cur_frame->esp + 3);

  if (validate_void_ptr(buffer + size))
    syscall_exit(-1);
  
  cur_frame->eax = sys_read(fd,buffer,size);
}

int
sys_write(int fd, void* buffer, int size)
{

  if (fd == 1)
  {
    
    lock_acquire(&files_sys_lock);
    putbuf(buffer,size);
    lock_release(&files_sys_lock);
    return size;

  } else {
    
    struct thread* t = thread_current();
    struct file* my_file = get_file(fd)->ptr;

    if (my_file == NULL)
    {
      return -1;
    }
    int res;
    lock_acquire(&files_sys_lock);
    res = file_write(my_file,buffer,size);
    lock_release(&files_sys_lock);
    return res;
  }

}

void
write_handler(struct intr_frame *cur_frame)
{

  for (int i = 4; i <= 12; i += 4)
  {
    if (validate_void_ptr(cur_frame->esp+i))
      syscall_exit(-1);
  }

  int fd, size;
  void* buffer;
  fd = *((int*)cur_frame->esp + 1);
  buffer = (void*)(*((int*)cur_frame->esp + 2));
  size = *((int*)cur_frame->esp + 3);

  if (buffer == NULL) syscall_exit(-1);
  
  cur_frame->eax = sys_write(fd,buffer,size);
}

void
sys_seek(int fd, unsigned pos)
{
  struct thread* t = thread_current();
  struct file* my_file = get_file(fd)->ptr;

  if (my_file == NULL)
  {
    return;
  }

  lock_acquire(&files_sys_lock);
  file_seek(my_file,pos);
  lock_release(&files_sys_lock);
}

void
seek_handler(struct intr_frame *cur_frame)
{
  for (int i = 4; i <= 8; i += 4)
  {
    if (validate_void_ptr(cur_frame->esp+i))
      syscall_exit(-1);
  }

  int fd;
  unsigned pos;
  fd = *((int*)cur_frame->esp + 1);
  pos = *((unsigned*)cur_frame->esp + 2);

  sys_seek(fd,pos);
}

unsigned
sys_tell(int fd)
{ 
  struct thread* t = thread_current();
  struct file* my_file = get_file(fd)->ptr;

  if (my_file == NULL)
  {
    return -1;
  }

  unsigned res;
  lock_acquire(&files_sys_lock);
  res = file_tell(my_file);
  lock_release(&files_sys_lock);
  return res;
}

void
tell_handler(struct intr_frame *cur_frame)
{
  validate_void_ptr(cur_frame->esp + 4);
  int fd = *((int*)cur_frame->esp + 1);

  cur_frame->eax = sys_tell(fd);
}

void
sys_close(int fd)
{
  struct thread* t = thread_current();
  struct open_file* my_file = get_file(fd);

  if (my_file == NULL)
  {
    return;
  }

  lock_acquire(&files_sys_lock);
  file_close(my_file->ptr);
  lock_release(&files_sys_lock);
  list_remove(&my_file->elem);
  palloc_free_page(my_file);
}

void
close_handler(struct intr_frame *cur_frame)
{
  validate_void_ptr(cur_frame->esp + 4);
  int fd = *((int*)cur_frame->esp + 1);
  sys_close(fd);
}

struct open_file* get_file(int fd){
    struct thread* t = thread_current();
    struct file* my_file = NULL;
    for (struct list_elem* e = list_begin (&t->open_file_list); e != list_end (&t->open_file_list);
    e = list_next (e))
    {
      struct open_file* opened_file = list_entry (e, struct open_file, elem);
      if (opened_file->fd == fd)
      {
        return opened_file;
      }
    }
    return NULL;
}