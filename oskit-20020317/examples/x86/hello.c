#include <stdio.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/version.h>

int main()
{
#ifndef KNIT
	oskit_clientos_init();
#endif
#ifdef  GPROF
	start_fs_bmod();
	start_gprof();
#endif
	oskit_print_version();
	printf("Hello, World\n");
        return 0;
}
