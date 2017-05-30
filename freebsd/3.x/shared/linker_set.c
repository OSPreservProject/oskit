#include <sys/param.h>
#include <sys/linker_set.h>
#include <oskit/dev/dev.h>    /* for osenv_mem_{alloc,free} */
#include <oskit/c/strings.h>  /* for bcopy */

void
freebsd_linker_set_add(struct linker_set *set, void *sym)
{
	void **new_items;

	new_items = (void *)osenv_mem_alloc((set->ls_length + 2) * sizeof(void *), 0, 0);

#if 0
	printf("in %s: ",__FUNCTION__);
	printf("add %x to %x (%d entries)\n", sym, set, set->ls_length);
#endif

	bcopy(set->ls_items, new_items, set->ls_length * sizeof(void *));
	new_items[set->ls_length] = sym;
	set->ls_length++;
	new_items[set->ls_length] = NULL;

	if (set->ls_items)
		osenv_mem_free(set->ls_items, 0, 
			       set->ls_length * sizeof(void *));

#if 0
	printf("set->ls_items is %x; new_items is %x\n", set->ls_items, 
	       new_items);
#endif

	set->ls_items = new_items;
}
