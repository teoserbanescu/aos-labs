#include <kernel/mem/swap.h>
#include <kernel/sched/sched.h>
#include <kernel/dev/disk.h>
#include <include/kernel/mem.h>
#include <error.h>
#include <include/cpu.h>
#include <include/kernel/sched.h>
#include <include/atomic.h>

#define SWAP_DISK_ID 1
#define DISK_SIZE 128 * 1024 * 1024
#define NMAX_SECTORS DISK_SIZE / SECT_SIZE

struct spinlock disk_lock;
struct spinlock lru_lock;

struct list *lru_head = NULL;
struct task *task_swapkd = NULL;

char *sectors = NULL;

struct page_info* lru_get_page() {
	struct page_info *page;

	spin_lock(&lru_lock);

	if (!lru_head)
		goto out;

	while(true) {
		page = container_of(lru_head, struct page_info, rmap);

		if (page->rmap.r == SECOND_CHANCE) {
			list_pop_left(lru_head);
			spin_unlock(&lru_lock);
			return page;
		}

		page->rmap.r = SECOND_CHANCE;
		lru_head = lru_head->next;
	}

out:
	spin_unlock(&lru_lock);
	cprintf("SWAP EMPTY but page was requested\n");
	return NULL;
}

/* save page to disk */
static void put_page(struct page_info *page, int sector) {
	struct disk *disk;
	char *buf;
	static int nsectors;

	nsectors = PAGE_SIZE / SECT_SIZE; /* = 8 */
	disk = disks[SWAP_DISK_ID];
	buf = page2kva(page);

	while(!spin_trylock(&disk_lock)) {
		ksched_yield();
	}

	assert(disk_write(disk, buf, nsectors, sector)  == -EAGAIN);

	while (!disk_poll(disk)){
		ksched_yield();
	}

	assert(disk_write(disk, buf, nsectors, sector) == PAGE_SIZE);

	spin_unlock(&disk_lock);
}

void put_page_blocking(struct page_info *page) {
	char *buf;
	int nsectors = PAGE_SIZE / SECT_SIZE; /* = 8 */
	int sector = 0; // sector id
	struct disk *disk;

	disk = disks[SWAP_DISK_ID];

	buf = page2kva(page);
	if(disk_write(disk, buf, nsectors, sector) == -EAGAIN) { // Start transaction
		int64_t n = 0;
		while(n < PAGE_SIZE) {
			n += disk_write(disk, buf + n * SECT_SIZE,
						   nsectors - n, sector + n);
		}
	}
}

void get_page_blocking(struct page_info *page) {
	char *buf;
	int nsectors = PAGE_SIZE / SECT_SIZE; /* = 8 */
	int sector = 0; // sector id
	struct disk *disk;

	disk = disks[SWAP_DISK_ID];

	buf = page2kva(page);
	if(disk_read(disk, buf, nsectors, sector) == -EAGAIN) { // Start transaction
		int64_t n = 0;
		while(n < PAGE_SIZE) {
			n += disk_read(disk, buf + n * SECT_SIZE,
						 nsectors - n, sector + n);
		}
	}
}

/* read page from disk */
static void get_page(struct page_info *page, int sector) {
	struct disk *disk;
	char *buf;
	static int nsectors;

	nsectors = PAGE_SIZE / SECT_SIZE; /* = 8 */
	disk = disks[SWAP_DISK_ID];
	buf = page2kva(page);

	while(!spin_trylock(&disk_lock)) {
		ksched_yield();
	}

	assert(disk_read(disk, buf, nsectors, sector)  == -EAGAIN);

	while (!disk_poll(disk)){
		ksched_yield();
	}

	assert(disk_read(disk, buf, nsectors, sector) == PAGE_SIZE);

	spin_unlock(&disk_lock);
}

void test_disk() {
	struct page_info *page;
	char *buf;
	page = page_alloc(ALLOC_ZERO);
//	get_page_blocking(page);
	buf = page2kva(page);

	for (int i = 0; i < PAGE_SIZE; ++i) {
		buf[i] = 'A' + i / SECT_SIZE;
	}
//	put_page_blocking(page);
	put_page(page, 0);

	for (int i = 0; i < PAGE_SIZE; ++i) {
		buf[i] = 0;
	}

//	get_page_blocking(page);
	get_page(page, 0);

	for (int i = 0; i < PAGE_SIZE; ++i) {
		if (buf[i])
			cprintf("%c", buf[i]);
		else
			cprintf("0");
	}
}

void swap_init() {
//	test_disk();
	spin_init(&disk_lock, "disk_lock");
	spin_init(&lru_lock, "lru_lock");

	sectors = (char *)page2kva(page_alloc(ALLOC_ZERO | ALLOC_HUGE));
	task_swapkd = ktask_create(swap_kd);
}

int get_free_sector() {
	int i;

	spin_lock(&disk_lock);

	for (i = 0; i < NMAX_SECTORS; ++i) {
		if (!sectors[i]) {
			break;
		}
	}

	assert(i < NMAX_SECTORS);
	sectors[i] = 1;

	spin_unlock(&disk_lock);

	return i;
}

static void set_swapped(struct page_info *page, int sector) {
	physaddr_t *entry;

	atomic_inc(&page->rmap.task->nswapped_pages);
	atomic_dec(&page->rmap.task->nactive_pages);

	entry = page->rmap.entry;

	spin_lock(&page->rmap.lock);
	*entry &= (~PAGE_PRESENT);
	*entry |= (PAGE_SWAP);
	*entry &= (PAGE_MASK);
	*entry |= PAGE_ADDR(sector << PAGE_TABLE_SHIFT);
	spin_unlock(&page->rmap.lock);
}

static void set_active(struct page_info *page,physaddr_t *entry) {

	atomic_dec(&page->rmap.task->nswapped_pages);
	atomic_inc(&page->rmap.task->nactive_pages);

	spin_lock(&page->rmap.lock);
	*entry &= (~PAGE_SWAP);
	*entry |= (PAGE_PRESENT);
	*entry &= (PAGE_MASK);
	*entry |= PAGE_ADDR(page2pa(page));
	spin_unlock(&page->rmap.lock);
}

static void do_swap_out() {
	struct page_info *page;
	int sector;

	page = lru_get_page();

	sector = get_free_sector();
	set_swapped(page, sector);
	put_page(page, sector);
	page->pp_ref = 0;
	page_free(page);

}

void swap_in(physaddr_t *entry) {
	struct page_info *page;
	int sector;

	assert(*entry | PAGE_SWAP);
	sector = PAGE_ADDR(*entry) >> PAGE_TABLE_SHIFT;
	page = page_alloc(ALLOC_SIMPLE);
	swap_rmap_add(page);
	set_active(page, entry);
	get_page(page, sector);
}

// direct swapping
// called only from page_alloc, locked by buddy
void swap_free() {
	static int swapping = 0;
/*	Swapping is running. This means that swap_kd got activated
 *  This function is called only from page_alloc when there is not enough mem.
 *	On another thread or waiting for disk
 *	wait to finish until running this task
 */
	while (!spin_trylock(&task_swapkd->task_lock)) {
		swapping = 1;
		ksched_yield();
	}
//	gem mem fast, do not let user wait
//	already swapped by kthread
	if (!swapping) {
		do_swap_out();
	}
	spin_unlock(&task_swapkd->task_lock);
}

void swap_rmap_add(struct page_info *page) {
	page->rmap.r = FIRST_CHANCE;
	page->rmap.task = cur_task;
	list_init(&page->rmap.node);
	spin_init(&page->rmap.lock, "rmap page lock");

	spin_lock(&lru_lock);

	if (!lru_head)
		lru_head = &page->rmap.node;
	else
		list_push_left(lru_head, &page->rmap.node);

	spin_unlock(&lru_lock);
}

void swap_rmap_remove(struct page_info *page) {
	if (!page->rmap.task) {
		return;
	}
	spin_lock(&lru_lock);

	list_remove(&page->rmap.node);

	spin_unlock(&lru_lock);
}

void swap_kd() {
	cprintf("hello from %s cpuid %u\n", __PRETTY_FUNCTION__, this_cpu->cpu_id);
	cur_task->task_runtime = -2;
/*
//	cprintf("hello from kernel task cpuid %u\n", this_cpu->cpu_id);
	while (!spin_trylock(&task_swapkd->task_lock)) {
		ksched_yield();
	}
//	TODO while (not enough mem) do_swap_out()

	spin_unlock(&task_swapkd->task_lock);
*/

}