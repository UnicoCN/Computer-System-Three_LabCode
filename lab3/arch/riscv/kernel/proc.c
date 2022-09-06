#include"proc.h"
#include"printk.h"
#include"mm.h"
#include"rand.h"
#include"defs.h"

extern void __dummy();
extern void __stwich_to(struct task_struct *prev, struct task_struct *next);
void do_timer();
void schedule();
struct task_struct *idle;
struct task_struct *current;
struct task_struct *task[NR_TASKS];

void task_init()
{
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
		__asm volatile(
			"mv t1, %[current]\n"
			"mv t2, %[next]\n"
			"addi sp, sp, -16\n"
			"sd t1, 8(sp)\n"
			"sd t2, 0(sp)\n"
			"call __switch_to\n"
			: : [current] "r" (current_addr), [next] "r" (next_addr)
			: "memory"
			      );
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
