#include <kernel/mem/swap.h>

void put_page();
void get_page();

void swap_init();
void swap_out();
void swap_in();

void swap_kd(); // task kernel daemon