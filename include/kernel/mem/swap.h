#pragma once

#include <spinlock.h>
#include <include/list.h>
#include <include/paging.h>

struct rmap {
	struct list node;
	uint8_t r;	// second chance bit
};

// global struct to keep swap related data
struct swap_info_struct {
	struct spinlock lock;
	struct list rmap;	// lru list
};

void swap_init();

// put data to disk and free frames (one for now)
void swap_free();
// add page to lru list
void swap_rmap_add(struct page_info *page);
// add page from lru list
void swap_rmap_remove(struct page_info *page);

void swap_kd(); // task kernel daemon

