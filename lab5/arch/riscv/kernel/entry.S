.extern trap_handler
.extern dummy
.extern do_timer
	.section .text.entry
	.align 2
	.global _traps
	.global __switch_to
	.global __dummy
	.global ret_from_fork
_traps:
	csrr t2, sscratch
	beqz t2, _traps_branch1
	csrrw sp, sscratch, sp 
_traps_branch1:
	# Save registers and spec to stack 
	addi sp,sp,-272
	csrr t1, sstatus
	sd t1, 264(sp)
	csrr t1, sepc
	sd t1, 256(sp)

	sd x31, 248(sp)
	sd x30, 240(sp)
	sd x29, 232(sp)
	sd x28, 224(sp)
	sd x27, 216(sp)
	sd x26, 208(sp)
	sd x25, 200(sp)
	sd x24, 192(sp)
	sd x23, 184(sp)
	sd x22, 176(sp)
	sd x21, 168(sp)
	sd x20, 160(sp)
	sd x19, 152(sp)
	sd x18, 144(sp)
	sd x17, 136(sp)
	sd x16, 128(sp)
	sd x15, 120(sp)
	sd x14, 112(sp)
	sd x13, 104(sp)
	sd x12, 96(sp)
	sd x11, 88(sp)
	sd x10, 80(sp)
	sd x9, 72(sp)
	sd x8, 64(sp)
	sd x7, 56(sp)
	sd x6, 48(sp)
	sd x5, 40(sp)
	sd x4, 32(sp)
	sd x3, 24(sp)
	sd x2, 16(sp)
	sd x1, 8(sp)
	
	# call trap_handler	
	csrr a0, scause # save registers as parameters
	csrr a1, sepc
	mv a2, sp
	call trap_handler	
	# Restore
	
	ld t1, 264(sp)
	csrw sstatus, t1
	ld t1, 256(sp)
	csrw sepc, t1
	
	ld x31, 248(sp)
	ld x30, 240(sp)
	ld x29, 232(sp)
	ld x28, 224(sp)
	ld x27, 216(sp)
	ld x26, 208(sp)
	ld x25, 200(sp)
	ld x24, 192(sp)
	ld x23, 184(sp)
	ld x22, 176(sp)
	ld x21, 168(sp)
	ld x20, 160(sp)
	ld x19, 152(sp)
	ld x18, 144(sp)
	ld x17, 136(sp)
	ld x16, 128(sp)
	ld x15, 120(sp)
	ld x14, 112(sp)
	ld x13, 104(sp)
	ld x12, 96(sp)
	ld x11, 88(sp)
	ld x10, 80(sp)
	ld x9, 72(sp)
	ld x8, 64(sp)
	ld x7, 56(sp)
	ld x6, 48(sp)
	ld x5, 40(sp)
	ld x4, 32(sp)
	ld x3, 24(sp)
	ld x2, 16(sp)
	ld x1, 8(sp)
	addi sp, sp, 272
	
	csrr t1, sscratch
	beqz t1, _traps_branch2
	csrrw sp, sscratch, sp
_traps_branch2:
	sret
	
__switch_to:
	# store register
	sd ra, 40(a0)
    sd sp, 48(a0)
    sd s0, 56(a0)
    sd s1, 64(a0)
    sd s2, 72(a0)
    sd s3, 80(a0)
    sd s4, 88(a0)
    sd s5, 96(a0)
    sd s6, 104(a0)
    sd s7, 112(a0)
    sd s8, 120(a0)
    sd s9, 128(a0)
    sd s10, 136(a0)
    sd s11, 144(a0)
	# store sepc, sstatus, sscratch
	csrr t0, sepc
    sd t0,152(a0)
    csrr t0, sstatus
    sd t0, 160(a0)
    csrr t0, sscratch
    sd t0, 168(a0)
	# change page table

	ld t0, 176(a1)
    srli t0, t0, 12
    li t1, (8 << 60)
    or t2, t0, t1
    csrw satp, t2
    sfence.vma zero, zero	

	# load register
	ld ra, 40(a1)
    ld sp, 48(a1)
    ld s0, 56(a1)
    ld s1, 64(a1)
    ld s2, 72(a1)
    ld s3, 80(a1)
    ld s4, 88(a1)
    ld s5, 96(a1)
    ld s6, 104(a1)
    ld s7, 112(a1)
    ld s8, 120(a1)
    ld s9, 128(a1)
    ld s10, 136(a1)
    ld s11, 144(a1)
	# load sepc, sstatus, sscratch
	ld t0, 152(a1)
    csrw sepc, t0
    ld t0, 160(a1)
    csrw sstatus, t0
    ld t0, 168(a1)
    csrw sscratch, t0
	ret

__dummy:
	csrrw sp, sscratch, sp
    la t0, 0
    csrw sepc, t0
	sret

ret_from_fork:
	ld t0, 264(a0)
	csrw sstatus, t0
	ld t0, 256(a0)
	addi t0, t0, 4
	csrw sepc, t0
	ld x1, 8(a0)
	ld x2, 16(a0)
	ld x3, 24(a0)
	ld x4, 32(a0)
	ld x5, 40(a0)
	ld x6, 48(a0)
	ld x7, 56(a0)
	ld x8, 64(a0)
	ld x9, 72(a0)
	ld x11, 88(a0)
	ld x12, 96(a0)
	ld x13, 104(a0)
	ld x14, 112(a0)
	ld x15, 120(a0)
	ld x16, 128(a0)
	ld x17, 136(a0)
	ld x18, 144(a0)
	ld x19, 152(a0)
	ld x20, 160(a0)
	ld x21, 168(a0)
	ld x22, 176(a0)
	ld x23, 184(a0)
	ld x24, 192(a0)
	ld x25, 200(a0)
	ld x26, 208(a0)
	ld x27, 216(a0)
	ld x28, 224(a0)
	ld x29, 232(a0)
	ld x30, 240(a0)
	ld x31, 248(a0)
	ld x10, 80(a0)
	sret