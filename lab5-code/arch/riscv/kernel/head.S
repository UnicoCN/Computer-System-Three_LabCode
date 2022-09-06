.extern start_kernel
.extern _traps
.extern mm_init
.extern task_init
.extern clock_set_next_event
.extern setup_vm
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
     
    #la sp,boot_stack_top
    #call mm_init
    #call task_init

    call start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    li t1,0x0000000080000000  # PHY_START
    li t2,0xffffffe000000000  # VM_START
    sub t1,t2,t1 # VM_SIZE
    add ra,ra,t1
  
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    add sp,sp,t1

    # set satp with early_pgtbl
    la t1,early_pgtbl
    srli t1,t1,12
    li t2,0x8000000000000000
    or t1,t1,t2
    csrw satp,t1

    # flush tlb
    sfence.vma zero, zero

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 0x5000 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
