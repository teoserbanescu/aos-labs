# [Lab 7] Under Pressure

## Design
Getting the design we had to read many sources, besides the lab and the lectures. We looked in the kernel code, read some chapters from MODERN OPERATING SYSTEMS book and searched through https://wiki.osdev.org 

We added a new structure inside page_info:
```
struct rmap {
	/* The anchor node for the lru like list where swap keeps track of pages */
	struct list node;

	/* Second chance bit */
	uint8_t r;

	/* Lock when updating the entry */
	struct spinlock lock;

	/* To update easily the number of active or swapped pages inside the task 
	 * Update them for oom reasons
	 * */
	struct task *task;

	/* Pointer to the page table entry.
	 * Saved when the page is allocated (populate_pte)
	 * Updated by swap_in and swap_out
	 * */
	physaddr_t *entry;
};
```
## OOM Killing
We keep 2 more field inside to task struct to find fast which has higher badness
```
uint32_t nactive_pages;
uint32_t nswapped_pages;
```


## Page swapping
Because of the observation below we thought of adding another flag for page tables entries and we added PAGE_SWAPPED flag. \
https://wiki.osdev.org/Paging
"The remaining bits 9 through 11 are not used by the processor, and are free for the OS to store some of its own accounting information. In addition, when P is not set, the processor ignores the rest of the entry and you can use all remaining 31 bits for extra information, like recording where the page has ended up in swap space."

A problem that we had was what pages to keep track of for swapping. And the data structures needed. MODERN OPERATING SYSTEMS book was helpful and we got the answer that we need to keep track of all pages. There was nothing stated about kernel pages, but we thought that would be complicated to deal with those. So we swap only user pages. One more reason was the thought that kernel should handle everything fast and one would not like to have its pages swapped.\
Page 211, chapter 3.4.3 The First-In, First-Out (FIFO) Page Replacement Algorithm.
"The operating system maintains a list of all pages currently in memory, with the most recent arrival at the tail and the least recent arrival at the head. On a page fault, the page at the head is removed and the new page added to the tail of the list."

We implemented clock so we would only move the head instead of removing and adding the page to the tail.

#### Swap out
Swapping out (freeing pages from memory and saving them to disk) is initiated from only 2 places.
One is from buddy when we do not have enough frames left. This is direct swapping. We could have increased the priority of the swap daemon, but it seemed cleaner to just free some pages when we need to. Also, we are already in the kernel space so we can just call a function. But we look if the kernel thread is already running (checking his lock) and if so we just yield and we should have enough memory after that. We can still do not have enough memory when we come back maybe we were scheduled too far away from this moment and or another thread moved faster and already consumed it. We just swap out again in this case.
#### Swap in
Swapping in should have been simpler after we implemented swapping out, but when we actually started to run the code and debugging there were multiple bugs, but we solved them. Here are some problems we had:
The most interesting started from the fact that  when swap_in is called, we need to have a page allocated where we can copy the data from the disk. A more advanced version might have a cache from where we could grab this page. But we do not have this cache, so we have to call page_alloc. But here comes the problem when the buddy does not have anymore frames and we have to call swap_out. And so we have to make sure that all locks are right and we do not deadlock. Another bug we had was from the fact that we did not occupied and freed the sectors properly.   
#### Blocking vs Non-blocking
Swap is non-blocking because we implemented yielding from kernel.
### Limitations
We still have to make some tweaks to the kernel thread.
### Possible improvements
Making sure that there are always some pages free for swap_in so we do not have to call swap_out and waste time.
Have a bitmap/bittree for sectors, maybe in a similar way a filesystem does. For now we waste some memory and just allocated a byte for every sector, where we store if it is free or not.

## Writing to disk and yielding from kernel
We starting with a simple blocking function for reading and writing to disk, just to get used with how to do it. Then more problems arrived for non blocking. First, we are in kernel space when we do this, but we want to be non-blocking. This means that we should yield somehow. We do not have a reason to do disk operations when there is no user task so this means that there is always a cur_task present. We also wanted to implement kernel threads so cur_task may be a user or a kernel task. A thought was to make all disk operations in a kernel thread, but this is no helpful because we still need to do something until it finishes. So, we needed a sched_yield function and we called it ksched_yield. What this does is saving the kernel frame and calls sched_yield. Because the frame is already stored for a user task we added another int_frame to task structure, where we store the kernel frame. We called it kframe, and now the only thing that was needed to do was to pop kframe instead of frame when we run the task after it yielded. The kernel thread also stores its information there, to have somewhat of a consistency.

## Kernel task
### Limitations
Scheduling kernel tasks needs to be tweaked. This is because every threads pops tasks from its local runqueue, we had the problem that when user tasks finished on that thread, this kernel task might keep a thread busy for no reason.

## References
[1] https://wiki.osdev.org/Paging
[3] https://elixir.bootlin.com/linux/latest/source/mm/swap.c
[4] https://elixir.bootlin.com/linux/latest/source/include/linux/mmzone.h#L246
[5] https://elixir.bootlin.com/linux/latest/source/mm/oom_kill.c
