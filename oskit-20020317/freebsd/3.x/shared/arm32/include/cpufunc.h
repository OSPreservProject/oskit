/*
 * Stub
 */
#include <sys/cdefs.h>
#include <sys/types.h>
#include <machine/ipl.h>

void    insque          __P((void *a, void *b));
void    remque          __P((void *a));

static __inline void
setbits(volatile u_int *addr, u_int bits)
{
	*addr |= bits;
}
