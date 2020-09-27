#include <types.h>

#include <kernel/mem.h>
#include <kernel/sched.h>
#include <kernel/vma.h>

#include <lib.h>

int sys_mquery(struct vma_info *info, void *addr)
{
	struct vma *vma;
	struct list *node;
	physaddr_t *entry;

	/* Check if the user has read/write access to the info struct. */
	assert_user_mem(cur_task, info, sizeof *info, PAGE_USER | PAGE_WRITE);

	/* Do not leak information about the kernel space. */
	if (addr >= (void *)USER_LIM) {
		return -1;
	}

	/* Clear the info struct. */
	memset(info, 0, sizeof *info);

	/* Find the VMA with an end address that is greater than the requested
	 * address, but also the closest to the requested address.
	 */
	vma = find_vma(NULL, NULL, &cur_task->task_rb, addr);

	if (!vma) {
		/* If there is no such VMA, it means the address is greater
		 * than the address of any VMA in the address space, i.e. the
		 * user is requesting the free gap at the end of the address
		 * space. The base address of this free gap is the end address
		 * of the highest VMA and the end address is simply USER_LIM.
		 */
		node = list_tail(&cur_task->task_mmap);

		info->vm_end = (void *)USER_LIM;

		if (!node) {
			return 0;
		}

		vma = container_of(node, struct vma, vm_mmap);
		info->vm_base = vma->vm_end;

		return 0;
	}

	if (addr < vma->vm_base) {
		/* The address lies outside the found VMA. This means the user
		 * is requesting the free gap between two VMAs. The base
		 * address of the free gap is the end address of the previous
		 * VMA. The end address of the free gap is the base address of
		 * the VMA that we found.
		 */
		node = list_prev(&cur_task->task_mmap, &vma->vm_mmap);

		info->vm_end = vma->vm_base;

		if (!node) {
			return 0;
		}

		vma = container_of(node, struct vma, vm_mmap);
		info->vm_base = vma->vm_end;

		return 0;
	}

	/* The requested address actually lies within a VMA. Copy the
	 * information.
	 */
	strncpy(info->vm_name, vma->vm_name, 64);
	info->vm_base = vma->vm_base;
	info->vm_end = vma->vm_end;
	info->vm_prot = vma->vm_flags;
	info->vm_type = vma->vm_src ? VMA_EXECUTABLE : VMA_ANONYMOUS;

	/* Check if the address is backed by a physical page. */
	if (page_lookup(cur_task->task_pml4, addr, &entry)) {
		info->vm_mapped = (*entry & PAGE_HUGE) ? VM_2M_PAGE : VM_4K_PAGE;
	}

	return 0;
}

void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd,
	uintptr_t offset)
{
	/* LAB 4: your code here. */
	struct vma *vma;
	int ret;
    uint64_t vm_flags;

    /* Do not leak information about the kernel space. */
    if (addr >= (void *)USER_LIM) {
        return MAP_FAILED;
    }

    // The only mappings currently supported are MAP ANONYMOUS | MAP PRIVATE.
    if ((flags & (MAP_ANONYMOUS | MAP_PRIVATE)) != (MAP_ANONYMOUS | MAP_PRIVATE)) {
        return MAP_FAILED;
    }

    // Some configurations of the protection flags are not supported on x86-64.
    //FIXME are these checks correct?
    if ((prot == PROT_WRITE) ||
        (prot == PROT_EXEC) ||
        ((prot & (PROT_WRITE | PROT_EXEC)) == (PROT_WRITE | PROT_EXEC))) {
        return MAP_FAILED;
    }

    if (flags & MAP_FIXED) {
        // If the user passes MAP FIXED, remove any previous mappings.
        sys_munmap(addr, len);
    }

    vm_flags = 0;
    vm_flags |= (prot & PROT_READ) ? VM_READ : 0;
    vm_flags |= (prot & PROT_WRITE) ? VM_WRITE : 0;
    vm_flags |= (prot & PROT_EXEC) ? VM_EXEC : 0;

    vma = add_vma(cur_task, "user", addr, len, vm_flags);

    if (vma != NULL) {
        if (flags & MAP_POPULATE) {
            ret = populate_vma_range(cur_task, ROUNDDOWN(addr, PAGE_SIZE), vma->vm_end - ROUNDDOWN(addr, PAGE_SIZE), vm_flags);
            if (ret < 0) {
                return MAP_FAILED;
            }
        }

        return ROUNDDOWN(vma->vm_base, PAGE_SIZE);
    }

	return MAP_FAILED;
}

void sys_munmap(void *addr, size_t len)
{
	/* LAB 4: your code here. */
	remove_vma_range(cur_task, addr, len);
}

int sys_mprotect(void *addr, size_t len, int prot)
{
	/* LAB 4 (bonus): your code here. */
	return -ENOSYS;
}

int sys_madvise(void *addr, size_t len, int advise)
{
	/* LAB 4 (bonus): your code here. */
	return -ENOSYS;
}

