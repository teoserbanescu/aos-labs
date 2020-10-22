#include <kernel/mem/swap.h>
#include <kernel/sched/sched.h>
#include <kernel/dev/disk.h>
#include <include/kernel/mem.h>
#include <error.h>

struct spinlock disk_lock;
struct spinlock lru_lock;

#define SECTOR_SIZE 512
#define SWAP_DISK_ID 1

/* save page to disk */
static void put_page(struct page_info *page, int sector) {
	struct disk *disk;
	char *buf;
	static int nsectors = PAGE_SIZE / SECT_SIZE; /* = 8 */

	while(!spin_trylock(&disk_lock)) {
		ksched_yield();
	}

	disk = disks[SWAP_DISK_ID];
	buf = page2kva(page);
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
	static int nsectors = PAGE_SIZE / SECT_SIZE; /* = 8 */

	while(!spin_trylock(&disk_lock)) {
		ksched_yield();
	}

	disk = disks[SWAP_DISK_ID];
	buf = page2kva(page);
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
	test_disk();
}

void swap_free() {

}

void swap_rmap_add(struct page_info *page) {

}

void swap_rmap_remove(struct page_info *page) {

}

void swap_kd() {

}