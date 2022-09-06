#include"defs.h"
#include"mm.h"
#include"printk.h"
#include"types.h"

#include<stddef.h>
#include<string.h>


extern unsigned long _stext;
extern unsigned long _srodata;
extern unsigned long _sdata;
extern unsigned long _sbss;

unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
	/* 
    	1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    	2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit | 
	high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
	低 30 bit 作为 页内偏移 
	这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    	3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    	*/
	early_pgtbl[2] = ((unsigned long) (0x1 << 29)) | 0x000000000000000f;
	early_pgtbl[384] = ((unsigned long) (0x1 << 29)) | 0x000000000000000f;
}

unsigned long *get_PTE_addr(unsigned long *root, unsigned long va) {
	unsigned long *cur_pg_addr = root;
	unsigned long *cur_addr;
	int level = 2;
	for (level; level > 0; --level) {
		if (level == 2) {
			cur_addr = &cur_pg_addr[(va >> 30) & 511];
		} else if (level == 1) {
			cur_addr = &cur_pg_addr[(va >> 21) & 511];
		}
		if ((*cur_addr) & 0x1) { // if PTE is valid
			cur_pg_addr = (unsigned long*)((((*cur_addr) >> 10) << 12) + PA2VA_OFFSET);
		} else { // allocate a free page
			if ((cur_pg_addr = (uint64*)kalloc()) == NULL) {
				printk("There is no free space!\n");
				return NULL;
			} 
			memset(cur_pg_addr, 0, PGSIZE);
			unsigned long PPN = ((unsigned long)cur_pg_addr - PA2VA_OFFSET) >> 12;
			unsigned long perm = 0;
			unsigned long V = 1;
			*cur_addr = ((unsigned long)(*cur_addr) & 0xffc0000000000000) | ((unsigned long)PPN << 10) |
				((unsigned long)perm | (unsigned long)V);
		}
	}
	return &cur_pg_addr[(va >> 12) & 511];
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
void setup_vm_final(void) {
	printk("setup_vm_final start\n");
	memset(swapper_pg_dir, 0x0, PGSIZE);
	unsigned long *pgtbl = swapper_pg_dir;
	// No OpenSBI mapping required
	unsigned long va = VM_START + OPENSBI_SIZE; // Jump over OpenSBI block 
	unsigned long pa = PHY_START + OPENSBI_SIZE; // Jump over OpenSBI block
	
	// mapping kernel text X|-|R|V
	unsigned long text_sz = ((unsigned long)&_srodata) - ((unsigned long)&_stext);
	create_mapping(pgtbl, va, pa, text_sz, 11); // privilege = 0b1011 = 11
	va += text_sz; // update va
	pa += text_sz; // update pa
	printk(".text va = 0x%lx, pa = 0x%lx\n",va,pa);
	// mapping kernel rodata -|-|R|V
	unsigned long rodata_sz = ((unsigned long)&_sdata) - ((unsigned long)&_srodata);
	create_mapping(pgtbl, va, pa, rodata_sz, 3); // privilege = 0b0011 = 3 
	va += rodata_sz;
	pa += rodata_sz;
	printk(".rodata va = 0x%lx, pa = 0x%lx\n",va,pa);
	// mapping other memory -|W|R|V
	unsigned long extra_sz = PHY_SIZE - text_sz - rodata_sz - OPENSBI_SIZE; // other memory size
	create_mapping(pgtbl, va, pa, extra_sz, 7); // privilege = 0b0111 = 7
	// set satp with swapper_pg_dir
	unsigned long tmp = ((unsigned long)pgtbl) - PA2VA_OFFSET;
	unsigned long PPN = ((unsigned long)tmp) >> 12;
	unsigned long satp_write = (0x00000fffffffffff & PPN) | 0x8000000000000000;
	csr_write(satp,satp_write);	
	// flush TLB
 
	asm volatile("sfence.vma zero, zero");
	printk("setup_vm_final end\n");
	return;
}

void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
	/*
	pgtbl 为根页表的基地址     
 	va, pa 为需要映射的虚拟地址、物理地址     
	sz 为映射的大小     
	perm 为映射的读写权限      
	创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录     
	可以使用 V bit 来判断页表项是否存在     
	*/	
	unsigned long cur_va = va;
	unsigned long cur_pa = pa;
	for (cur_va; cur_va < va + sz; cur_va += PGSIZE, cur_pa += PGSIZE) {
		unsigned long *PTE_addr = get_PTE_addr(pgtbl, cur_va);
		unsigned long PPN = ((unsigned long)cur_pa) >> 12;
		unsigned long V = 1;
		// write PTE
		*PTE_addr = ((unsigned long)*(PTE_addr) & 0xffc0000000000000) | ((unsigned long)PPN << 10) |
			((unsigned long)perm | (unsigned long)V); 
	}
	return;
}
