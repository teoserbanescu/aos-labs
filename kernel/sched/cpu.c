#include <cpu.h>

struct cpuinfo cpus[NCPUS];
struct cpuinfo *boot_cpu = cpus;
volatile size_t ncpus = 1;

