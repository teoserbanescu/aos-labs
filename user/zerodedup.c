#include <lib.h>
char *page;

int main(void)
{
	pid_t child;
	print_vmas();
	page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
					  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	printf("%p\n", page);
	printf("rodata\n");

	child = fork();
	if (child > 0) {
		printf("[PID %5u] I am the parent!\n", getpid());
		sched_yield();
		page[0] = 1;
		printf("[PID %5u] %d%d%d\n",getpid(), page[0], page[1], page[2]);
		print_vmas();
	} else {
		printf("[PID %5u] I am the child!\n", getpid());
		sched_yield();
		page[0] = 2;
		sched_yield();
		printf("[PID %5u] %d%d%d\n",getpid(), page[0], page[1], page[2]);
	}

	return 0;
}
