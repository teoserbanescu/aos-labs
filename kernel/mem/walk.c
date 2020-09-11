#include <types.h>
#include <paging.h>

#include <kernel/mem.h>

/* Given an address addr, this function returns the sign extended address. */
static uintptr_t sign_extend(uintptr_t addr)
{
	return (addr < USER_LIM) ? addr : (0xffff000000000000ull | addr);
}

/* Given an addresss addr, this function returns the page boundary. */
static uintptr_t ptbl_end(uintptr_t addr)
{
	return addr | (PAGE_SIZE - 1);
}

/* Given an address addr, this function returns the page table boundary. */
static uintptr_t pdir_end(uintptr_t addr)
{
	return addr | (PAGE_TABLE_SPAN - 1);
}

/* Given an address addr, this function returns the page directory boundary. */
static uintptr_t pdpt_end(uintptr_t addr)
{
	return addr | (PAGE_DIR_SPAN - 1);
}

/* Given an address addr, this function returns the PDPT boundary. */
static uintptr_t pml4_end(uintptr_t addr)
{
	return addr | (PDPT_SPAN - 1);
}

/* Walks over the page range from base to end iterating over the entries in the
 * given page table ptbl. The user may provide walker->get_pte() that gets
 * called for every entry in the page table. In addition the user may provide
 * walker->pte_hole() that gets called for every unmapped entry in the page
 * table.
 *
 * Hint: this function calls ptbl_end() to get the end boundary of the current
 * page.
 * Hint: the next page is at ptbl_end() + 1.
 * Hint: the loop condition is next < end.
 */
static int ptbl_walk_range(struct page_table *ptbl, uintptr_t base,
    uintptr_t end, struct page_walker *walker)
{
	/* LAB 2: your code here. */
	return 0;
}

/* Walks over the page range from base to end iterating over the entries in the
 * given page directory pdir. The user may provide walker->get_pde() that gets
 * called for every entry in the page directory. In addition the user may
 * provide walker->pte_hole() that gets called for every unmapped entry in the
 * page directory. If the PDE is present, but not a huge page, this function
 * calls ptbl_walk_range() to iterate over the entries in the page table. The
 * user may provide walker->unmap_pde() that gets called for every present PDE
 * after walking over the page table.
 *
 * Hint: see ptbl_walk_range().
 */
static int pdir_walk_range(struct page_table *pdir, uintptr_t base,
    uintptr_t end, struct page_walker *walker)
{
	/* LAB 2: your code here. */
	return 0;
}

/* Walks over the page range from base to end iterating over the entries in the
 * given PDPT pdpt. The user may provide walker->get_pdpte() that gets called
 * for every entry in the PDPT. In addition the user may provide
 * walker->pte_hole() that gets called for every unmapped entry in the PDPT. If
 * the PDPTE is present, but not a large page, this function calls
 * pdir_walk_range() to iterate over the entries in the page directory. The
 * user may provide walker->unmap_pdpte() that gets called for every present
 * PDPTE after walking over the page directory.
 *
 * Hint: see ptbl_walk_range().
 */
static int pdpt_walk_range(struct page_table *pdpt, uintptr_t base,
    uintptr_t end, struct page_walker *walker)
{
	/* LAB 2: your code here. */
	return 0;
}

/* Walks over the page range from base to end iterating over the entries in the
 * given PML4 pml4. The user may provide walker->get_pml4e() that gets called
 * for every entry in the PML4. In addition the user may provide
 * walker->pte_hole() that gets called for every unmapped entry in the PML4. If
 * the PML4E is present, this function calls pdpt_walk_range() to iterate over
 * the entries in the PDPT. The user may provide walker->unmap_pml4e() that
 * gets called for every present PML4E after walking over the PDPT.
 *
 * Hint: see ptbl_walk_range().
 */
static int pml4_walk_range(struct page_table *pml4, uintptr_t base, uintptr_t end,
    struct page_walker *walker)
{
	/* LAB 2: your code here. */

	int ret;

    uintptr_t addr_end;
    for (int i = 0; i < 512; ++i) {
//    for (uintptr_t addr = base; addr < end; addr = addr_end + 1) {
//        cprintf("working\n");

//        physaddr_t *entry = &pml4->entries[PML4_INDEX(addr)];
        physaddr_t *entry = &pml4->entries[i];

        pdpt_walk_range((struct page_table *)entry, base, end, walker);
        if (!entry)
            return 0;
//        if (!entry)
//            return 1;
//        if (walker->get_pml4e) {
//    	    pdpt_walk_range(entry, base, end, walker);
//                if (walker->unmap_pml4e)
//                    walker->unmap_pml4e(entry, base, end, walker);
//        }

//        addr_end = ptbl_end(addr);
//        if (walker->get_pml4e) {
//            ret = walker->get_pml4e(entry, addr, addr_end, walker);
//            if (ret < 0)
//                return ret;
//        }
//        if (walker->pte_hole) {
//            ret = walker->pte_hole(base, end, walker);
//            if (ret < 0)
//                return ret;
//        }
//        panic("%p\n", entry);

    }

//	    if (!entry) {
//            entry = page2pa(page_alloc(0));
//            if (!entry)
//                return 0;
//        }
//        if (walker->get_pml4e) {
//    	    pdpt_walk_range(entry, base, end, walker);
//                if (walker->unmap_pml4e)
//                    walker->unmap_pml4e(entry, base, end, walker);
//        }
/*
 * The first level is called PML4 and the upper 9 bits of the 36 bits bits are used to find the
 * offset in the PML4 directory. The entry at that offset will point to the second level (or page directory
 * pointer (PDPT))
 * */

//	int entry = 0x109000 >>
//	for (int i = 0; i < 512; ++i) {
//	    if (!pml4->entries[i] & PAGE_PRESENT)
//		    pml4->entries[i] = page2pa(page_alloc(ALLOC_ZERO | PAGE_PRESENT));
////        pml4->
//	}
//	if (!pml4->entries[base] & PAGE_PRESENT) {
//		pml4->entries[base] = page2pa(page_alloc(ALLOC_ZERO));
//	}
//	for (uintptr_t it = base; it < end; ++it) {
//
//	}
	return 0;
}

/* Helper function to walk over a page range starting at base and ending before
 * end.
 */
int walk_page_range(struct page_table *pml4, void *base, void *end,
	struct page_walker *walker)
{
	return pml4_walk_range(pml4, ROUNDDOWN((uintptr_t)base, PAGE_SIZE),
		ROUNDUP((uintptr_t)end, PAGE_SIZE) - 1, walker);
}

/* Helper function to walk over all pages. */
int walk_all_pages(struct page_table *pml4, struct page_walker *walker)
{
	return pml4_walk_range(pml4, 0, KERNEL_LIM, walker);
}

/* Helper function to walk over all user pages. */
int walk_user_pages(struct page_table *pml4, struct page_walker *walker)
{
	return pml4_walk_range(pml4, 0, USER_LIM, walker);
}

/* Helper function to walk over all kernel pages. */
int walk_kernel_pages(struct page_table *pml4, struct page_walker *walker)
{
	return pml4_walk_range(pml4, KERNEL_VMA, KERNEL_LIM, walker);
}

