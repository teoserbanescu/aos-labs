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

//#define DEBUG

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
    addr = sign_extend(base);
    end = sign_extend(end);
    addr_end = MIN(end, ptbl_end(addr));

    while (addr < end) {
        int i = PAGE_TABLE_INDEX(addr);
        physaddr_t *entry = &ptbl->entries[i];

#ifdef DEBUG
        cprintf("addr %p addr_end %p base %p end %p ptbl_walk_range\n", addr, addr_end, base, end);
#endif

        if (walker->get_pte) {
            ret = walker->get_pte(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if (walker->pte_hole && (!(*entry & PAGE_PRESENT))) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if (sign_extend(ptbl_end(addr)) == KERNEL_LIM)
            break;

        addr = sign_extend(ptbl_end(addr) + 1);
        addr_end = sign_extend(MIN(end, ptbl_end(addr)));
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
    addr = sign_extend(base);
    end = sign_extend(end);
    addr_end = MIN(end, pdir_end(addr));

    while (addr < end) {
        int i = PAGE_DIR_INDEX(addr);
        struct page_table *ptbl;
        physaddr_t *entry = &pdir->entries[i];

#ifdef DEBUG
        cprintf("addr %p addr_end %p base %p end %p pdir_walk_range\n", addr, addr_end, base, end);
#endif

        if (walker->get_pde) {
            ret = walker->get_pde(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        ptbl = (struct page_table *) KADDR(PAGE_ADDR(*entry));

        if (*entry & PAGE_PRESENT) {
            if (!(*entry & PAGE_HUGE)) {
                ret = ptbl_walk_range(ptbl, addr, addr_end, walker);
                if (ret < 0)
                    return ret;
                if (walker->unmap_pde) {
                    ret = walker->unmap_pde(entry, addr, addr_end, walker);
                    if (ret < 0)
                        return ret;
                }
            }
        }
        else if (walker->pte_hole) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }
        if (sign_extend(pdir_end(addr)) == KERNEL_LIM)
            break;

        addr = sign_extend(pdir_end(addr) + 1);
        addr_end = sign_extend(MIN(end, pdir_end(addr)));
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
    addr = sign_extend(base);
    end = sign_extend(end);
    addr_end = MIN(end, pdpt_end(addr));

    while (addr < end) {
        int i = PDPT_INDEX(addr);
        struct page_table *pdir;
        physaddr_t *entry = &pdpt->entries[i];

#ifdef DEBUG
        cprintf("addr %p addr_end %p base %p end %p pdpt_walk_range\n", addr, addr_end, base, end);
#endif

        if (walker->get_pdpte) {
            ret = walker->get_pdpte(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        pdir = (struct page_table *) KADDR(PAGE_ADDR(*entry));

        if (*entry & PAGE_PRESENT) {
            if (!(*entry & PAGE_HUGE)) {
                ret = pdir_walk_range(pdir, addr, addr_end, walker);
                if (ret < 0)
                    return ret;
                if (walker->unmap_pdpte) {
                    ret = walker->unmap_pdpte(entry, addr, addr_end, walker);
                    if (ret < 0)
                        return ret;
                }
            }
        }
        else if (walker->pte_hole) {
            ret = walker->pte_hole(addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        if (sign_extend(pdpt_end(addr)) == KERNEL_LIM)
                break;
        addr = sign_extend(pdpt_end(addr) + 1);
        addr_end = sign_extend(MIN(end, pdpt_end(addr)));
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
//    cprintf("base %p end %p pml4_walk_range\n", base, end);

	/* LAB 2: your code here. */
	int ret;
    uintptr_t addr, addr_end;
    addr = sign_extend(base);
    end = sign_extend(end);
    addr_end = MIN(end, pml4_end(addr));


    while (addr < end) {
        int i = PML4_INDEX(addr);
        struct page_table *pdpt;
        physaddr_t *entry = &pml4->entries[i];

#ifdef DEBUG
        cprintf("\naddr %p addr_end %p base %p end %p pml4_walk_range\n", addr, addr_end, base, end);
#endif
        if (walker->get_pml4e) {
            ret = walker->get_pml4e(entry, addr, addr_end, walker);
            if (ret < 0)
                return ret;
        }

        pdpt = (struct page_table *) KADDR(PAGE_ADDR(*entry));

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

        if (sign_extend(pml4_end(addr)) == KERNEL_LIM)
            break;
        addr = sign_extend(pml4_end(addr) + 1);
        addr_end = sign_extend(MIN(end, pml4_end(addr)));
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

