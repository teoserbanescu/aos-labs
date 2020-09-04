#include <types.h>
#include <list.h>
#include <paging.h>
#include <string.h>

#include <kernel/mem.h>

/* Physical page metadata. */
size_t npages;
struct page_info *pages;

/* Lists of physical pages. */
struct list page_free_list[BUDDY_MAX_ORDER];

/* Counts the number of free pages for the given order.
 */
size_t count_free_pages(size_t order)
{
	struct list *node;
	size_t nfree_pages = 0;

	if (order >= BUDDY_MAX_ORDER) {
		return 0;
	}
//    static int x = 0;

	list_foreach(page_free_list + order, node) {
//        cprintf("%d\n", ++x);
		++nfree_pages;
	}

	return nfree_pages;
}

/* Shows the number of free pages in the buddy allocator as well as the amount
 * of free memory in kiB.
 *
 * Use this function to diagnose your buddy allocator.
 */
void show_buddy_info(void)
{
	struct page_info *page;
	struct list *node;
	size_t order;
	size_t nfree_pages;
	size_t nfree = 0;

	cprintf("Buddy allocator:\n");

	for (order = 0; order < BUDDY_MAX_ORDER; ++order) {
		nfree_pages = count_free_pages(order);

		cprintf("  order #%u pages=%u\n", order, nfree_pages);

		nfree += nfree_pages * (1 << (order + 12));
	}

	cprintf("  free: %u kiB\n", nfree / 1024);
}

/* Gets the total amount of free pages. */
size_t count_total_free_pages(void)
{
	struct page_info *page;
	struct list *node;
	size_t order;
	size_t nfree_pages;
	size_t nfree = 0;

	for (order = 0; order < BUDDY_MAX_ORDER; ++order) {
		nfree_pages = count_free_pages(order);
		nfree += nfree_pages * (order + 1);
	}

	return nfree;
}

/* Splits lhs into free pages until the order of the page is the requested
 * order req_order.
 *
 * The algorithm to split pages is as follows:
 *  - Given the page of order k, locate the page and its buddy at order k - 1.
 *  - Decrement the order of both the page and its buddy.
 *  - Mark the buddy page as free and add it to the free list.
 *  - Repeat until the page is of the requested order.
 *
 * Returns a page of the requested order.
 */
 struct page_info *buddy_split(struct page_info *lhs, size_t req_order)
{
	/* LAB 1: your code here. */

    /* Node - there will be no buddy between order k and req_order, because otherwise
     * it would have been returned by buddy_find(..)
     */
    struct page_info *page, *buddy_page;
    struct list *node;

    page = NULL;

    list_foreach(page_free_list + req_order, node) {
        page = container_of(node, struct page_info, pp_node);
    }

    page->pp_order -= 1;
    list_pop(page_free_list + req_order);
    list_push(page_free_list + req_order, node);

    buddy_page = page + (1 << page->pp_order);
    buddy_page->pp_order -= 1;
    buddy_page->pp_free = 1;
    list_push(page_free_list + buddy_page->pp_order, &buddy_page->pp_node);

    if (page->pp_order != req_order) {
        page = buddy_split(page, req_order);
    }

	return page;
}

/* Merges the buddy of the page with the page if the buddy is free to form
 * larger and larger free pages until either the maximum order is reached or
 * no free buddy is found.
 *
 * The algorithm to merge pages is as follows:
 *  - Given the page of order k, locate the page with the lowest address
 *    and its buddy of order k.
 *  - Check if both the page and the buddy are free and whether the order
 *    matches.
 *  - Remove the page and its buddy from the free list.
 *  - Increment the order of the page.
 *  - Repeat until the maximum order has been reached or until the buddy is not
 *    free.
 *
 * Returns the largest merged free page possible.
 */
struct page_info *buddy_merge(struct page_info *page)
{
    static int i = 0;
    cprintf("iter %d \n", i++);
	/* LAB 1: your code here. */
    struct list *head, *node;
    struct page_info *buddy_page, *next_order;

    /*  if page is the 2 or 3rd consecutive with the same order => it is not the head, respect de algorithm
     *  investigate: can it be the 3rd?
     */
    head = list_head(page_free_list + page->pp_order);
    cprintf("head pointer %p \n", *head);
    if (head == NULL)
        return NULL;
    page = container_of(head, struct page_info, pp_node);
    cprintf("head order %d \n", page->pp_order);

    if (!page->pp_free)
        return NULL;

//    in case of bigger pages than PAGESIZE the buddy is not page + 1
    node = list_next(page_free_list + page->pp_order, head);
    cprintf("buddy pointer %p \n", *node);

    if (node == NULL)
        return NULL;
    buddy_page = container_of(node, struct page_info, pp_node);
    cprintf("buddy order %d \n", buddy_page->pp_order);

    if (!buddy_page->pp_free || page->pp_order != buddy_page->pp_order)
        return NULL;

//  remove page
    list_pop_left(page_free_list + page->pp_order);
//  remove buddy
    list_pop_left(page_free_list + page->pp_order);

    page->pp_order++;
    buddy_page->pp_order++;

    if (page->pp_order == BUDDY_MAX_ORDER)
        return page;

//    get the last one from the previous order
//    list_foreach(page_free_list + page->pp_order, node) {
//        next_order = container_of(node, struct page_info, pp_node);
//    }
    list_push(page_free_list + page->pp_order, &page->pp_node);
    next_order = buddy_merge(buddy_page);
    if (next_order) {
        return next_order;
    }
    else {
        list_pop(page_free_list + page->pp_order);
        return page;
    }
}

/* Given the order req_order, attempts to find a page of that order or a larger
 * order in the free list. In case the order of the free page is larger than the
 * requested order, the page is split down to the requested order using
 * buddy_split().
 *
 * Returns a page of the requested order or NULL if no such page can be found.
 */
struct page_info *buddy_find(size_t req_order)
{
	/* LAB 1: your code here. */

    struct list *node;
    struct page_info *page, *big_page;

    page = NULL;
    big_page = NULL;

    if (req_order >= BUDDY_MAX_ORDER) {
        return page;
    }

//    find exact order
    list_foreach(page_free_list + req_order, node) {
        page = container_of(node, struct page_info, pp_node);
        return page;
    }

    for (size_t order = req_order + 1; order < BUDDY_MAX_ORDER; ++order){
        list_foreach(page_free_list + req_order, node) {
            big_page = container_of(node, struct page_info, pp_node);
        }
        if (big_page) {
            page = buddy_split(big_page, req_order);
            return page;
        }
    }

    return page;
}

/*
 * Allocates a physical page.
 *
 * if (alloc_flags & ALLOC_ZERO), fills the entire returned physical page with
 * '\0' bytes.
 * if (alloc_flags & ALLOC_HUGE), returns a huge physical 2M page.
 *
 * Beware: this function does NOT increment the reference count of the page -
 * this is the caller's responsibility.
 *
 * Returns NULL if out of free memory.
 *
 * Hint: use buddy_find() to find a free page of the right order.
 * Hint: use page2kva() and memset() to clear the page.
 */
struct page_info *page_alloc(int alloc_flags)
{
	/* LAB 1: your code here. */
    struct page_info *page;
    size_t req_order;

    page = buddy_find(alloc_flags);

    if (!page) {
        panic("out of free memory");
    }

    if (alloc_flags & ALLOC_ZERO) {
        memset(page, '\0', 1 << alloc_flags);
    }

	return page;
}

/*
 * Return a page to the free list.
 * (This function should only be called when pp->pp_ref reaches 0.)
 *
 * Hint: mark the page as free and use buddy_merge() to merge the free page
 * with its buddies before returning the page to the free list.
 */
void page_free(struct page_info *pp)
{
	/* LAB 1: your code here. */
    struct page_info *page;

	pp->pp_free = 1;
    list_push(page_free_list + pp->pp_order, &pp->pp_node);
//    show_buddy_info();

    page = buddy_merge(pp);

    if (page) {
        list_push(page_free_list + page->pp_order, &page->pp_node);
//        cprintf("#########merged#######\n");
//        show_buddy_info();
    }
}

/*
 * Decrement the reference count on a page,
 * freeing it if there are no more refs.
 */
void page_decref(struct page_info *pp)
{
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

