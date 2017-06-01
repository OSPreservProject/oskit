#include "misc.h"

#include <stdio.h>
#include <assert.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/freebsd.h>
#include <oskit/net/freebsd.h>


int
xf86ProcessArgument (int argc, char *argv[], int i)
{
	/* No real oskit specific command line arguments... */

	return 0;
}


void
xf86UseMsg(void)
{
	return;
}


void
xf86OpenConsole(void)
{
	xf86Config(FALSE);
}

void
xf86CloseConsole(void)
{
}
