#include "printk.h"
extern void clock_set_next_event();
extern void do_timer();
void trap_handler(unsigned long scause, unsigned long sepc)
{
	unsigned long sie;
	__asm__ volatile(
		"ld s2, 8(sp)\n"
		"ld s3, 0(sp)\n"
		"csrrs s4,sie,x0\n"
		"mv %[scause], s2\n"
		"mv %[sepc], s3\n"
		"mv %[sie], s4\n"
		: [scause] "=r" (scause),[sepc] "=r" (sepc),[sie] "=r" (sie)
		: :"memory"
			);
	// If it's interrupt
	if (scause >> 31 != 0)
	{
		if ((sie >> 5) % 2 ==1)
		{
			//printk("[S] Supervisor Mode Timer Interrput\n");
			clock_set_next_event();
			do_timer();
		}
		//printk("The kernel is running!\n");
	}
	//printk("[S] Supervisor Mode Timer Interrupt\n");
	return;
}
