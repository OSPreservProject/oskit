/*
 * Copyright (c) 2000, 2001 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/console.h>
#include <oskit/dev/dev.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/boot/bootwhat.h>

#include <oskit/machine/reset.h>
#include <oskit/machine/ofw/ofw.h>
#include <oskit/machine/ofw/ofw_cons.h>

#include "decls.h"

/* Forward decls */
static void		boot_system(void);
static void		boot_default(void);
static void		boot_net(char *kernel, const char *cmdline);
static void		boot_interactive(void);

int
main(int argc, char **argv)
{
	oskit_error_t err;

	oskit_clientos_init();
	printf("\nOFW boot program (compiled %s)\n", __DATE__);

	osenv_process_lock();
	start_net_devices();
	osenv_process_unlock();
	err = net_init();
	if (err)
		panic("Could not initialize network");
	bootinfo_init();

	printf("DHCP Info: IP: %s, Server: %s, Gateway: %s\n",
	       ipaddr, server, gateway);

	boot_system();

	arm32_reset();
	return 0;
}

static void
boot_system(void)
{
	boot_info_t		boot_info;
	boot_what_t	       *boot_whatp;
	int			success, i;

	/*
	 * Give the user a few seconds to interrupt and go into
	 * interactive mode. If we return, go through the auto
	 * boot and hope it works.
	 */
	printf("Type a key for interactive mode (quick, quick!)\n");
	for (i = 0; i < 500; i++) {
		if (console_trygetchar() < 0)
			delay(10);
		else {
			boot_interactive();
			break;
		}
	}

	/*
	 * Try 5 times.
	 */
	success = 0;
	for (i = 5; i > 0; i--) {
		memset(&boot_info, 0, sizeof(boot_info));
		if (bootinfo_request(&boot_info) == 0)
			break;
	}
	if (i == 0) {
		printf("No reply from server\n");
		goto defboot;
	}
	if (boot_info.status == BISTAT_FAIL) {
		printf("No bootinfo for us on server\n");
		goto defboot;
	}

	/*
	 * Got a seemingly valid boot info packet that says what to do.
	 * Ack it and do it!
	 */
	bootinfo_ack();
	boot_whatp = (boot_what_t *)&boot_info.data;
	switch (boot_whatp->type) {
	case BIBOOTWHAT_TYPE_MB:
		printf("Rebooting with kernel: %s:%s\n",
		       server, boot_whatp->what.mb.filename);
		boot_net(boot_whatp->what.mb.filename, boot_whatp->cmdline);
		break;
		
	case BIBOOTWHAT_TYPE_PART:
		printf("Cannot boot from partitions\n");
		break;

	case BIBOOTWHAT_TYPE_SYSID:
		printf("Cannot boot using sysid\n");
		break;

	default:
		printf("Invalid bootinfo response: %d\n", boot_whatp->type);
		break;
	}

	/*
	 * Well, the only way to get here is if the specified boot failed.
	 * Fall through to booting the default, and hope it works.
	 */
 defboot:
	boot_default();
}

/*
 * Tell OFW to boot the default kernel
 */
static void
boot_default(void)
{
	char buf[512];

	strncpy(buf, DEF_KERNEL, sizeof buf);
	printf("Rebooting with default kernel: %s:%s\n", server, buf);

	boot_net(buf, 0);
}

/*
 * Convert kernel and command line to format acceptible by OFW
 * and then call OF_boot.
 */
static void
boot_net(char *kernel, const char *cmdline)
{
	char buf[512], *cp, *kstr;
	int len;

	/*
	 * Pass over kernel string, stripping the TFTP root prefix
	 * and converting '/' to '\' to keep the shark OFW happy.
	 */
	len = strlen(TFTP_ROOT);
	kstr = kernel;
	if (strncmp(kstr, TFTP_ROOT, len) == 0)
		kstr += len;
	for (cp = kstr; *cp != 0; cp++) {
		if (*cp == '/')
			*cp = '\\';
	}

#if 1
	/*
	 * Don't need the GW arg in our environment so leave it off
	 * to keep the command line as short as possible.  Unfortunately,
	 * we can't skip any of the other args...
	 */
	snprintf(buf, sizeof buf, "net:%s,%s,%s %s",
		 server, kstr, ipaddr, cmdline);
#else
	snprintf(buf, sizeof buf, "net:%s,%s,%s,%s %s",
		 server, kstr, ipaddr, gateway, cmdline);
#endif

	/*
	 * Hmm...looks like the OF_boot command only takes 128 bytes
	 * of command line!  Warn them if the command line might be truncated.
	 */
	if (strlen(buf) > 128)
		printf("WARNING: OF_boot command line will be truncated\n");

#if 0
	printf("Attempting boot with `%s'\n", buf);
#endif

	OF_boot(buf);
	panic("OF_boot failed!?");
}

#define PROMPT	"Enter boot spec: "
#define AUTO	"auto"
#define EXIT	"exit"
#define DEFAULT	"default"

/*
 * Allow for an interactive boot specification. Returning implies that
 * we go through the automated boot.
 */
static void
boot_interactive(void)
{
	char	buf[512];
	
	while (1) {
		/*
		 * Flush!
		 */
		while (console_trygetchar() >= 0)
			;

		console_putbytes(PROMPT, strlen(PROMPT));
		gets(buf);

		if (strncmp(buf, AUTO, strlen(AUTO)) == 0)
			return;

		if (strncmp(buf, EXIT, strlen(EXIT)) == 0)
			OF_exit();

		if (strncmp(buf, DEFAULT, strlen(EXIT)) == 0)
			boot_default();

		if (isalpha(buf[0]) || buf[0] == '/') {
			char *args = strchr(buf, ' ');

			if (args) {
				*args = '\0';
				args++;
			}
			else
				args = "";
			boot_net(buf, args);
		}

		printf("Choices are: \n"
		       "  <path to multiboot image> <args ...> or\n"
		       "  ``%s'' for default\n"
		       "  ``%s'' to drop to OFW prompt\n"
		       "  ``%s'' to use compiled in default\n",
		       AUTO, EXIT, DEFAULT);
	}
}

/*
 * Kind of a hack.
 */
void
delay(int millis)
{
	while (millis--)
		osenv_timer_spin(1000000);
}

int
console_trygetchar(void)
{
	return ofw_cons_trygetchar();
}

/*
 * Default reset routine exits which drops into the OFW prompt.
 * That is bad for us, since we don't want to rely on the console.
 * But, if we take a recursive reset, then we just drop to the prompt.
 */
void
arm32_reset()
{
	static int called;

	if (called++)
		OF_exit();

	printf("Unexpected exit!\n");
	dump_stack_trace();
	boot_default();
}
