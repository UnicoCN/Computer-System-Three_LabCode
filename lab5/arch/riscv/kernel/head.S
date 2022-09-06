.extern start_kernel
.extern _traps
.extern mm_init
.extern task_init
.extern setup_vm
.extern setup_vm_final
.extern early_pgtbl

.section .text.init
    .globl _start
_start:
   la sp,boot_stack_top
   
    call setup_vm
    call relocate

    call mm_init
    call setup_vm_final

    call task_init
    
    #set stvec to _traps
    la t0,_traps
    #add t0,t0,t0
    #add t0,t0,t0
    csrw stvec,t0

    #set sie[STIE]=1
    csrr t1,sie
    addi t2,x0,32
    or t1,t1,t2
    csrw sie,t1
    
    #set first time interrupt
    
    #call clock_set_next_event
    rdtime t0
    li t1,10000000
    add a0,t1,t0
    add a1,x0,x0
    add a2,x0,x0
    add a3,x0,x0
    add a4,x0,x0
    add a5,x0,x0
    add a7,x0,x0
    add a6,x0,x0
    ecall
   
    #set sstatus[sie]=1
    #csrr t1,sstatus
    #addi t2,x0,2
    #or t1,t1,t2
    #csrw sstatus,t1

    call start_kernel

relocate:
    li s2, 0xffffffe000000000 # s2 = vm_start_addr
    li s3, 0x0000000080000000 # s3 = phy_start_addr
    sub s4, s2, s3	# s4 = s2 - s3 = pa2va_offset
    add ra, ra, s4	# ra = ra + offset
    add sp, sp, s4	# sp = sp + offset
    # set satp
    la s2, early_pgtbl
    srli s2, s2, 12 # PPN = PA >> 12
    li s3, 0x8000000000000000 # set MODE as Sv39
    or s2, s3, s2
    csrw satp, s2 # csr write
    # flush tlb
    sfence.vma zero, zero
    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 0x1000 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
