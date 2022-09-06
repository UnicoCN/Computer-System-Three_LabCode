#include"proc.h"
#include"printk.h"
#include"mm.h"
#include"rand.h"
#include"defs.h"
#include"types.h"

extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
extern unsigned long swapper_pg_dir[512];
extern void __dummy();
extern void __stwich_to(struct task_struct *prev, struct task_struct *next);

extern char uapp_start[];
extern char uapp_end[];

void do_timer();
void schedule();
struct task_struct *idle;
struct task_struct *current;
struct task_struct *task[6];
uint64 cnt = NR_TASKS;

void task_init()
{
	// printk("...proc_init begin!\n");
	idle = (struct task_struct *)kalloc();
	idle -> state = TASK_RUNNING;
	idle -> counter = 0;
	idle -> priority = 0;
	idle -> pid = 0;
	idle -> thread.sp = PGSIZE + (uint64)idle; // sp -> High Address 

	task[0] = idle;
	current = idle;

	int i;
	for (i = 1; i < NR_TASKS; ++i)
	{
		task[i] = (struct task_struct*)kalloc();
		task[i] -> state = TASK_RUNNING;
		task[i] -> counter = 0;
		task[i] -> priority = rand();
		task[i] -> pid = i;
		task[i] -> thread.sp = PGSIZE + (uint64)task[i];
		task[i] -> thread.ra = (uint64)(__dummy);
		// printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n",i,task[i]->priority,task[i]->counter);

		task[i]->mm = kalloc();
		task[i]->mm->mmap = NULL;
		task[i]->trapframe = kalloc();

		// kalloc space for user stack
		uint64 *user_stack = (uint64*)kalloc();
		// set user_sp as the physical address of the bottom of user stack
		task[i]->mm->user_sp = (uint64)user_stack + PGSIZE;
		// create a root page table for each user process
		uint64 *root_pg_tbl = (uint64*)kalloc();
		// set user process page table address
		task[i]->pgd = (uint64)root_pg_tbl - PA2VA_OFFSET;
		// copy the kernel page table to user page table
		for (int i = 0; i < 512; ++i)
			root_pg_tbl[i] = swapper_pg_dir[i];
		// printk("before do_mmap\n");
		do_mmap(task[i]->mm, USER_START, uapp_end - uapp_start, VM_READ | VM_WRITE | VM_EXEC);
		do_mmap(task[i]->mm, USER_END - PGSIZE, PGSIZE, VM_READ | VM_WRITE);
		// printk("after do_mmap\n");
		// set sstatus
		task[i]->thread.sstatus = csr_read(sstatus);
		// set SUM(bit 18), so kernel mode can access user mode page 
		// set SPIE(bit 5), so interruption is enabled after sret
		// set SPP to be 0, so after calling mret, the system can return to user mode 
		task[i]->thread.sstatus = task[i]->thread.sstatus | 0x00040020;
		csr_write(sstatus, task[i]->thread.sstatus);
		// set sepc as USER_START
		// set sscratch as USER_END
		task[i]->thread.sepc =  USER_START;
		task[i]->thread.sscratch = USER_END;
	}
	printk("...proc_init done!\n");
	return;
}

void do_page_fault(uint64 scause, uint64 sepc, struct pt_regs *regs) {
	uint64 stval;
	__asm__ __volatile__ ("csrr %0, stval\n\t" : "=r" (stval));
	printk("[S] PAGE_FAULT: scause: %ld, sepc: 0x%lx, badaddr: 0x%lx\n", scause, sepc, stval);
	struct vm_area_struct* valid_vma = find_vma(current->mm, stval);
	if (!valid_vma) {
		printk("vm is not found with badaddr:0x%lx \n",stval);
		return;
	}
	if (scause == 12 && (valid_vma->vm_flags & VM_EXEC) == 0) {
		printk("Instruction Page Fault!\n");
		return;
	} else if (scause == 13 && (valid_vma->vm_flags & VM_READ) == 0) {
		printk("Load Page Fault!\n");
		return;		
	} else if (scause == 15 && (valid_vma->vm_flags & VM_WRITE) == 0) {
		printk("Store/AMO Page Fault!\n");
		return;
	}
	uint64 va, pa, size;
	if (valid_vma->vm_start == USER_START) {
		va = USER_START;
		pa = (uint64)uapp_start - PA2VA_OFFSET;
		size = (uint64)uapp_end - (uint64)uapp_start;
		create_mapping((uint64)current->pgd + PA2VA_OFFSET, USER_START, (uint64)uapp_start - PA2VA_OFFSET, (uint64)uapp_end - (uint64)uapp_start, 0b11111);
	} else if (valid_vma->vm_start == USER_END - PGSIZE) {
		va = USER_END - PGSIZE;
		pa = current->mm->user_sp - PGSIZE - PA2VA_OFFSET;
		size = PGSIZE;
		create_mapping((uint64)current->pgd + PA2VA_OFFSET, USER_END - PGSIZE, pa, PGSIZE, 0b10111);
	} else {
		va = valid_vma->vm_start;
		pa = (uint64)kalloc() - PA2VA_OFFSET;
		size = PGSIZE;
		create_mapping((uint64)current->pgd + PA2VA_OFFSET, valid_vma->vm_start, pa, PGSIZE, valid_vma->vm_flags << 1);
	}
	printk("[S] mapped PA: 0x%lx to VA: 0x%lx with size: 0x%lx \n",pa,va,size);
}

void dummy() 
{
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            if (current -> counter > 0) printk("[PID = %d] is running. auto_inc_local_var = %d, thread space begin at 0x%lx\n", current->pid, auto_inc_local_var,(unsigned long)current); 
        	else printk("Switching - - - - - -\n");
	}
}
}

void switch_to(struct task_struct *next)
{
	if (next == current) return;
	else
	{
		struct task_struct *current_addr = current;
			//(struct task_struct *)&(current->thread.ra);
		struct task_struct *next_addr = next;
			//(struct task_struct *)&(next->thread.ra);
		current = next;
		printk("Switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",current -> pid, current -> priority, current -> counter);
		__switch_to(current_addr, next);
	}
	return;
}

void do_timer()
{
	if (current -> pid == 0 || current -> counter == 0) schedule();
	else if (current -> pid != 0 && current -> counter > 0) 
	{
		current -> counter --;
		return;
	}
	return;
}

/*
* @mm          : current thread's mm_struct
* @address     : the va to look up
*
* @return      : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr) {
	struct vm_area_struct* pre_vma = mm->mmap;
	while (pre_vma) {
		if (addr >= pre_vma->vm_start && addr < pre_vma->vm_end) break;
		pre_vma = pre_vma->vm_next;
	}
	return pre_vma;
}

int create_link(struct mm_struct *mm, uint64 addr, uint64 length, int prot) {
	struct vm_area_struct* vm = kalloc();
	struct vm_area_struct* pre_vma = mm->mmap;
	struct vm_area_struct* prev_vma;
	while (pre_vma && pre_vma->vm_start < addr + length) {
		prev_vma = pre_vma;
		pre_vma = pre_vma->vm_next;
	}
	// Classify different situations
	if (pre_vma == mm->mmap) {
		vm->vm_start = addr;
		vm->vm_end = addr + length;
		vm->vm_prev = NULL;
		vm->vm_next = mm->mmap;
		vm->vm_mm = mm;
		vm->vm_flags = (uint64)prot;
		if (mm->mmap)
			mm->mmap->vm_prev = vm;
		mm->mmap = vm;
	} 
	else if (pre_vma && pre_vma != mm->mmap && addr >= pre_vma->vm_prev->vm_end) {
		vm->vm_start = addr;
		vm->vm_end = addr + length;
		vm->vm_prev = pre_vma->vm_prev;
		vm->vm_next = pre_vma;
		pre_vma->vm_prev->vm_next = vm;
		pre_vma->vm_prev = vm;
		vm->vm_mm = mm;
		vm->vm_flags = (uint64)prot;
	} 
	else if (pre_vma == NULL && addr >= prev_vma->vm_end) {
		prev_vma->vm_next = vm;
		vm->vm_start = addr;
		vm->vm_end = addr + length;
		vm->vm_prev = prev_vma;
		vm->vm_next = NULL;
		vm->vm_mm = mm;
		vm->vm_flags = (uint64)prot;
	} else return 0;
	return 1; 
}

int is_valid(struct mm_struct *mm, uint64 addr, uint64 length) {
	struct vm_area_struct *pre_vma = mm->mmap;
	while (pre_vma) {
		if (addr >= pre_vma->vm_start && addr < pre_vma->vm_end) return 1;
			else if ((addr + length) > pre_vma->vm_start && (addr + length) <= pre_vma->vm_end) return 1;
		pre_vma = pre_vma->vm_next;
	}
	return 0;
}

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length) {
	uint64 valid_vma = 0;
	// for (valid_vma = 0; valid_vma <= 0xC0000000 && is_valid(mm,valid_vma,length); valid_vma += PGSIZE);
	while (valid_vma <= 0xC0000000 && is_valid(valid_vma,mm,length)) {
		valid_vma += PGSIZE;
	}
	return valid_vma;

}

/*
 * @mm     : current thread's mm_struct
 * @addr   : the suggested va to map
 * @length : memory size to map
 * @prot   : protection
 *
 * @return : start va
*/
uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot) {
	while (!create_link(mm, addr, length, prot)) {
		addr = get_unmapped_area(mm,length);
	}
	return addr;
}


#ifdef SJF
int Get_task_id() {
    uint64 i, index=-1;
    for(i = 1; i < cnt; ++i) {
        if((task[i]->counter) && (task[i]->state == TASK_RUNNING)) {
            if((index == -1)|| (task[i]->counter < task[index]->counter))
                index = i;
        }
    }
    return index;
}
#endif

#ifdef PRIORITY
int Get_task_id() {
    uint64 i, index=-1;
    for(i = 1; i < cnt; ++i) {
        if((task[i]->counter) && (task[i]->state == TASK_RUNNING)) {
            if((index == -1) || (task[i]->priority > task[index]->priority))
                index = i;
        }
    }
   return index;
}
#endif

void schedule(void){
    uint64 index;
    uint64 tmp = 0;
	int i;
    while((index = Get_task_id()) == -1) {
        for(i = 1; i < cnt; ++i){
            if(task[i]->state == TASK_RUNNING) {
                task[i]->counter = (uint64)rand();
            }
       }
        tmp++;
    }
    if(tmp) {
        for(i = 1; i < cnt; ++i) {
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n",task[i]->pid,task[i]->priority,task[i]->counter);
        }
    }
    switch_to(task[index]);
    return;
}
