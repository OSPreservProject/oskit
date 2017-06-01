#include <sys/types.h>

#include "osenv.h"

u_long
kvtop(void *addr)
{
        return (u_long)osenv_mem_get_phys((oskit_addr_t)addr);
}
