#include "printk.h"
#include "../include/syscall.h"
#include "../include/proc.h"

extern void clock_set_next_event();
extern void do_timer();
extern void do_page_fault(unsigned long scause, unsigned long sepc,struct pt_regs *regs);
extern uint64 do_fork(struct pt_regs *regs);
extern uint64 clone(struct pt_regs *regs);

/*
	sstatus		<- High address
	sepc
	x31
	x30
	...
	x1
	x0 			<- sp (Low address)
*/

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs)
{
	// unsigned long sie;
	// __asm__ volatile(
	// 	"ld s2, 8(sp)\n"
	// 	"ld s3, 0(sp)\n"
	// 	"csrrs s4,sie,x0\n"
	// 	"mv %[scause], s2\n"
	// 	"mv %[sepc], s3\n"
	// 	"mv %[sie], s4\n"
	// 	: [scause] "=r" (scause),[sepc] "=r" (sepc),[sie] "=r" (sie)
	// 	: :"memory"
	// 		);
	// If it's interrupt
	if (scause == 0x8000000000000005)
	{
		printk("[S] Supervisor Mode Timer Interrput\n");
		clock_set_next_event();
		do_timer();
	} else {
		unsigned long a0 = regs->x[10];
		unsigned long a1 = regs->x[11];
		unsigned long a2 = regs->x[12];
		unsigned long a7 = regs->x[17];
		if (scause == 8) { // system call
			if (a7 == SYS_WRITE) {
				sys_write(a0, a1, a2);
			} else if (a7 == SYS_GETPID) {
				regs->x[10] = sys_getpid();
			} else if (a7 == SYS_CLONE) {
				regs->x[10] = clone(regs);
			}
			regs->sepc = (unsigned long)((unsigned long)regs->sepc + (unsigned long)0x4);
		} else if (scause == 12 || scause == 13 || scause == 15) do_page_fault(scause,sepc,regs);
	}
	return;
}
