#include <types.h>
#include <boot.h>
#include <list.h>
#include <paging.h>

#include <x86-64/asm.h>

#include <kernel/mem.h>
#include <kernel/tests.h>

extern struct list page_free_list[];

/* The kernel's initial PML4. */
struct page_table *kernel_pml4;

/* This function sets up the initial PML4 for the kernel. */
int pml4_setup(struct boot_info *boot_info)
{
	struct page_info *page;

	/* Allocate the kernel PML4. */
	page = page_alloc(ALLOC_ZERO);

	if (!page) {
		panic("unable to allocate the PML4!");
	}

	kernel_pml4 = page2kva(page);

	/* Map in the regions used by the kernel from the ELF header passed to
	 * us through the boot info struct.
	 */
	boot_map_kernel(kernel_pml4, KADDR((uintptr_t)boot_info->elf_hdr));

	/* Use the physical memory that 'bootstack' refers to as the kernel
	 * stack. The kernel stack grows down from virtual address KSTACK_TOP.
	 * Map 'bootstack' to [KSTACK_TOP - KSTACK_SIZE, KSTACK_TOP).
	 */
    boot_map_region(kernel_pml4, (void *)(KSTACK_TOP-KSTACK_SIZE), KSTACK_SIZE, (physaddr_t)bootstack , PAGE_PRESENT | PAGE_WRITE | PAGE_NO_EXEC);

	/* Map in the pages from the buddy allocator as RW-. */
    boot_map_region(kernel_pml4, (void *)KPAGES, npages * (sizeof *pages),  PADDR(pages), PAGE_PRESENT | PAGE_WRITE | PAGE_NO_EXEC);

    /* Migrate the struct page_info structs to the newly mapped area using
     * buddy_migrate().
     */
	buddy_migrate();

	return 0;
}

/*
 * Set up a four-level page table:
 * kernel_pml4 is its linear (virtual) address of the root
 *
 * This function only sets up the kernel part of the address space (i.e.
 * addresses >= USER_TOP). The user part of the address space will be set up
 * later.
 *
 * From USER_TOP to USER_LIM, the user is allowed to read but not write.
 * Above USER_LIM, the user cannot read or write.
 */
void mem_init(struct boot_info *boot_info)
{
	struct mmap_entry *entry;
	uintptr_t highest_addr = 0;
	uint32_t cr0;
	size_t i, n;

	/* Align the areas in the memory map. */
	align_boot_info(boot_info);

	/* Set up the page free lists. */
	for (i = 0; i < BUDDY_MAX_ORDER; ++i) {
		list_init(page_free_list + i);
	};

	/* Find the amount of pages to allocate structs for. */
	entry = (struct mmap_entry *)((physaddr_t)boot_info->mmap_addr);

	for (i = 0; i < boot_info->mmap_len; ++i, ++entry) {
		if (entry->type != MMAP_FREE)
			continue;

		highest_addr = entry->addr + entry->len;
	}

	/* Limit the struct page_info array to the first 8 MiB, as the rest is
	 * still not accessible until lab 2.
	 */
	npages = MIN(BOOT_MAP_LIM, highest_addr) / PAGE_SIZE;

	/* Remove this line when you're ready to test this function. */

	/*
	 * Allocate an array of npages 'struct page_info's and store it in 'pages'.
	 * The kernel uses this array to keep track of physical pages: for each
	 * physical page, there is a corresponding struct page_info in this array.
	 * 'npages' is the number of physical pages in memory.  Your code goes here.
	 */
	pages = boot_alloc(npages * sizeof *pages);

	/*
	 * Now that we've allocated the initial kernel data structures, we set
	 * up the list of free physical pages. Once we've done so, all further
	 * memory management will go through the page_* functions. In particular, we
	 * can now map memory using boot_map_region or page_insert.
	 */
	page_init(boot_info);

	/* We will set up page tables here in lab 2. */

	/* Setup the initial PML4 for the kernel. */
	pml4_setup(boot_info);

	/* Enable the NX-bit. */
    write_msr(MSR_EFER, MSR_EFER_NXE);

	/* Check the kernel PML4. */
//	lab2_check_pml4();

	/* Load the kernel PML4. */
    load_pml4((struct page_table *)PADDR(kernel_pml4));

	/* Check the paging functions. */
//	lab2_check_paging();

	/* Add the rest of the physical memory to the buddy allocator. */
	page_init_ext(boot_info);

	/* Check the buddy allocator. */
//	lab2_check_buddy(boot_info);
}

/*
 * Initialize page structure and memory free list. After this is done, NEVER
 * use boot_alloc() again. After this function has been called to set up the
 * memory allocator, ONLY the buddy allocator should be used to allocate and
 * free physical memory.
 */
void page_init(struct boot_info *boot_info)
{
	struct page_info *page;
	struct mmap_entry *entry;
	uintptr_t pa, end;
	size_t i;

	/* Go through the array of struct page_info structs and:
	 *  1) call list_init() to initialize the linked list node.
	 *  2) set the reference count pp_ref to zero.
	 *  3) mark the page as in use by setting pp_free to zero.
	 *  4) set the order pp_order to zero.
	 */
	for (i = 0; i < npages; ++i) {
		/* LAB 1: your code here. */
		page = pages + i;
		page->pp_ref = 0;
		page->pp_free = 0;
		page->pp_order = 0;
		list_init(&page->pp_node);
	}

	entry = (struct mmap_entry *)KADDR(boot_info->mmap_addr);
	end = PADDR(boot_alloc(0));

	/* Go through the entries in the memory map:
	 *  1) Ignore the entry if the region is not free memory.
	 *  2) Iterate through the pages in the region.
	 *  3) If the physical address is above BOOT_MAP_LIM, ignore.
	 *  4) Hand the page to the buddy allocator by calling page_free() if
	 *     the page is not reserved.
	 *
	 * What memory is reserved?
	 *  - Address 0 contains the IVT and BIOS data.
	 *  - boot_info->elf_hdr points to the ELF header.
	 *  - Any address in [KERNEL_LMA, end) is part of the kernel.
	 */
	for (i = 0; i < boot_info->mmap_len; ++i, ++entry) {
		/* LAB 1: your code here. */
		for (pa = entry->addr; pa < entry->addr + entry->len; pa += PAGE_SIZE) {

			if (pa >= BOOT_MAP_LIM)
				continue;

			page = pa2page(pa);

			if ((pa == 0 ||
				 pa == PAGE_ADDR(PADDR(boot_info)) ||
				 pa == (uintptr_t)boot_info->elf_hdr ||
				 (KERNEL_LMA <= pa && pa < end) ||
				 entry->type != MMAP_FREE)) {
				continue;
			}

			page_free(page);
		}
	}
#ifdef DEBUG
	show_buddy_info();
#endif
}

/* Extend the buddy allocator by initializing the page structure and memory
 * free list for the remaining available memory.
 */
void page_init_ext(struct boot_info *boot_info)
{
	struct page_info *page;
	struct mmap_entry *entry;
	uintptr_t pa, end;
	size_t i;

	entry = (struct mmap_entry *)KADDR(boot_info->mmap_addr);
	end = PADDR(boot_alloc(0));

	/* Go through the entries in the memory map:
	 *  1) Ignore the entry if the region is not free memory.
	 *  2) Iterate through the pages in the region.
	 *  3) If the physical address is below BOOT_MAP_LIM, ignore.
	 *  4) Hand the page to the buddy allocator by calling page_free().
	 */
	for (i = 0; i < boot_info->mmap_len; ++i, ++entry) {
		/* LAB 2: your code here. */
        for (pa = entry->addr; pa < entry->addr + entry->len; pa += PAGE_SIZE) {

            if (pa < BOOT_MAP_LIM)
                continue;

            if(npages <= PAGE_INDEX(pa)){
                buddy_map_chunk(kernel_pml4, PAGE_INDEX(pa));
                boot_map_region(kernel_pml4, page2kva(pa2page(pa)), HPAGE_SIZE, pa, PAGE_PRESENT | PAGE_WRITE | PAGE_NO_EXEC);
            }

            page = pa2page(pa);

            if ((KERNEL_LMA <= pa && pa < end) ||
                    (entry->type != MMAP_FREE)) {
                continue;
            }

            page_free(page);
        }
	}
}

