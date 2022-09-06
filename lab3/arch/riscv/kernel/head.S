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
    # ------------------
    # - your code here -
    # ------------------
    # Set stvec to &_traps
    
    la sp, boot_stack_top
    
    call setup_vm
    call relocate
    call mm_init
    call setup_vm_final

    call task_init
    la s2, _traps
    csrrw x0,stvec,s2 
    # Set stvec[5] to 1
    addi s2, x0, 0x20
    csrrs x0,sie,s2
    # set initial time interrupt
    rdtime s2 
    li s3, 10000000
    add s2,s2,s3
    addi a7,x0,0x0
    addi a6,x0,0x0
    add a0,x0,s2
    addi a1,x0,0x0
    addi a2,x0,0x0
    addi a3,x0,0x0
    addi a4,x0,0x0
    addi a5,x0,0x0
    ecall
    # Set sstatus[1] to 1
    addi s2,x0,0x2
    csrrs x0,sstatus,s2
    # call setup_vm
    # call relocate
    # call mm_init
    # call setup_vm_final
    # call task_init
    j start_kernel

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
