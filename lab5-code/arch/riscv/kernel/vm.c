// arch/riscv/kernel/vm.c
#include "defs.h"
#include "mm.h"
#include "string.h"
#include "printk.h"

extern unsigned long  _stext;
extern unsigned long _srodata;
extern unsigned long _sdata;
extern unsigned long _sbss;

unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    //VA 0x80000000   index=10
    early_pgtbl[2]=(unsigned long)(((0x80000000>>12)<<10)|0x000000000000000F);

    //VA 0xffffffe00000000  e-1110 index=1 10000000=0x180
    early_pgtbl[0x180]=(unsigned long)(((0x80000000>>12)<<10)|0x000000000000000F);

}

void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm){
        unsigned long i;
        unsigned long *first,*second,*third,*final;
	if(pgtbl!=swapper_pg_dir) ;
	    //printk("into create_mapping...  pgtbl=0x%lx va=0x%lx  pa=0x%lx  sz=0x%lx \n",pgtbl,va,pa,sz);
        for(i=0;i<sz;i+=PGSIZE){
            //first level
            first=&pgtbl[((va+i)>>30)&0x1FF];//page fault occurs when visiting pgtbl of certain process
            //if first level page is valid
            if((*first)&0x1)
                second=(unsigned long*)((unsigned long)(((*first)>>10)<<12)+PA2VA_OFFSET);
           //if frist level page is not valid, allocate one page for it and fill the page table entry
            else{
                second=(uint64*)kalloc();
                memset(second,0,PGSIZE);
                *first=(unsigned long)((*first)&0xffc0000000000000)|(((((unsigned long)second-PA2VA_OFFSET)>>12)<<10)|(unsigned long)0x1);
            }
	    //if(pgtbl!=swapper_pg_dir)
            //printk("first level finished...\n");
            //second level
            second=&second[((va+i)>>21)&0x1FF];
            //if second level page is valid
            if((*second)&0x1)
                third=(unsigned long*)((unsigned long)(((*second)>>10)<<12)+PA2VA_OFFSET);
            else{
                third=(uint64*)kalloc();
                memset(third,0,PGSIZE);
                *second=(unsigned long)((*second)&0xffc0000000000000)|(((((unsigned long)third-PA2VA_OFFSET)>>12)<<10)|(unsigned long)0x1);
            }
	    //if(pgtbl!=swapper_pg_dir)
            //printk("second level finished...\n");
            //third level fill it with corresponding pa
            third=&third[((va+i)>>12)&0x1FF];
            *third=(unsigned long)(((*third)&0xffc0000000000000)|((((unsigned long)(pa+i)>>12)<<10)|0x1|(unsigned long)perm));
            //if(pgtbl!=swapper_pg_dir)
	    //printk("third level finished...\n");
	}
	printk("third level pa: %lx\n",*third);
        return;
}
void setup_vm_final(void) {
    unsigned long* tmp=swapper_pg_dir;
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required
    unsigned long va=VM_START+OPENSBI_SIZE;
    unsigned long pa=PHY_START+OPENSBI_SIZE;

    // mapping kernel text X|-|R|V
    unsigned long text_sz=(unsigned long)&_srodata-(unsigned long)&_stext;
    create_mapping(tmp,va,pa,text_sz,11);
    printk("text section: %lx~%lx\n",va,va+text_sz);
    // mapping kernel rodata -|-|R|V
    va=va+text_sz;
    pa=pa+text_sz;
    unsigned long rodata_sz=(unsigned long)&_sdata-(unsigned long)&_srodata;
    create_mapping(tmp,va,pa,rodata_sz,3);
    printk("rodata section: %lx~%lx\n",va,va+rodata_sz);

    // mapping other memory -|W|R|V
    va=va+rodata_sz;
    pa=pa+rodata_sz;
    unsigned long data_sz=PHY_SIZE-text_sz-rodata_sz-OPENSBI_SIZE;
    create_mapping(tmp,va,pa,data_sz,7);

    // set satp with swapper_pg_dir
    unsigned long val=(0x000fffffffffffff&(((unsigned long)swapper_pg_dir-PA2VA_OFFSET)>>12))|(0x8000000000000000);
    csr_write(satp,val);

    // flush TLB
    asm volatile("sfence.vma zero, zero");
    return;
}
