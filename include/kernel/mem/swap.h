#pragma once

#include <spinlock.h>

//FIXME rmap or swap_info_struct
//struct rmap {
//	struct spinlock rmap_lock;
//};

struct swap_info_struct {
	unsigned long	flags;
	signed short prio;
	signed char type;
	unsigned int max;
	unsigned char *swap_map;
	unsigned int lowest_bit;
	unsigned int highest_bit;
	unsigned int pages;
	unsigned int inuse_pages;
	unsigned int cluster_next;
	struct block_device *bdev;
	struct file *swap_file;
	spinlock_t lock;
};

void swap_init();
void swap_out();
void swap_in();

void swap_kd(); // task kernel daemon

/* clock lru_list second chance
 *
 * */