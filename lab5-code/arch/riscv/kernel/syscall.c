#include "syscall.h"
#include "proc.h"
#include "printk.h"
#include "defs.h"
extern struct task_struct* current;
extern struct task_struct* task[6];
extern uint64 sum;
extern void ret_from_fork(struct pt_regs *trapframe);
extern unsigned long swapper_pg_dir[512];

int sys_getpid(){
    return current->pid;
}
unsigned int sys_write(unsigned int fd, const char* buf, unsigned int count){
    if(fd==1){
        printk("%s",buf);
    	return count;
    }
}

void forkret() {
    ret_from_fork(current->trapframe);
}

uint64 do_fork(struct pt_regs *regs) {
    //printk("[PID = %d] fork [PID = %d]\n",current->pid,sum);
    task[sum]=(struct task_struct*)kalloc();
    task[sum]->state=TASK_RUNNING;
    task[sum]->counter=0;
    task[sum]->pid=sum;
    task[sum]->priority=rand();
    task[sum]->thread.ra=(uint64)forkret;
    task[sum]->thread.sp=(uint64)task[sum]+PGSIZE;
    task[sum]->thread.sscratch=task[sum]->thread.sp;
    task[sum]->thread.sepc=regs->sepc;
    task[sum]->mm=kalloc();
    //printk("kalloc for mm: 0x%lx  user_sp: 0x%lx \n",task[sum]->mm,task[sum]->mm->user_sp);
    task[sum]->mm->mmap=NULL;
    task[sum]->trapframe=kalloc();
    //printk("regs initial finished! allocating user_stack...\n");
    //allocate to store user_stack
    unsigned long * user_stack=kalloc();
    unsigned long sscratch=csr_read(sscratch);
    for(int i=0;i<512;i++){
	user_stack[i]=((unsigned long*)(USER_END-PGSIZE))[i];
        //printk("child  user stack:0x%lx  0x%lx\n",(unsigned long*)(USER_END-PGSIZE)+i,user_stack[i]);
    }
    //printk("user stack coping finished!\n");
    (task[sum]->mm)->user_sp=(unsigned long)user_stack+PGSIZE;//pa of bottom of user stack vitural address
    //task[i]->thread_info->user_sp=user_stack+PGSIZE-PA2VA_OFFSET;
    //allocate to store kernel page
    //printk("assigning user_stack to user_sp finished!\n");
    unsigned long * kernel_pg=kalloc();
    //printk("allocating pagetbl finished!\n");
    task[sum]->pgd=(unsigned long)kernel_pg-PA2VA_OFFSET;//physical address
    //printk("kalloc pgtbl for task %d is at addres : 0x%lx\n",sum,(unsigned long)task[sum]->pgd+PA2VA_OFFSET);
    for(int i=0;i<512;i++)
        kernel_pg[i]=swapper_pg_dir[i];
    //copy  vma struct
    //printk("page initial finished! copying vma struct ....\n");
    struct vm_area_struct* tmp=current->mm->mmap,*child,*prev=task[sum]->mm->mmap;
    while(tmp){
	child=kalloc();
	child->vm_mm=task[sum]->mm;
	child->vm_start=tmp->vm_start;
	child->vm_end=tmp->vm_end;
	child->vm_flags=tmp->vm_flags;
	if(prev==NULL){
	    task[sum]->mm->mmap=child;
	}
	else{
	    prev->vm_next=child;
	}
	child->vm_prev=prev;
	prev=child;
	tmp=tmp->vm_next;
    }
    prev->vm_next=NULL;
   // tmp=task[sum]->mm->mmap;
   // printk("===========now vma link list of child process===========\n");
   //for(;tmp;tmp=tmp->vm_next){
   //    printk("vm->vm_mm :0x%lx  vm->vm_start:0x%lx vm->vm_end:0x%lx \n",tmp->vm_mm,tmp->vm_start,tmp->vm_end);
   //    printk("vm->vm_length:0x%lx vm->vm_flags:0x%lx \n",tmp->vm_end-tmp->vm_start,tmp->vm_flags);
   //}
    //task[i]->thread_info=kalloc();
    //do_mmap(task[sum]->mm,USER_START,uapp_end - uapp_start,VM_READ | VM_WRITE | VM_EXEC);
    //do_mmap(task[sum]->mm,USER_END - PGSIZE,PGSIZE,VM_READ | VM_WRITE);
    task[sum]->thread.sstatus=csr_read(sstatus);
    task[sum]->thread.sstatus|=0x00040020;
    //copy the regs into trapframe
    task[sum]->trapframe->sepc=regs->sepc; task[sum]->trapframe->sstatus=regs->sstatus;
    task[sum]->trapframe->x31=regs->x31; task[sum]->trapframe->x30=regs->x30; task[sum]->trapframe->x29=regs->x29; task[sum]->trapframe->x28=regs->x28;
    task[sum]->trapframe->x27=regs->x27; task[sum]->trapframe->x26=regs->x26; task[sum]->trapframe->x25=regs->x25; task[sum]->trapframe->x24=regs->x24;
    task[sum]->trapframe->x23=regs->x23; task[sum]->trapframe->x22=regs->x22; task[sum]->trapframe->x21=regs->x21; task[sum]->trapframe->x20=regs->x20;
    task[sum]->trapframe->x19=regs->x19; task[sum]->trapframe->x18=regs->x18; task[sum]->trapframe->x17=regs->x17; task[sum]->trapframe->x16=regs->x16;
    task[sum]->trapframe->x15=regs->x15; task[sum]->trapframe->x14=regs->x14; task[sum]->trapframe->x13=regs->x13; task[sum]->trapframe->x12=regs->x12;
    task[sum]->trapframe->x11=regs->x11; task[sum]->trapframe->x10=regs->x10; task[sum]->trapframe->x9=regs->x9;   task[sum]->trapframe->x8=regs->x8;
    task[sum]->trapframe->x7=regs->x7;   task[sum]->trapframe->x6=regs->x6;   task[sum]->trapframe->x5=regs->x5;   task[sum]->trapframe->x4=regs->x4;
    task[sum]->trapframe->x3=regs->x3;   task[sum]->trapframe->x2=regs->x2;   task[sum]->trapframe->x1=regs->x1;   task[sum]->trapframe->x0=regs->x0;
    //change sp(x2)
    task[sum]->trapframe->x2=csr_read(sscratch);
    //task[sum]->trapframe->x2-=8;
    //printk("sp: 0x%lx  s0: 0x%lx\n",task[sum]->trapframe->x2,task[sum]->trapframe->x8);
    //change a0(x10)
    task[sum]->trapframe->x10=0;
    return sum++;
}

uint64 clone(struct pt_regs *regs) {
    return do_fork(regs);
}
