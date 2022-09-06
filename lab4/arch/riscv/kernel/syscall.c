#include "syscall.h"
#include "proc.h"
#include "defs.h"

extern struct task_struct* current;
int sys_getpid(){
    return current->pid;
}
void sys_write(unsigned int fd, const char* buf, unsigned int count){
    printk("%s", buf);
}