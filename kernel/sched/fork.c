#include <cpu.h>
#include <error.h>
#include <list.h>

#include <kernel/console.h>
#include <kernel/mem.h>
#include <kernel/monitor.h>
#include <kernel/sched.h>
#include <kernel/vma.h>
#include <kernel/sched/task.h>

//extern struct list runq;

#define ENTRY_FLAGS(x) (((physaddr_t)x) & PAGE_MASK)

static inline void dup_entry(physaddr_t *entry_dst, physaddr_t *entry_src) {
	struct page_info *page;

	page = page_alloc(ALLOC_ZERO);
	page->pp_ref++;
	*entry_dst = PAGE_ADDR(page2pa(page)) | ENTRY_FLAGS(*entry_src);
}

static int copy_ptbl(struct page_table *dst_pte, struct page_table *src_pte)
{
	struct page_info *page;

	for (int i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		if (src_pte->entries[i]) {
			dst_pte->entries[i] = src_pte->entries[i] & ~PAGE_WRITE;
			src_pte->entries[i] &= ~PAGE_WRITE;
			page = pa2page(PAGE_ADDR(src_pte->entries[i]));
			page->pp_ref++;
		}
	}

	return 0;
}

// FIXME do something with huge pages ....
static int copy_pdir(struct page_table *dst_pdir, struct page_table *src_pdir)
{
	for (int i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		if (src_pdir->entries[i]) {
			dup_entry((physaddr_t *)&dst_pdir->entries[i],
					  (physaddr_t *)&src_pdir->entries[i]);
			copy_ptbl(entry2page_table(dst_pdir->entries[i]),
					  entry2page_table(src_pdir->entries[i]));
		}
	}

	return 0;
}

static int copy_pdpt(struct page_table *dst_pdpt, struct page_table *src_pdpt)
{
	for (int i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		if (src_pdpt->entries[i]) {
			dup_entry((physaddr_t *)&dst_pdpt->entries[i],
					  (physaddr_t *)&src_pdpt->entries[i]);
			copy_pdir(entry2page_table(dst_pdpt->entries[i]),
				entry2page_table(src_pdpt->entries[i]));
		}
	}

	return 0;
}

static int copy_pml4(struct page_table *dst_pml4, struct page_table *src_pml4)
{
	struct page_info *page;
	int i;

	for (i = 0; i < PML4_INDEX(USER_LIM); ++i) {
		if (src_pml4->entries[i]) {
			dup_entry((physaddr_t *)&dst_pml4->entries[i],
						(physaddr_t *)&src_pml4->entries[i]);

			copy_pdpt(entry2page_table(dst_pml4->entries[i]),
					entry2page_table(src_pml4->entries[i]));
		}
	}

	return 0;
}


static int copy_vma(struct task *task_dst, struct task *task_src) {
	struct vma *vma_dst, *vma_src;
	struct list *node;

	list_foreach(&task_src->task_mmap, node) {
		vma_src = container_of(node, struct vma, vm_mmap);

		vma_dst = kmalloc(sizeof(struct vma));
		vma_dst->vm_name	= vma_src->vm_name;
		vma_dst->vm_base	= vma_src->vm_base;
		vma_dst->vm_end		= vma_src->vm_end;
		vma_dst->vm_src		= vma_src->vm_src;
		vma_dst->vm_len		= vma_src->vm_len;
		vma_dst->vm_flags	= vma_src->vm_flags;
		list_init(&vma_dst->vm_mmap);
		rb_node_init(&vma_dst->vm_rb);

		if (insert_vma(task_dst, vma_dst)) {
			cprintf("%s insert_vma failed", __PRETTY_FUNCTION__ );
			return -1;
		}
	}

	return 0;
}

/* Allocates a task struct for the child process and copies the register state,
 * the VMAs and the page tables. Once the child task has been set up, it is
 * added to the run queue.
 */
struct task *task_clone(struct task *source)
{
	/* LAB 5: your code here. */
	/* discussion https://canvas.vu.nl/courses/49330/discussion_topics/294226
	 * I think it's easier to have individual page tables per process.
	 * If you share them, you might encounter some tricky cases later when you implement swapping or shared pages.
	 * Similarly for the VMAs, you need to make deep copies,
	 * since changes in the parent's VMAs should definitely not be visible in the child.
	 * */

	struct task *clone;

	/* Allocate a new task struct. */
	clone = task_alloc(source->task_pid);

	if (!clone) {
		return NULL;
	}

	clone->task_frame = source->task_frame;
	clone->task_frame.rax = 0;

	/* Clone the page tables. */
	if (copy_pml4(clone->task_pml4, source->task_pml4) < 0)
		goto out_free;

	if (copy_vma(clone, source) < 0)
		goto out_free;

	return clone;

out_free:
	kfree(clone);
	return NULL;
}

pid_t sys_fork(void)
{
	/* LAB 5: your code here. */
	struct task *clone;
//	cprintf("[PID %5u] %s\n", cur_task ? cur_task->task_pid : 0, __PRETTY_FUNCTION__ );

	clone = task_clone(cur_task);
	if (clone == NULL) {
		panic("NULL clone");
	}

	if (clone->task_type == TASK_TYPE_USER) {
		extern int nuser_tasks;
		nuser_tasks++;
	}

	list_init(&clone->task_node);
//	list_push(&runq, &clone->task_node);
	insert_task(clone);
//	list_push_left(&runq, &clone->task_node);
	list_push(&cur_task->task_children, &clone->task_child);

	return clone->task_pid;
}

