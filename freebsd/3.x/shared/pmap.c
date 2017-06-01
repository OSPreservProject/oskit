#include <sys/types.h>

void *
pmap_mapdev(pa, size)
        vm_offset_t pa;
        vm_size_t size;
{
	return pa;
}


/*
 * This does nothing in FreeBSD, but is intended to set write combining
 * on video memory for P6 class machines.
 */
void
pmap_setvidram(void)
{
}
