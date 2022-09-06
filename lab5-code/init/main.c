#include "printk.h"
#include "sbi.h"
#include "proc.h"

extern void test();

int start_kernel() {

    printk("[S-MODE] Hello RISC-V\n");
    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
