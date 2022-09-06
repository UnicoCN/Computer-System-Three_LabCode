// clock.c
#include "sbi.h"

unsigned long TIMECLOCK = 10000000;
unsigned long get_cycles() {
   unsigned long ret_time;
   __asm__ volatile(
	   "rdtime t0\n"
	   "mv %[ret_time],t0"
	   :[ret_time]"=r"(ret_time)
	   :
	   );	   
   return ret_time;
}

void clock_set_next_event() {
    unsigned long next = get_cycles() + TIMECLOCK;
    sbi_ecall(0x00,0,next,0,0,0,0,0);
    return;
} 
