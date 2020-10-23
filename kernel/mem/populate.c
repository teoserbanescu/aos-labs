#include <types.h>
#include <paging.h>
#include <atomic.h>

#include <kernel/mem.h>
#include <include/kernel/mem/swap.h>

struct populate_info {
	uint64_t flags;
	uintptr_t base, end;
	bool isuser_mem;
};

/* Similar to ptbl_alloc, but flags are given here
 * and udata is a different structure
 */
static int populate_pte(physaddr_t *entry, uintptr_t base, uintptr_t end,
    struct page_walker *walker)
{
	struct page_info *page;
	struct populate_info *info = walker->udata;

	/* LAB 3: your code here. */
	if (*entry & PAGE_PRESENT) {
		return 0;
	}

	page = page_alloc(ALLOC_ZERO);
	atomic_inc(&page->pp_ref);
	*entry = PAGE_ADDR(page2pa(page)) | info->flags;

	/* Add to global list of pages for swap purposes
	 * No swap for kernel pages
	 * */
	if (info->isuser_mem) {
		page->rmap.entry = entry;
		swap_rmap_add(page);
	}

	return 0;
}

static int populate_pde(physaddr_t *entry, uintptr_t base, uintptr_t end,
    struct page_walker *walker)
{
	struct page_info *page;
	struct populate_info *info = walker->udata;

	/* LAB 3: your code here. */
//	no huge pages for now
	populate_pte(entry, base, end, walker);

	return 0;
}

/* Populates the region [va, va + size) with pages by allocating pages from the
 * frame allocator and mapping them.
 */
void populate_region(struct page_table *pml4, void *va, size_t size,
	uint64_t flags)
{
	/* LAB 3: your code here. */
	struct populate_info info = {
		.flags = flags,
		.base = ROUNDDOWN((uintptr_t)va, PAGE_SIZE),
		.end = ROUNDUP((uintptr_t)va + size, PAGE_SIZE) - 1,
		.isuser_mem = pml4 == kernel_pml4,
	};

	struct page_walker walker = {
		.get_pte = populate_pte,
		.get_pde = populate_pde,
		.get_pdpte = ptbl_alloc,
		.get_pml4e = ptbl_alloc,
		.udata = &info,
	};

	walk_page_range(pml4, va, (void *)((uintptr_t)va + size), &walker);
}

