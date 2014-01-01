/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER_NOEC(_divide_error_handler, T_DIVIDE)
TRAPHANDLER_NOEC(_debug_exception_handler, T_DEBUG)
TRAPHANDLER_NOEC(_non_maskable_interrupt_handler, T_NMI)
TRAPHANDLER_NOEC(_breakpoint_handler, T_BRKPT)
TRAPHANDLER_NOEC(_overflow_handler, T_OFLOW)
TRAPHANDLER_NOEC(_bounds_check_handler, T_BOUND)
TRAPHANDLER_NOEC(_illegal_opcode_handler, T_ILLOP)
TRAPHANDLER_NOEC(_device_not_available_handler, T_DEVICE)
TRAPHANDLER(_double_fault_handler, T_DBLFLT)
TRAPHANDLER(_invalid_tss_handler, T_TSS)
TRAPHANDLER(_segment_not_present_handler, T_SEGNP)
TRAPHANDLER(_stack_exception_handler, T_STACK)
TRAPHANDLER(_general_protection_fault_handler, T_GPFLT)
TRAPHANDLER(_page_fault_handler, T_PGFLT)
TRAPHANDLER_NOEC(_floating_point_error_handler, T_FPERR)
TRAPHANDLER(_aligment_check_handler, T_ALIGN)
TRAPHANDLER_NOEC(_machine_check_handler, T_MCHK)
TRAPHANDLER_NOEC(_simd_floating_point_error_handler, T_SIMDERR)
TRAPHANDLER_NOEC(_system_call_handler, T_SYSCALL)



/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
   pushw    $0x0
   pushw    %ds
   pushw    $0x0
   pushw    %es
   pushal

   movl     $GD_KD, %eax

   movw     %ax, %ds
   movw     %ax, %es

   pushl    %esp

   call     trap
