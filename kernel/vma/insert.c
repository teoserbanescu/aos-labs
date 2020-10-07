#include <types.h>

#include <kernel/mem.h>
#include <kernel/vma.h>

/* Inserts the given VMA into the red-black tree of the given task. First tries
 * to find a VMA for the end address of the given end address. If there is
 * already a VMA that overlaps, this function returns -1. Then the VMA is
 * inserted into the red-black tree and added to the sorted linked list of
 * VMAs.
 */
int insert_vma(struct task *task, struct vma *vma)
{
	struct rb_node *rb_parent = NULL;
	struct list *node;
	struct vma *found, *parent;
	int dir;

	found = find_vma(&rb_parent, &dir, &task->task_rb, vma->vm_end);

	if (found && found->vm_base < vma->vm_end) {
		return -1;
	}

	parent = rb_parent ? container_of(rb_parent, struct vma, vm_rb) : NULL;
	node = &parent->vm_mmap;

	if (!parent) {
		task->task_rb.root = &vma->vm_rb;
	} else {
		rb_parent->child[dir] = &vma->vm_rb;
		vma->vm_rb.parent = rb_parent;
	}

	if (rb_insert(&task->task_rb, &vma->vm_rb) < 0) {
		return -1;
	}

	if (!parent) {
		list_insert_before(&task->task_mmap, &vma->vm_mmap);
	} else {
		if (dir) {
			list_insert_before(node, &vma->vm_mmap);
		} else { 
			list_insert_after(node, &vma->vm_mmap);
		}
	}

	return 0;
}

/* Allocates and adds a new VMA for the given task.
 *
 * This function first allocates a new VMA. Then it copies over the given
 * information. The VMA is then inserted into the red-black tree and linked
 * list. Finally, this functions attempts to merge the VMA with the adjacent
 * VMAs.
 *
 * Returns the new VMA if it could be added, NULL otherwise.
 */
struct vma *add_executable_vma(struct task *task, char *name, void *addr,
	size_t size, int flags, void *src, size_t len)
{
	/* LAB 4: your code here. */
    struct vma *vma;

    vma = kmalloc(sizeof(*vma));
    vma->vm_name = name;
    list_init(&vma->vm_mmap);
    vma->vm_base = addr;
    vma->vm_end = ROUNDUP(addr + size, PAGE_SIZE);
    vma->vm_src = src;
    vma->vm_len = len;
    vma->vm_flags = flags;

    if (insert_vma(task, vma) == -1) {
        cprintf("insert_vma: There is already a VMA that overlaps");
        return NULL;
    }

    return merge_vmas(task, vma);
}

/* A simplified wrapper to add anonymous VMAs, i.e. VMAs not backed by an
 * executable.
 */
struct vma *add_anonymous_vma(struct task *task, char *name, void *addr,
	size_t size, int flags)
{
	return add_executable_vma(task, name, addr, size, flags, NULL, 0);
}

/* Allocates and adds a new VMA to the requested address or tries to find a
 * suitable free space that is sufficiently large to host the new VMA. If the
 * address is NULL, this function scans the address space from the end to the
 * beginning for such a space. If an address is given, this function scans the
 * address space from the given address to the beginning and then scans from
 * the end to the given address for such a space.
 *
 * Returns the VMA if it could be added. NULL otherwise.
 */
struct vma *add_vma(struct task *task, char *name, void *addr, size_t size,
	int flags)
{
	/* LAB 4: your code here. */
	struct list *node, *prev;
	struct vma *vma, *vma_prev;

	if (addr)
	{
//		end address greater than the requested address
		struct vma *vma = find_vma(NULL, NULL, &task->task_rb, addr);
		if(vma != NULL) {
//			check if we do not use space from the left neighbour
			if(vma->vm_base >= addr + size) {
                return add_anonymous_vma(task, name, addr, size, flags);
			}
		}
		else {
//			check if we do not use space from the right neighbour
			vma = find_vma(NULL, NULL, &task->task_rb, addr + size);
			if(vma->vm_base >= addr + size) {
                return add_anonymous_vma(task, name, addr, size, flags);
            }
		}
//			we can not allocate at this addr because neighbour uses this space
//			try to find space at any addr as we would if we were not given an addr
		addr = NULL;
	}

//		The last vma should be the stack which is at the and of the user space
	list_foreach_safe_rev(&task->task_mmap, node, prev) {
		vma = container_of(node, struct vma, vm_mmap);
		vma_prev = container_of(prev, struct vma, vm_mmap);

		if(vma->vm_base >= vma_prev->vm_end + size) {
			addr = vma->vm_base-size;
			break;
		}
	}

	if (addr)
		return add_anonymous_vma(task, name, addr, size, flags);

	return NULL;
}

