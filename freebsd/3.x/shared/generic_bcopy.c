#include <string.h>

/*
 * XXX This should probably just be a #define.
 */
void
generic_bcopy(const void *src, void *dst, size_t len)
{
	bcopy(src, dst, len);
}

void
generic_bzero(void *b, size_t len)
{	
	bzero(b, len);
}
