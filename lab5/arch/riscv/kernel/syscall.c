#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include "printk.h"
#include<stddef.h>

extern struct task_struct* current;
extern void ret_from_fork(struct pt_regs *trapframe);
extern unsigned long swapper_pg_dir[512];
extern uint64 cnt;
extern struct task_struct* task[6];

int sys_getpid() {
    return current->pid;
}
void sys_write(unsigned int fd, const char* buf, unsigned int count) {
    printk("%s", buf);
}

void forkret() {
    ret_from_fork(current->trapframe);
}

uint64 do_fork(struct pt_regs *regs) {
    // printk("[PID = %d] fork [PID = %d]\n",current->pid,cnt);
    task[cnt] = (struct task_struct*)kalloc();
    task[cnt]->state = TASK_RUNNING;
    task[cnt]->counter = 0;
    task[cnt]->pid = cnt;
    task[cnt]->priority = rand();
    task[cnt]->thread.ra = (uint64)forkret;
    task[cnt]->thread.sp = (uint64)task[cnt] + PGSIZE;
    task[cnt]->thread.sscratch = task[cnt]->thread.sp;
    task[cnt]->thread.sepc = regs->sepc;
    task[cnt]->mm = kalloc();
    task[cnt]->mm->mmap = NULL;
    task[cnt]->trapframe = kalloc();

    uint64* user_stack = kalloc();
    uint64 sscratch = csr_read(sscratch);
    for (int i = 0; i < 512; ++i)
        user_stack[i] = ((uint64*)(USER_END - PGSIZE))[i];
    task[cnt]->mm->user_sp = (uint64)user_stack + PGSIZE;
    uint64* root_pg_tbl = kalloc();
    task[cnt]->pgd = (uint64)root_pg_tbl - PA2VA_OFFSET;
    for (int i = 0; i < 512; ++i)
        root_pg_tbl[i] = swapper_pg_dir[i];
    struct vm_area_struct* cur_mmap = current->mm->mmap, *child, *prev_mmap = task[cnt]->mm->mmap;
    while (cur_mmap) {
        child = kalloc();
        child->vm_start = cur_mmap->vm_start;
        child->vm_end = cur_mmap->vm_end;
        child->vm_flags = cur_mmap->vm_flags;
        if (prev_mmap == NULL) {
            task[cnt]->mm->mmap = child;
        } else prev_mmap->vm_next = child;
        child->vm_prev = prev_mmap;
        prev_mmap = child;
        cur_mmap = cur_mmap->vm_next;
    }
    prev_mmap->vm_next = NULL;
    task[cnt]->thread.sstatus = csr_read(sstatus);
    task[cnt]->thread.sstatus = task[cnt]->thread.sstatus | 0x00040020;
    task[cnt]->trapframe->sepc = regs->sepc;
    task[cnt]->trapframe->sstatus = regs->sstatus;
    for (int i = 0; i < 32; ++i)
        task[cnt]->trapframe->x[i] = regs->x[i];
    task[cnt]->trapframe->x[2] = csr_read(sscratch);
    task[cnt]->trapframe->x[10] = 0;
    return cnt++;
}

uint64 clone(struct pt_regs *regs) {
    return do_fork(regs);
}

