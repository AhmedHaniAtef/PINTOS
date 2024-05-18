#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

void syscall_init (void);

/* check if the pointer is valid */
bool validate_void_ptr(const void* pt);

void syscall_exit(int status);

#endif /* userprog/syscall.h */