/*
 * Simple implementation of cprintf console output for the kernel, based on
 * printfmt() and the kernel console's cputchar().
 */

#include <types.h>
#include <cpu.h>
#include <spinlock.h>
#include <stdio.h>
#include <stdarg.h>

//#ifndef USE_BIG_KERNEL_LOCK
//struct spinlock console_lock = {
//#ifdef DEBUG_SPINLOCK
//	.name = "console_lock",
//#endif
//};
//#endif

struct spinlock console_lock = {
	.name = "console_lock",
};

static void putch(int ch, int *cnt)
{
	cputchar(ch);
	*cnt++;
}

int vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	spin_lock(&console_lock);
	vprintfmt((void*)putch, &cnt, fmt, ap);
	spin_unlock(&console_lock);
	return cnt;
}

int cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

