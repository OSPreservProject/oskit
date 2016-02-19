#include <sys/param.h>
#include <sys/systm.h>

/*
 * Three routines to handle adding/deleting items on the
 * shutdown callout lists
 *
 * at_shutdown():
 * Take the arguments given and put them onto the shutdown callout list.
 * However first make sure that it's not already there.
 * returns 0 on success.
 */
int
at_shutdown(bootlist_fn function, void *arg, int queue)
{
	printf("%s: called for function %x, arg %x\n", __FUNCTION__,
	       function, arg);

	return 0;
}
