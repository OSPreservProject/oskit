#include <oskit/dev/dev.h>

int c;
void mem_lock()
{
	if (c++ == 0)
		osenv_intr_disable();
}

void mem_unlock()
{
	if (--c == 0)
		osenv_intr_enable();
}

void mem_lock_init(void)
{
}
