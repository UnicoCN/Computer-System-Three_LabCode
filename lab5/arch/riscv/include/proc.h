// arch/riscv/include/proc.h
#ifndef _PROC_H
#define _PROC_H

#include "types.h"


#define NR_TASKS  (1 + 1) // 用于控制 最大线程数量 （idle 线程 + 1 内核线程）

#define TASK_RUNNING    0 // 为了简化实验，所有的线程都只有一种状态

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

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
    struct vm_area_struct *mmap;       /* list of VMAs */
    uint64 user_sp;
    uint64 kernel_sp;
};

struct pt_regs {
	unsigned long x[32];
	unsigned long sepc;
	unsigned long sstatus;
};

/* 用于记录 `线程` 的 `内核栈与用户栈指针` */
struct thread_info {
    uint64 kernel_sp;
    uint64 user_sp;
};

/* 线程状态段数据结构 */
struct thread_struct {
    uint64 ra;
    uint64 sp;
    uint64 s[12];
    uint64 sepc, sstatus, sscratch; 
};

typedef unsigned long* pagetable_t;

/* 线程数据结构 */
struct task_struct {
    struct thread_info* thread_info;
    uint64 state;    // 线程状态
    uint64 counter;  // 运行剩余时间 
    uint64 priority; // 运行优先级 1最低 10最高
    uint64 pid;      // 线程id

    struct thread_struct thread;

    pagetable_t pgd;

    struct mm_struct *mm;

    struct pt_regs *trapframe;
};

/* 线程初始化 创建 NR_TASKS 个线程 */ 
void task_init(); 

/* 在时钟中断处理中被调用 用于判断是否需要进行调度 */
void do_timer();

int Get_task_id();

/* 调度程序 选择出下一个运行的线程 */
void schedule();

/* 线程切换入口函数*/
void switch_to(struct task_struct* next);

/* dummy funciton: 一个循环程序，循环输出自己的 pid 以及一个自增的局部变量*/
void dummy();

struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr);

uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot);

void do_page_fault(uint64 scause, uint64 sepc, struct pt_regs *regs);

int create_link(struct mm_struct *mm, uint64 addr, uint64 length, int prot);

#endif