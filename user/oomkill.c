#include <lib.h>

int main(void)
{
	pid_t child;

	child = fork();

	if (child > 0) {
		printf("Child is %p\n", child);
		printf("[PID %5u] I am the parent!\n", getpid());
		// Map more memory.
		// This process will be killed to free up space.

	} else {
		printf("[PID %5u] I am the child!\n", getpid());
		// Map some memory.
	}

	return 0;
}

