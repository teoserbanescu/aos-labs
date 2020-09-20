.section .text

#include <x86-64/gdt.h>
#include <x86-64/idt.h>
#include <x86-64/memory.h>

#include <cpu.h>

.macro ISR_NOERRCODE int_no
.global isr\int_no
isr\int_no:
	pushq $0
	pushq $\int_no
	jmp isr_common_stub
.endm

.macro ISR_ERRCODE int_no
.global isr\int_no
isr\int_no:
	pushq $\int_no
	jmp isr_common_stub
.endm

isr_common_stub:
	/* LAB 3: your code here. */
	/* Save the register state on the stack. */

// https://canvas.vu.nl/courses/49330/discussion_topics/289392

	/* Recover the segments. */
	movq $GDT_KDATA, %rax
    mov %rax, %ds
    mov %rax, %ss
    mov %rax, %es

	/* Jump to the generic interrupt handler. */
	movq %rsp, %rdi
	jmp int_handler

.global syscall64
syscall64:
	/* LAB 3: your code here. */
	/* Store the userspace stack pointer in the per-cpu struct. */

	/* Load the kernel stack pointer from the per-cpu struct. */

	/* Store the register state into the per-cpu struct. */

	/* Jump to the system call handler. */
	movq %r10, %rcx
	pushq %rbp
	pushq $0
	jmp syscall_handler

.global iret64
iret64:
	/* LAB 3: your code here. */
	/* Restore the register state. */

	/* Return from the interrupt. */
	addq $16, %rsp

    pushq IF_SS(%rdi)
    pushq IF_RSP(%rdi)
    pushq IF_RFLAGS(%rdi)
    pushq IF_CS(%rdi)
    pushq IF_RIP(%rdi)

/* I think for context switch we need all of them
 */
	movq IF_R8(%rdi), %r8
	movq IF_R9(%rdi), %r9
	movq IF_R10(%rdi), %r10
	movq IF_R11(%rdi), %r11
	movq IF_R12(%rdi), %r12
	movq IF_R13(%rdi), %r13
	movq IF_R14(%rdi), %r14
    movq IF_R15(%rdi), %r15

	movq IF_RSI(%rdi), %rsi
	movq IF_RBP(%rdi), %rbp
	movq IF_RBX(%rdi), %rbx
	movq IF_RDX(%rdi), %rdx
	movq IF_RCX(%rdi), %rcx

	movq IF_DS(%rdi), %rax
	mov %rax, %ds
	mov %rax, %es

	movq IF_RAX(%rdi), %rax
	movq IF_RDI(%rdi), %rdi

	iretq

.global sysret64
sysret64:
	/* LAB 3: your code here. */
	/* Recover the register state. */

	/* Return from the system call. */
	sysretq

