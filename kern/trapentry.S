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
#define TRAPHANDLER(name, num)                                          \
        .text;                                                          \
        .globl name;            /* define global symbol for 'name' */   \
        .type name, @function;  /* symbol type is function */           \
        .align 2;               /* align function definition */         \
        name:                   /* function starts here */              \
        pushl $(num);                                                   \
        jmp _alltraps;                                                  \
        .data;                                                          \
        .long name

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)                                     \
        .text;                                                          \
        .globl name;                                                    \
        .type name, @function;                                          \
        .align 2;                                                       \
        name:                                                           \
        pushl $0;                                                       \
        pushl $(num);                                                   \
        jmp _alltraps;                                                  \
        .data;                                                          \
        .long name

# handler array
.data
.globl handlers
handlers:
.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

  TRAPHANDLER_NOEC(handler0, T_DIVIDE);
  TRAPHANDLER_NOEC(handler1, T_DEBUG);
  TRAPHANDLER_NOEC(handler2, T_NMI);
  TRAPHANDLER_NOEC(handler3, T_BRKPT);
  TRAPHANDLER_NOEC(handler4, T_OFLOW);
  TRAPHANDLER_NOEC(handler5, T_BOUND);
  TRAPHANDLER_NOEC(handler6, T_ILLOP);
  TRAPHANDLER_NOEC(handler7, T_DEVICE);
  TRAPHANDLER(handler8, T_DBLFLT);
  TRAPHANDLER(handler9, T_DIVIDE);
  TRAPHANDLER(handler10, T_TSS);
  TRAPHANDLER(handler11, T_SEGNP);
  TRAPHANDLER(handler12, T_STACK);
  TRAPHANDLER(handler13, T_GPFLT);
  TRAPHANDLER(handler14, T_PGFLT);
  TRAPHANDLER(handler15, T_PGFLT);
  TRAPHANDLER_NOEC(handler16, T_FPERR);
  TRAPHANDLER_NOEC(handler17, T_ALIGN);
  TRAPHANDLER_NOEC(handler18, T_MCHK);
  TRAPHANDLER_NOEC(handler19, T_SIMDERR);

  TRAPHANDLER_NOEC(handler20, 20);
  TRAPHANDLER_NOEC(handler21, 21);
  TRAPHANDLER_NOEC(handler22, 22);
  TRAPHANDLER_NOEC(handler23, 23);
  TRAPHANDLER_NOEC(handler24, 24);
  TRAPHANDLER_NOEC(handler25, 25);
  TRAPHANDLER_NOEC(handler26, 26);
  TRAPHANDLER_NOEC(handler27, 27);
  TRAPHANDLER_NOEC(handler28, 28);
  TRAPHANDLER_NOEC(handler29, 29);
  TRAPHANDLER_NOEC(handler30, 30);
  TRAPHANDLER_NOEC(handler31, 31);
  TRAPHANDLER_NOEC(handler32, 32);
  TRAPHANDLER_NOEC(handler33, 33);
  TRAPHANDLER_NOEC(handler34, 34);
  TRAPHANDLER_NOEC(handler35, 35);
  TRAPHANDLER_NOEC(handler36, 36);
  TRAPHANDLER_NOEC(handler37, 37);
  TRAPHANDLER_NOEC(handler38, 38);
  TRAPHANDLER_NOEC(handler39, 39);
  TRAPHANDLER_NOEC(handler40, 40);
  TRAPHANDLER_NOEC(handler41, 41);
  TRAPHANDLER_NOEC(handler42, 42);
  TRAPHANDLER_NOEC(handler43, 43);
  TRAPHANDLER_NOEC(handler44, 44);
  TRAPHANDLER_NOEC(handler45, 45);
  TRAPHANDLER_NOEC(handler46, 46);
  TRAPHANDLER_NOEC(handler47, 47);
  TRAPHANDLER_NOEC(handler48, T_SYSCALL);

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
    // continue to build Trapframe
    pushl %ds
    pushl %es
    pushal

    // store kernel data segment into ds and es
    movw $(GD_KD), %ax
    movw %ax, %ds
    movw %ax, %es

    // calling convention
    pushl %esp
    call trap
