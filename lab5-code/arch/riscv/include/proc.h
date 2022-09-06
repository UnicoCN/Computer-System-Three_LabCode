// arch/riscv/include/proc.h
#ifndef _PROC_H
#define _PROC_H

#include "types.h"

#define NR_TASKS  1+1

#define TASK_RUNNING    0 

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

/* vm_area_struct vm_flags */
#define VM_READ     0x00000001
#define VM_WRITE    0x00000002
#define VM_EXEC     0x00000004

struct vm_area_struct {
    struct mm_struct *vm_mm;    /* The mm_struct we belong to. */
    uint64 vm_start;          /* Our start address within vm_mm. */
    uint64 vm_end;            /* The first byte after our end address 
                                    within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    struct vm_area_struct *vm_next, *vm_prev;

    uint64 vm_flags;      /* Flags as listed above. */
};

struct mm_struct {
    struct vm_area_struct *mmap;    /* list of VMAs */
    uint64 user_sp;
    uint64 kernel_sp;
};

struct pt_regs{
    uint64 sstatus;
    uint64 sepc;
    uint64 x31,x30,x29,x28,x27,x26,x25,x24,x23,x22,x21,x20,x19,x18,x17,x16,x15,x14,x13,x12,x11,x10,x9,x8,x7,x6,x5,x4,x3,x2,x1,x0;
};

typedef unsigned long* pagetable_t;

struct thread_info {
    uint64 kernel_sp;
    uint64 user_sp;
};

struct thread_struct {
    uint64 ra;
    uint64 sp;
    uint64 s[12];

    uint64 sepc,sstatus,sscratch;
};

struct task_struct {
    struct thread_info* thread_info;
    uint64 state;
    uint64 counter;
    uint64 priority;
    uint64 pid;      //thread ID

    struct thread_struct thread;
    
    pagetable_t pgd;

    struct mm_struct *mm;
    struct pt_regs *trapframe;
};

void task_init(); 

void do_timer();

void schedule();

void switch_to(struct task_struct* next);

void dummy();

//void do_page_fault(unsigned long scause,unsigned long sepc,struct pt_regs *regs);
/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr);

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot);

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length);
#endif
