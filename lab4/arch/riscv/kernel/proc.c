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
struct task_struct *task[NR_TASKS];

void task_init()
{
	printk("...proc_init begin!\n");
	idle = (struct task_struct *)kalloc();
	idle -> state = TASK_RUNNING;
	idle -> counter = 0;
	idle -> priority = 0;
	idle -> pid = 0;
	idle -> thread.sp = PGSIZE + (uint64)idle; // sp -> High Address 
	task[0] = idle;
	current = idle;

	int i;
	for (i = 1; i < NR_TASKS; i++)
	{
		task[i] = (struct task_struct*)kalloc();
		task[i] -> state = TASK_RUNNING;
		task[i] -> counter = rand();
		task[i] -> priority = rand();
		task[i] -> pid = i;
		task[i] -> thread.sp = PGSIZE + (uint64)task[i];
		task[i] -> thread.ra = (uint64)(__dummy);
		printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n",i,task[i]->priority,task[i]->counter);
		// kalloc space for user stack
		uint64 *user_stack = (uint64*)kalloc();
		// create a root page table for each user process
		uint64 *root_pg_tbl = (uint64*)kalloc();
		// set user process page table address
		task[i]->pgd = (uint64)root_pg_tbl - PA2VA_OFFSET;
		// copy the kernel page table to user page table
		for (int i = 0; i < 512; ++i)
			root_pg_tbl[i] = swapper_pg_dir[i];
		//create mapping for each user process
		create_mapping(root_pg_tbl, USER_END - PGSIZE, (uint64)user_stack - PA2VA_OFFSET, PGSIZE, 0b10111);
		create_mapping(root_pg_tbl, USER_START, (uint64)uapp_start - PA2VA_OFFSET, (uint64)uapp_end - (uint64)uapp_start, 0b11111);
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
#ifdef SJF
void schedule()
{
	int i,p;
	for (i = 1; i < NR_TASKS; i++)
		if (task[i] -> counter != 0) break;
	if (i == NR_TASKS)
	{
		task_init();
		return;
	}
	p = -1;
	for (i = 1; i < NR_TASKS; i++)
		if (p == -1 && task[i] -> counter != 0) p = i;
			else if (task[i] -> counter != 0 && task[i] -> counter < task[p] -> counter) p = i;
	
	switch_to(task[p]);
	return;
}
#endif

#ifdef PRIORITY
void schedule()
{
	int i,p;
	for (i = 1; i < NR_TASKS; i++)
		if (task[i] -> counter != 0) break;
	if (i == NR_TASKS)
	{
		task_init();
		return;
	}
	p = -1;
	for (i = 1; i < NR_TASKS; i++)
		if (p == -1 && task[i] -> counter != 0) p = i;
			else if (task[i] -> counter != 0 && task[i] -> priority > task[p] -> priority) p = i;
	switch_to(task[p]);
	return;
}
#endif
