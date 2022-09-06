//arch/riscv/kernel/proc.c

#include "defs.h"
#include "mm.h"
#include "proc.h"
#include "printk.h"
#include "rand.h"
#include "syscall.h"
extern unsigned long swapper_pg_dir[512];

extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
extern char uapp_start[];
extern char uapp_end[];
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
struct task_struct* idle;           // idle process
struct task_struct* current;        //point to the current process
struct task_struct* task[6]; // store all processes
uint64 sum=NR_TASKS;

void task_init() {

    uint64 i;

    idle=(struct task_struct*)kalloc();
    idle->thread.sp=(uint64)idle+PGSIZE;
    idle->state=TASK_RUNNING;
    idle->counter=0;
    idle->priority=0;
    idle->pid=0;

    current=idle;
    task[0]=idle;

    for(i=1;i<NR_TASKS;i++){
         task[i]=(struct task_struct*)kalloc();
         task[i]->state=TASK_RUNNING;
         task[i]->counter=0;
         task[i]->pid=i;
         task[i]->priority=rand();
         task[i]->thread.ra=(uint64)__dummy;
         task[i]->thread.sp=(uint64)task[i]+PGSIZE;
	 task[i]->mm=kalloc();
         task[i]->mm->mmap=NULL;
	 task[i]->trapframe=kalloc();
         //allocate to store user_stack
	 unsigned long * user_stack=kalloc();
         task[i]->mm->user_sp=(unsigned long)user_stack+PGSIZE;//pa of bottom of user stack vitural address
	 //task[i]->thread_info->user_sp=user_stack+PGSIZE-PA2VA_OFFSET;
         //allocate to store kernel page
         unsigned long * kernel_pg=kalloc();
         task[i]->pgd=(unsigned long)kernel_pg-PA2VA_OFFSET;//physical address
	 //printk("kalloc pgtbl for task %d is at addres : 0x%lx\n",i,(unsigned long)task[i]->pgd+PA2VA_OFFSET);
         for(int i=0;i<512;i++)
             kernel_pg[i]=swapper_pg_dir[i];
	 //allocate vma struct
	 //task[i]->thread_info=kalloc();
	 do_mmap(task[i]->mm,USER_START,uapp_end - uapp_start,VM_READ | VM_WRITE | VM_EXEC);
	 do_mmap(task[i]->mm,USER_END - PGSIZE,PGSIZE,VM_READ | VM_WRITE);
	 //create_mapping(kernel_pg,USER_END-PGSIZE,(unsigned long)user_stack-PA2VA_OFFSET,PGSIZE,0b10111);
         //create_mapping(kernel_pg,USER_START,(unsigned long)uapp_start-PA2VA_OFFSET,(unsigned long)uapp_end-(unsigned long)uapp_start,0b11111);
         task[i]->thread.sstatus=csr_read(sstatus);
         task[i]->thread.sstatus|=0x00040020;
         //task[i]->thread.sstatus=(csr_read(sstatus))|0x00040020;
         csr_write(sstatus,task[i]->thread.sstatus);
         task[i]->thread.sepc=USER_START;
         task[i]->thread.sscratch=USER_END;
    }
    printk("...proc_init done!\n");

}


void do_page_fault(unsigned long scause,unsigned long sepc,struct pt_regs *regs){
    unsigned long stval,va,pa,sz;
    __asm__ __volatile__ ("csrr %0, stval\n\t" : "=r" (stval));
    printk("[S] PAGE_FAULT: scause: %ld, sepc: 0x%lx, badaddr: 0x%lx\n",scause,sepc,stval);
    struct vm_area_struct* found=find_vma(current->mm, stval);
    if(found){
	 ;
    	//printk("vm found! badaddr:0x%lx is of vm:0x%lx with start:0x%lx and end:0x%lx\n",stval,found->vm_mm,found->vm_start,found->vm_end);
    }
    else{
	printk("vm is not found with badaddr:0x%lx \n",stval);
	return;
    }
    if(scause ==12&&(found->vm_flags&VM_EXEC)==0) {
        printk("Invalid vm area in page fault\n");
        return;
    } else if(scause==13&&(found->vm_flags&VM_READ)==0){
        printk("Invalid vm area in page fault\n");
        return;
    }else if(scause==15&&(found->vm_flags&VM_WRITE)==0){
        printk("Invalid vm area in page fault\n");
        return;
    }
    if(found->vm_start==USER_START){
        va=USER_START;
        pa=(unsigned long)uapp_start-PA2VA_OFFSET;
        sz=(unsigned long)uapp_end-(unsigned long)uapp_start;
        create_mapping((unsigned long)current->pgd+PA2VA_OFFSET,USER_START,(unsigned long)uapp_start-PA2VA_OFFSET,(unsigned long)uapp_end-(unsigned long)uapp_start,0b11111);
        //printk("[S] mapped PA: 0x%lx to VA: 0x%lx with size: 0x%lx \n",pa,va,sz);
    }
    else if(found->vm_start==USER_END-PGSIZE){
	va=USER_END-PGSIZE;
	pa=current->mm->user_sp-PGSIZE-PA2VA_OFFSET;
        sz=PGSIZE;	
        //printk("user stack page fault!\n");
	create_mapping((unsigned long)current->pgd+PA2VA_OFFSET,USER_END-PGSIZE,pa,PGSIZE,0b10111);
    }
    else{
        va=found->vm_start;
        pa=(unsigned long)kalloc()-PA2VA_OFFSET;
        sz=PGSIZE;
	//printk("mapping is creating... va: 0x%lx => pa: 0x%lx \n",va,pa);
        create_mapping((unsigned long)current->pgd+PA2VA_OFFSET,found->vm_start,pa,PGSIZE,(found->vm_flags<<1));
    }
    printk("[S] mapped PA: 0x%lx to VA: 0x%lx with size: 0x%lx \n",pa,va,sz);
}


/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr){
    struct vm_area_struct* root=mm->mmap;
    while(root){
        if(addr>=root->vm_start&&addr<root->vm_end){
	    break;
	}
	root=root->vm_next;
    }
    return root;
}
void print_vma(struct mm_struct *mm){
   struct vm_area_struct* tmp=mm->mmap;
   printk("===========now vma link list===========\n");
   for(;tmp;tmp=tmp->vm_next){
       printk("vm->vm_mm :0x%lx  vm->vm_start:0x%lx vm->vm_end:0x%lx \n",tmp->vm_mm,tmp->vm_start,tmp->vm_end);
       printk("vm->vm_length:0x%lx vm->vm_flags:0x%lx \n",tmp->vm_end-tmp->vm_start,tmp->vm_flags);
   }
}
int add_link(struct mm_struct *mm, uint64 addr, uint64 length, int prot){
   struct vm_area_struct* vm =kalloc();
   struct vm_area_struct* tmp=mm->mmap,*fol;
   while(tmp&&tmp->vm_start<(addr+length)){
        fol=tmp;
        tmp=tmp->vm_next;
   }
   if(tmp==mm->mmap){
       vm->vm_start=addr;
       vm->vm_end=addr+length;
       vm->vm_prev=NULL;
       vm->vm_next=mm->mmap;
       vm->vm_mm=mm;
       vm->vm_flags=(uint64)prot;
       if(mm->mmap)
           mm->mmap->vm_prev=vm;
       mm->mmap=vm;
   }
   else if(tmp&&tmp!=mm->mmap&&addr>=tmp->vm_prev->vm_end){
       vm->vm_start=addr;
       vm->vm_end=addr+length;
       vm->vm_prev=tmp->vm_prev;
       vm->vm_next=tmp;
       tmp->vm_prev->vm_next=vm;
       tmp->vm_prev=vm;
       vm->vm_mm=mm;
       vm->vm_flags=(uint64)prot;
  }
   else if(tmp==NULL&&addr>=fol->vm_end){
       fol->vm_next=vm;
       vm->vm_start=addr;
       vm->vm_end=addr+length;
       vm->vm_prev=fol;
       vm->vm_next=NULL;
       vm->vm_mm=mm;
       vm->vm_flags=(uint64)prot;
   }
   else
       return 0;
   return 1;
}

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot){
   while(!add_link(mm,addr,length,prot)){
       addr=get_unmapped_area(mm,length);
   }
   //print_vma(mm);
   return addr;
}

int find(struct mm_struct *mm,uint64 addr,uint64 length){
   struct vm_area_struct* tmp=mm->mmap,*fol;
   while(tmp){
        if(addr>=tmp->vm_start&&addr<tmp->vm_end)
	    return 1;
	else if((addr+length)>tmp->vm_start&&(addr+length)<=tmp->vm_end)
            return 1;
	tmp=tmp->vm_next;
   }
   return 0;

}

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length){
   uint64 start=0;
   for(start=0;start<=0xC0000000&&find(mm,start,length);start+=PGSIZE);
   return start;
}

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d thread space begin at %lx \n", current->pid, auto_inc_local_var,current);
        }
    }
}

void switch_to(struct task_struct* next) {
    struct task_struct *tmp=current;
    if(current==next);
    else{
        printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",next->pid,next->priority,next->counter);
        current=next;
        __switch_to(tmp,next);
    }
    return;
}

void do_timer(void) {
    if((current==idle)||(!(--current->counter)))
        schedule();
    return;
}
//shortest-job first
#ifdef SJF
int Findtask(void){
    uint64 i,index=-1;
    for(i=1;i<sum;i++){
        if((task[i]->counter)&&(task[i]->state==TASK_RUNNING)){
            if((index==-1)||(task[i]->counter<task[index]->counter))
                index=i;
        }
    }
    return index;
}
#endif
//priority
#ifdef PRIORITY
int Findtask(void){
    uint64 i,index=-1;
    for(i=1;i<sum;i++){
        if((task[i]->counter)&&(task[i]->state==TASK_RUNNING)){
            if((index==-1)||(task[i]->priority>task[index]->priority))
                index=i;
        }
    }
   return index;
}
#endif
void schedule(void){
    uint64 index,i;
    uint64 cnt=0;
    while((index=Findtask())==-1){
        for(i=1;i<sum;i++){
            if(task[i]->state==TASK_RUNNING){
                task[i]->counter=(uint64)rand();
            }
       }
        cnt++;
    }
    if(cnt){
        for(i=1;i<sum;i++){
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n",task[i]->pid,task[i]->priority,task[i]->counter);
        }
    }
    //printk("%x",(uint64)task[index]);
    //before=current;
    //current=task[index];
    switch_to(task[index]);
    return;
}

