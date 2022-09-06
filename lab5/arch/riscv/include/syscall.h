#ifndef _SYSCALL_H_
#define _SYSCALL_H

#define SYS_WRITE   64
#define SYS_GETPID  172
#define SYS_CLONE 220


int sys_getpid();
void sys_write(unsigned int fd, const char* buf, unsigned int count);

#endif