    .section .text.entry
    .align 2
    .globl _traps
    .globl __dummy
    .globl __switch_to
    .globl ret_from_fork
    .extern trap_handler
    .extern dummy
    .extern clock_set_next_event
ret_from_fork:
    ld t0, 0(a0)
    csrw sstatus, t0
    ld t0, 8(a0)
    addi t0,t0,4 # sepc = ecall+4
    csrw sepc, t0
    ld x31, 16(a0)
    ld x30, 24(a0)
    ld x29, 32(a0)
    ld x28, 40(a0)
    ld x27, 48(a0)
    ld x26, 56(a0)
    ld x25, 64(a0)
    ld x24, 72(a0)
    ld x23, 80(a0)
    ld x22, 88(a0)
    ld x21, 96(a0)
    ld x20, 104(a0)
    ld x19, 112(a0)
    ld x18, 120(a0)
    ld x17, 128(a0)
    ld x16, 136(a0)
    ld x15, 144(a0)
    ld x14, 152(a0)
    ld x13, 160(a0)
    ld x12, 168(a0)
    ld x11, 176(a0)
    ld x9, 192(a0)
    ld x8, 200(a0)
    ld x7, 208(a0)
    ld x6, 216(a0)
    ld x5, 224(a0)
    ld x4, 232(a0)
    ld x3, 240(a0)
    ld x2, 248(a0)
    ld x1, 256(a0)
    ld x10, 184(a0)
    sret
_traps:
    # YOUR CODE HERE
    # -----------
     # judge user or kernel
     csrr t0,sscratch
     beqz t0,is_kernel
     csrrw sp,sscratch,sp
is_kernel:
        # 1. save 32 registers and sepc to stack
     sd zero,-8(sp)
     sd ra, -16(sp)
     sd sp, -24(sp)
     sd gp, -32(sp)
     sd tp, -40(sp)
     sd t0, -48(sp)
     sd t1, -56(sp)
     sd t2, -64(sp)
     sd s0, -72(sp)
     sd s1, -80(sp)
     sd a0, -88(sp)
     sd a1, -96(sp)
     sd a2, -104(sp)
     sd a3, -112(sp)
     sd a4, -120(sp)
     sd a5, -128(sp)
     sd a6, -136(sp)
     sd a7, -144(sp)
     sd s2, -152(sp)
     sd s3, -160(sp)
     sd s4, -168(sp)
     sd s5, -176(sp)
     sd s6, -184(sp)
     sd s7, -192(sp)
     sd s8, -200(sp)
     sd s9, -208(sp)
     sd s10, -216(sp)
     sd s11, -224(sp)
     sd t3, -232(sp)
     sd t4, -240(sp)
     sd t5, -248(sp)
     sd t6, -256(sp)
     csrr t0,sepc
     sd t0,-264(sp)
     csrr t0,sstatus
     sd t0,-272(sp)
     addi sp,sp,-272

    # -----------

        # 2. call trap_handler
     csrr a0,scause
     csrr a1,sepc
     addi a2,sp,0 # place to store sepc and then the reversed order of 32 registers
     call trap_handler
        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
     ld t0,0(sp)
     csrw sstatus,t0
     ld t0,8(sp)
     csrw sepc,t0
     ld t6,16(sp)
     ld t5,24(sp)
     ld t4,32(sp)
     ld t3,40(sp)
     ld s11,48(sp)
     ld s10,56(sp)
     ld s9,64(sp)
     ld s8,72(sp)
     ld s7,80(sp)
     ld s6,88(sp)
     ld s5,96(sp)
     ld s4,104(sp)
     ld s3,112(sp)
     ld s2,120(sp)
     ld a7,128(sp)
     ld a6,136(sp)
     ld a5,144(sp)
     ld a4,152(sp)
     ld a3,160(sp)
     ld a2,168(sp)
     ld a1,176(sp)
     ld a0,184(sp)
     ld s1,192(sp)
     ld s0,200(sp)
     ld t2,208(sp)
     ld t1,216(sp)
     ld t0,224(sp)
     ld tp,232(sp)
     ld gp,240(sp)
     ld ra,256(sp)
     ld sp,248(sp)

     csrr t0,sscratch
     beqz t0,final #kernel
     csrrw sp,sscratch,sp

    # -----------
final:
        # 4. return from trap
     sret
    # -----------
__dummy:
    csrrw sp,sscratch,sp  #S-sp->sscratch  sscratch(U-sp)->sp
    la t0,0
    csrw sepc,t0
    sret
__switch_to:
    #unable the time interrupt
    #csrr t1,sie
    #andi t1,t1,0xFFFFFFFFFFFFFFDF
    #csrw sie,t1
    #save state to prev process
    addi t0,a0,40
    sd ra,0(t0)
    sd sp,8(t0)
    sd s0,16(t0)
    sd s1,24(t0)
    sd s2,32(t0)
    sd s3,40(t0)
    sd s4,48(t0)
    sd s5,56(t0)
    sd s6,64(t0)
    sd s7,72(t0)
    sd s8,80(t0)
    sd s9,88(t0)
    sd s10,96(t0)
    sd s11,104(t0)

    # save csrs
    csrr t1,sepc
    sd t1,112(t0)
    csrr t1,sstatus
    sd t1,120(t0)
    csrr t1,sscratch
    sd t1,128(t0)

    # change page table
    addi t0,a1,40
    ld t1,136(t0)  #PA
    srli t1,t1,12
    li t2,(8<<60) #set the mode SV39
    or t1,t1,t2
    csrw satp,t1
    sfence.vma zero,zero #open the vitual mapping

    # restore state from next process   

    ld ra,0(t0)
    ld sp,8(t0)
    ld s0,16(t0)
    ld s1,24(t0)
    ld s2,32(t0)
    ld s3,40(t0)
    ld s4,48(t0)
    ld s5,56(t0)
    ld s6,64(t0)
    ld s7,72(t0)
    ld s8,80(t0)
    ld s9,88(t0)
    ld s10,96(t0)
    ld s11,104(t0)

    # restore csrs
    ld t1,112(t0)
    csrw sepc,t1
    ld t1,120(t0)
    csrw sstatus,t1
    ld t1,128(t0)
    csrw sscratch,t1
    ret

