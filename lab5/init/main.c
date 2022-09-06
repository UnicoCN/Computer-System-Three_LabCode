#include "printk.h"
#include "sbi.h"

extern void test();
extern void schedule();

int start_kernel() {
    // puti(2021);
    // puts(" Hello RISC-V\n");
    // printk("2022");
    printk("[S-MODE] Hello RISC-V\n");
    schedule();
    test(); // DO NOT DELETE !!!

	return 0;
}
