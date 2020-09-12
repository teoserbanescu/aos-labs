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
    int ret;
    uintptr_t addr, addr_end;

    addr = base;
    addr_end = ptbl_end(addr);

    for (int i = PAGE_TABLE_INDEX(base); i <= PAGE_TABLE_INDEX(end); ++i) {
        physaddr_t *entry = &ptbl->entries[i];

        if (walker->get_pte) {
            ret = walker->get_pte(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if (walker->pte_hole &&
            (!(*entry & PAGE_PRESENT)) // should we check !entry ?
                ) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }
    }
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
    int ret;
    uintptr_t addr, addr_end;

    addr = base;
    addr_end = pdir_end(addr);

    for (int i = PAGE_DIR_INDEX(base); i <= PAGE_DIR_INDEX(end); ++i) {
        struct page_table *ptbl;
        physaddr_t *entry = &pdir->entries[i];


        if (walker->get_pde) {
            ret = walker->get_pde(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        ptbl = (struct page_table *) KADDR(PAGE_ADDR(*entry));

        if (walker->pte_hole &&
            (!(*entry & PAGE_PRESENT)) // should we check !entry ?
                ) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if ((*entry & PAGE_PRESENT) &&
            !(*entry & PAGE_HUGE)
                ) { // should we check entry ?
            ret = ptbl_walk_range(ptbl, addr, addr_end, walker);
            if (ret < 0)
                return ret;
            if (walker->unmap_pde) {
                ret = walker->unmap_pde(entry, addr, addr_end, walker);
                if (ret < 0)
                    return ret;
            }
        }

        addr = addr_end + 1;
        addr_end = pdir_end(addr);
    }

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
    int ret;
    uintptr_t addr, addr_end;

    addr = base;
    addr_end = pdpt_end(addr);

    for (int i = PDPT_INDEX(base); i <= PDPT_INDEX(end); ++i) {
        struct page_table *pdir;
        physaddr_t *entry = &pdpt->entries[i];

        ptbl_alloc(entry, addr, addr_end, walker);
        pdir = (struct page_table *) KADDR(PAGE_ADDR(*entry));

        if (walker->get_pdpte) {
            ret = walker->get_pdpte(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if (walker->pte_hole &&
            (!(*entry & PAGE_PRESENT)) // should we check !entry ?
                ) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if ((*entry & PAGE_PRESENT) &&
            !(*entry & PAGE_HUGE)
            ) {
            ret = pdir_walk_range(pdir, addr, addr_end, walker);
            if (ret < 0)
                return ret;
            if (walker->unmap_pdpte) {
                ret = walker->unmap_pdpte(entry, addr, addr_end, walker);
                if (ret < 0)
                    return ret;
            }
        }

        addr = addr_end + 1;
        addr_end = pdpt_end(addr);
    }
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
    uintptr_t addr, addr_end;
    addr = base;
    addr_end = pml4_end(addr);

    for (int i = PML4_INDEX(base); i <= PML4_INDEX(end); ++i) {
        struct page_table *pdpt;
        physaddr_t *entry = &pml4->entries[i];

        ptbl_alloc(entry, addr, addr_end, walker);
        pdpt = (struct page_table *) KADDR(PAGE_ADDR(*entry));

        if (walker->get_pml4e) {
            ret = walker->get_pml4e(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if (*entry & PAGE_PRESENT) { // should we check entry ?
            ret = pdpt_walk_range(pdpt, addr, addr_end, walker);
            if (ret < 0)
                return ret;
            if (walker->unmap_pml4e) {
                ret = walker->unmap_pml4e(entry, addr, addr_end, walker);
                if (ret < 0)
                    return ret;
            }
        }
        else if (walker->pte_hole) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        addr = addr_end + 1;
        addr_end = pml4_end(addr);
    }

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

