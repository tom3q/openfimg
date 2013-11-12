/*
 *  arch/arm/include/asm/assembler.h
 *
 *  Copyright (C) 1996-2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains arm architecture specific defines
 *  for the different processors.
 *
 *  Do not include any C declarations in this file - it is included by
 *  assembler source.
 */
#ifndef __ASM_ASSEMBLER_H__
#define __ASM_ASSEMBLER_H__

#define IOMEM(x)	(x)

#define __LINUX_ARM_ARCH__	6
#define __ASSEMBLY__

#if defined(__ASSEMBLY__) && defined(CONFIG_ARM_ASM_UNIFIED)
	.syntax unified
#endif

#ifdef CONFIG_THUMB2_KERNEL

#if __GNUC__ < 4
#error Thumb-2 kernel requires gcc >= 4
#endif

/* The CPSR bit describing the instruction set (Thumb) */
#define PSR_ISETSTATE	PSR_T_BIT

#define ARM(x...)
#define THUMB(x...)	x
#ifdef __ASSEMBLY__
#define W(instr)	instr.w
#define BSYM(sym)	sym + 1
#endif

#else	/* !CONFIG_THUMB2_KERNEL */

/* The CPSR bit describing the instruction set (ARM) */
#define PSR_ISETSTATE	0

#define ARM(x...)	x
#define THUMB(x...)
#ifdef __ASSEMBLY__
#define W(instr)	instr
#define BSYM(sym)	sym
#endif

#endif	/* CONFIG_THUMB2_KERNEL */

#ifndef CONFIG_ARM_ASM_UNIFIED

/*
 * If the unified assembly syntax isn't used (in ARM mode), these
 * macros expand to an empty string
 */
#ifdef __ASSEMBLY__
	.macro	it, cond
	.endm
	.macro	itt, cond
	.endm
	.macro	ite, cond
	.endm
	.macro	ittt, cond
	.endm
	.macro	itte, cond
	.endm
	.macro	itet, cond
	.endm
	.macro	itee, cond
	.endm
	.macro	itttt, cond
	.endm
	.macro	ittte, cond
	.endm
	.macro	ittet, cond
	.endm
	.macro	ittee, cond
	.endm
	.macro	itett, cond
	.endm
	.macro	itete, cond
	.endm
	.macro	iteet, cond
	.endm
	.macro	iteee, cond
	.endm
#else	/* !__ASSEMBLY__ */
__asm__(
"	.macro	it, cond\n"
"	.endm\n"
"	.macro	itt, cond\n"
"	.endm\n"
"	.macro	ite, cond\n"
"	.endm\n"
"	.macro	ittt, cond\n"
"	.endm\n"
"	.macro	itte, cond\n"
"	.endm\n"
"	.macro	itet, cond\n"
"	.endm\n"
"	.macro	itee, cond\n"
"	.endm\n"
"	.macro	itttt, cond\n"
"	.endm\n"
"	.macro	ittte, cond\n"
"	.endm\n"
"	.macro	ittet, cond\n"
"	.endm\n"
"	.macro	ittee, cond\n"
"	.endm\n"
"	.macro	itett, cond\n"
"	.endm\n"
"	.macro	itete, cond\n"
"	.endm\n"
"	.macro	iteet, cond\n"
"	.endm\n"
"	.macro	iteee, cond\n"
"	.endm\n");
#endif	/* __ASSEMBLY__ */

#endif	/* CONFIG_ARM_ASM_UNIFIED */

#define __ALIGN .align 0
#define __ALIGN_STR ".align 0"

#define ENDPROC(name) \
  .type name, %function; \
  END(name)

#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#define asmlinkage CPP_ASMLINKAGE
#endif

#ifndef cond_syscall
#define cond_syscall(x)	asm(				\
	".weak " VMLINUX_SYMBOL_STR(x) "\n\t"		\
	".set  " VMLINUX_SYMBOL_STR(x) ","		\
		 VMLINUX_SYMBOL_STR(sys_ni_syscall))
#endif

#ifndef SYSCALL_ALIAS
#define SYSCALL_ALIAS(alias, name) asm(			\
	".globl " VMLINUX_SYMBOL_STR(alias) "\n\t"	\
	".set   " VMLINUX_SYMBOL_STR(alias) ","		\
		  VMLINUX_SYMBOL_STR(name))
#endif

#define __page_aligned_data	__section(.data..page_aligned) __aligned(PAGE_SIZE)
#define __page_aligned_bss	__section(.bss..page_aligned) __aligned(PAGE_SIZE)

/*
 * For assembly routines.
 *
 * Note when using these that you must specify the appropriate
 * alignment directives yourself
 */
#define __PAGE_ALIGNED_DATA	.section ".data..page_aligned", "aw"
#define __PAGE_ALIGNED_BSS	.section ".bss..page_aligned", "aw"

/*
 * This is used by architectures to keep arguments on the stack
 * untouched by the compiler by keeping them live until the end.
 * The argument stack may be owned by the assembly-language
 * caller, not the callee, and gcc doesn't always understand
 * that.
 *
 * We have the return value, and a maximum of six arguments.
 *
 * This should always be followed by a "return ret" for the
 * protection to work (ie no more work that the compiler might
 * end up needing stack temporaries for).
 */
/* Assembly files may be compiled with -traditional .. */
#ifndef __ASSEMBLY__
#ifndef asmlinkage_protect
# define asmlinkage_protect(n, ret, args...)	do { } while (0)
#endif
#endif

#ifndef __ALIGN
#define __ALIGN		.align 4,0x90
#define __ALIGN_STR	".align 4,0x90"
#endif

#ifdef __ASSEMBLY__

#ifndef LINKER_SCRIPT
#define ALIGN __ALIGN
#define ALIGN_STR __ALIGN_STR

#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  ALIGN; \
  name:
#endif
#endif /* LINKER_SCRIPT */

#ifndef WEAK
#define WEAK(name)	   \
	.weak name;	   \
	name:
#endif

#ifndef END
#define END(name) \
  .size name, .-name
#endif

/* If symbol 'name' is treated as a subroutine (gets called, and returns)
 * then please use ENDPROC to mark 'name' as STT_FUNC for the benefit of
 * static analysis tools such as stack depth analyzer.
 */
#ifndef ENDPROC
#define ENDPROC(name) \
  .type name, @function; \
  END(name)
#endif

#endif

/*
 * Endian independent macros for shifting bytes within registers.
 */
#ifndef __ARMEB__
#define pull            lsr
#define push            lsl
#define get_byte_0      lsl #0
#define get_byte_1	lsr #8
#define get_byte_2	lsr #16
#define get_byte_3	lsr #24
#define put_byte_0      lsl #0
#define put_byte_1	lsl #8
#define put_byte_2	lsl #16
#define put_byte_3	lsl #24
#else
#define pull            lsl
#define push            lsr
#define get_byte_0	lsr #24
#define get_byte_1	lsr #16
#define get_byte_2	lsr #8
#define get_byte_3      lsl #0
#define put_byte_0	lsl #24
#define put_byte_1	lsl #16
#define put_byte_2	lsl #8
#define put_byte_3      lsl #0
#endif

/*
 * Data preload for architectures that support it
 */
#if __LINUX_ARM_ARCH__ >= 5
#define PLD(code...)	code
#else
#define PLD(code...)
#endif

/*
 * This can be used to enable code to cacheline align the destination
 * pointer when bulk writing to memory.  Experiments on StrongARM and
 * XScale didn't show this a worthwhile thing to do when the cache is not
 * set to write-allocate (this would need further testing on XScale when WA
 * is used).
 *
 * On Feroceon there is much to gain however, regardless of cache mode.
 */
#ifdef CONFIG_CPU_FEROCEON
#define CALGN(code...) code
#else
#define CALGN(code...)
#endif

/*
 * Enable and disable interrupts
 */
#if __LINUX_ARM_ARCH__ >= 6
	.macro	disable_irq_notrace
	cpsid	i
	.endm

	.macro	enable_irq_notrace
	cpsie	i
	.endm
#else
	.macro	disable_irq_notrace
	msr	cpsr_c, #PSR_I_BIT | SVC_MODE
	.endm

	.macro	enable_irq_notrace
	msr	cpsr_c, #SVC_MODE
	.endm
#endif

	.macro asm_trace_hardirqs_off
#if defined(CONFIG_TRACE_IRQFLAGS)
	stmdb   sp!, {r0-r3, ip, lr}
	bl	trace_hardirqs_off
	ldmia	sp!, {r0-r3, ip, lr}
#endif
	.endm

	.macro asm_trace_hardirqs_on_cond, cond
#if defined(CONFIG_TRACE_IRQFLAGS)
	/*
	 * actually the registers should be pushed and pop'd conditionally, but
	 * after bl the flags are certainly clobbered
	 */
	stmdb   sp!, {r0-r3, ip, lr}
	bl\cond	trace_hardirqs_on
	ldmia	sp!, {r0-r3, ip, lr}
#endif
	.endm

	.macro asm_trace_hardirqs_on
	asm_trace_hardirqs_on_cond al
	.endm

	.macro disable_irq
	disable_irq_notrace
	asm_trace_hardirqs_off
	.endm

	.macro enable_irq
	asm_trace_hardirqs_on
	enable_irq_notrace
	.endm
/*
 * Save the current IRQ state and disable IRQs.  Note that this macro
 * assumes FIQs are enabled, and that the processor is in SVC mode.
 */
	.macro	save_and_disable_irqs, oldcpsr
#ifdef CONFIG_CPU_V7M
	mrs	\oldcpsr, primask
#else
	mrs	\oldcpsr, cpsr
#endif
	disable_irq
	.endm

	.macro	save_and_disable_irqs_notrace, oldcpsr
	mrs	\oldcpsr, cpsr
	disable_irq_notrace
	.endm

/*
 * Restore interrupt state previously stored in a register.  We don't
 * guarantee that this will preserve the flags.
 */
	.macro	restore_irqs_notrace, oldcpsr
#ifdef CONFIG_CPU_V7M
	msr	primask, \oldcpsr
#else
	msr	cpsr_c, \oldcpsr
#endif
	.endm

	.macro restore_irqs, oldcpsr
	tst	\oldcpsr, #PSR_I_BIT
	asm_trace_hardirqs_on_cond eq
	restore_irqs_notrace \oldcpsr
	.endm

#define USER(x...)				\
9999:	x;					\
	.pushsection __ex_table,"a";		\
	.align	3;				\
	.long	9999b,9001f;			\
	.popsection

#ifdef CONFIG_SMP
#define ALT_SMP(instr...)					\
9998:	instr
/*
 * Note: if you get assembler errors from ALT_UP() when building with
 * CONFIG_THUMB2_KERNEL, you almost certainly need to use
 * ALT_SMP( W(instr) ... )
 */
#define ALT_UP(instr...)					\
	.pushsection ".alt.smp.init", "a"			;\
	.long	9998b						;\
9997:	instr							;\
	.if . - 9997b != 4					;\
		.error "ALT_UP() content must assemble to exactly 4 bytes";\
	.endif							;\
	.popsection
#define ALT_UP_B(label)					\
	.equ	up_b_offset, label - 9998b			;\
	.pushsection ".alt.smp.init", "a"			;\
	.long	9998b						;\
	W(b)	. + up_b_offset					;\
	.popsection
#else
#define ALT_SMP(instr...)
#define ALT_UP(instr...) instr
#define ALT_UP_B(label) b label
#endif

/*
 * Instruction barrier
 */
	.macro	instr_sync
#if __LINUX_ARM_ARCH__ >= 7
	isb
#elif __LINUX_ARM_ARCH__ == 6
	mcr	p15, 0, r0, c7, c5, 4
#endif
	.endm

/*
 * SMP data memory barrier
 */
	.macro	smp_dmb mode
#ifdef CONFIG_SMP
#if __LINUX_ARM_ARCH__ >= 7
	.ifeqs "\mode","arm"
	ALT_SMP(dmb	ish)
	.else
	ALT_SMP(W(dmb)	ish)
	.endif
#elif __LINUX_ARM_ARCH__ == 6
	ALT_SMP(mcr	p15, 0, r0, c7, c10, 5)	@ dmb
#else
#error Incompatible SMP platform
#endif
	.ifeqs "\mode","arm"
	ALT_UP(nop)
	.else
	ALT_UP(W(nop))
	.endif
#endif
	.endm

#if defined(CONFIG_CPU_V7M)
	/*
	 * setmode is used to assert to be in svc mode during boot. For v7-M
	 * this is done in __v7m_setup, so setmode can be empty here.
	 */
	.macro	setmode, mode, reg
	.endm
#elif defined(CONFIG_THUMB2_KERNEL)
	.macro	setmode, mode, reg
	mov	\reg, #\mode
	msr	cpsr_c, \reg
	.endm
#else
	.macro	setmode, mode, reg
	msr	cpsr_c, #\mode
	.endm
#endif

/*
 * Helper macro to enter SVC mode cleanly and mask interrupts. reg is
 * a scratch register for the macro to overwrite.
 *
 * This macro is intended for forcing the CPU into SVC mode at boot time.
 * you cannot return to the original mode.
 */
.macro safe_svcmode_maskall reg:req
#if __LINUX_ARM_ARCH__ >= 6
	mrs	\reg , cpsr
	eor	\reg, \reg, #HYP_MODE
	tst	\reg, #MODE_MASK
	bic	\reg , \reg , #MODE_MASK
	orr	\reg , \reg , #PSR_I_BIT | PSR_F_BIT | SVC_MODE
THUMB(	orr	\reg , \reg , #PSR_T_BIT	)
	bne	1f
	orr	\reg, \reg, #PSR_A_BIT
	adr	lr, BSYM(2f)
	msr	spsr_cxsf, \reg
	__MSR_ELR_HYP(14)
	__ERET
1:	msr	cpsr_c, \reg
2:
#else
/*
 * workaround for possibly broken pre-v6 hardware
 * (akita, Sharp Zaurus C-1000, PXA270-based)
 */
	setmode	PSR_F_BIT | PSR_I_BIT | SVC_MODE, \reg
#endif
.endm

/*
 * STRT/LDRT access macros with ARM and Thumb-2 variants
 */
#ifdef CONFIG_THUMB2_KERNEL

	.macro	usraccoff, instr, reg, ptr, inc, off, cond, abort, t=TUSER()
9999:
	.if	\inc == 1
	\instr\cond\()b\()\t\().w \reg, [\ptr, #\off]
	.elseif	\inc == 4
	\instr\cond\()\t\().w \reg, [\ptr, #\off]
	.else
	.error	"Unsupported inc macro argument"
	.endif

	.pushsection __ex_table,"a"
	.align	3
	.long	9999b, \abort
	.popsection
	.endm

	.macro	usracc, instr, reg, ptr, inc, cond, rept, abort
	@ explicit IT instruction needed because of the label
	@ introduced by the USER macro
	.ifnc	\cond,al
	.if	\rept == 1
	itt	\cond
	.elseif	\rept == 2
	ittt	\cond
	.else
	.error	"Unsupported rept macro argument"
	.endif
	.endif

	@ Slightly optimised to avoid incrementing the pointer twice
	usraccoff \instr, \reg, \ptr, \inc, 0, \cond, \abort
	.if	\rept == 2
	usraccoff \instr, \reg, \ptr, \inc, \inc, \cond, \abort
	.endif

	add\cond \ptr, #\rept * \inc
	.endm

#else	/* !CONFIG_THUMB2_KERNEL */

	.macro	usracc, instr, reg, ptr, inc, cond, rept, abort, t=TUSER()
	.rept	\rept
9999:
	.if	\inc == 1
	\instr\cond\()b\()\t \reg, [\ptr], #\inc
	.elseif	\inc == 4
	\instr\cond\()\t \reg, [\ptr], #\inc
	.else
	.error	"Unsupported inc macro argument"
	.endif

	.pushsection __ex_table,"a"
	.align	3
	.long	9999b, \abort
	.popsection
	.endr
	.endm

#endif	/* CONFIG_THUMB2_KERNEL */

	.macro	strusr, reg, ptr, inc, cond=al, rept=1, abort=9001f
	usracc	str, \reg, \ptr, \inc, \cond, \rept, \abort
	.endm

	.macro	ldrusr, reg, ptr, inc, cond=al, rept=1, abort=9001f
	usracc	ldr, \reg, \ptr, \inc, \cond, \rept, \abort
	.endm

/* Utility macro for declaring string literals */
	.macro	string name:req, string
	.type \name , #object
\name:
	.asciz "\string"
	.size \name , . - \name
	.endm

	.macro check_uaccess, addr:req, size:req, limit:req, tmp:req, bad:req
#ifndef CONFIG_CPU_USE_DOMAINS
	adds	\tmp, \addr, #\size - 1
	sbcccs	\tmp, \tmp, \limit
	bcs	\bad
#endif
	.endm

#endif /* __ASM_ASSEMBLER_H__ */
