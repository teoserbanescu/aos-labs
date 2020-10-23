#pragma once

#include <spinlock.h>
#include <include/list.h>
#include <include/paging.h>

#define FIRST_CHANCE 0
#define SECOND_CHANCE 1

void swap_init();

// put data to disk and free frames (one for now)
void swap_free();
// add page to lru list
void swap_rmap_add(struct page_info *page);
// add page from lru list
void swap_rmap_remove(struct page_info *page);

void swap_kd(); // task kernel daemon

void swap_in(physaddr_t *entry);