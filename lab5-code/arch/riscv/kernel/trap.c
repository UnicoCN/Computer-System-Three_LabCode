// trap.c

#include "printk.h"
#include "sbi.h"
#include "proc.h"
#include "syscall.h"
extern void clock_set_next_event();
extern void do_page_fault(unsigned long scause, unsigned long sepc,struct pt_regs *regs);
extern uint64 clone(struct pt_regs *regs);
void trap_handler(unsigned long scause, unsigned long sepc,struct pt_regs *regs) {
    unsigned long x=scause,y=sepc,xi,yi;
    xi=0x8000000000000000;
    if(x>=xi){
        xi=0x8000000000000005;
        if(x==xi){
            //a supervisor time interrupt       
            //printk("kernel is running!\n");
            printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
       }
    }
    else {
        unsigned long a7=regs->x17;
        //printk("sysnumber=%lx scause=%lx\n",a7,x);
        if(x==8){
            if(a7==SYS_WRITE){
                sys_write(regs->x10,regs->x11,regs->x12);
            }
            else if(a7==SYS_GETPID){
                regs->x10=sys_getpid();//store return value in a0
            }
	    else if(a7==SYS_CLONE){
		regs->x10=clone(regs);
	    }
            regs->sepc =(unsigned long)(((unsigned long)regs->sepc) + (unsigned long)0x4);
        }
	else if(x==0xC||x==0xD||x==0xF)
		do_page_fault(scause,sepc,regs);
    }
    return;
}


