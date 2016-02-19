/*
 * Copyright (c) 1995-2001 University of Utah and the Flux Group.
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

#include <oskit/x86/pc/rtc.h>		/* RTC_BASEHI */
#include <oskit/x86/pc/phys_lmm.h>	/* phys_lmm_add */
#include <oskit/x86/types.h>		/* oskit_addr_t */
#include <oskit/x86/base_cpu.h>		/* base_cpu_setup */
#include <oskit/x86/base_vm.h>		/* kvtophys */

#include <ctype.h>			/* isspace */
#include <stdlib.h>			/* malloc */
#include <string.h>			/* strcpy */
#include <stdio.h>			/* printf */
#include <assert.h>			/* assert */
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>			/* ntohl */
#include <malloc.h>			/* malloc_lmm */
#include <oskit/lmm.h>			/* lmm_remove_free */
#include <oskit/x86/pc/base_multiboot.h> /* boot_info */

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_timer.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>

#include "timer.h"			/* timer_{init,shutdown} */
#include "driver.h"			/* net_init */
#include "gdt.h"			/* copy_base_gdt */
#include "boot.h"			/* kerninfo */
#include "misc.h"			/* netprintf */
#include "reboot.h"			/* reboot_stub_loc */

#define CMD_LINE_LEN 512

static char prev_cmd[CMD_LINE_LEN] = "";
static int boot_count = 1;

static void print_help(char **);
extern char version[], build_info[];
extern oskit_addr_t return_address;

#define NDEFAULT_ARGS 2

/*
 * Figure out if -h (serial) or -f (fast serial) are in the oskit_bootargv
 * and put them in the default_args list.
 */
static void
init_default_args(int bootargc, char *bootargv[], char *default_args[])
{
	char **p = &default_args[0];

#ifdef FORCESERIALCONSOLE
	*p++ = "-h";
	*p++ = "-f";
#else
	int i;

	for (i = 0; i < bootargc; i++)
		if (strcmp(bootargv[i], "-h") == 0 ||
		    strcmp(bootargv[i], "-f") == 0)
			*p++ = bootargv[i];
#endif
	*p = NULL;
}

/*
 * Build a command line from the program name, the default_args,
 * what they specified, and our -retaddr option.
 *
 * The command line is in a special format understood by the oskit
 * multiboot startup code (see base_multiboot_init_cmdline.c).
 * Basically, the format separates booting-options and environment
 * variable settings from normal args to pass to main().  The format is like
 *	progname [<booting-options and foo=bar> --] <args to main>
 * For example
 *	kernel DISPLAY=host:0 -d -- -rf foo bar
 */
static char *
build_cmdline(char *progname, char *default_args[NDEFAULT_ARGS], char *input)
{
	char *toks[input? strlen(input) : 0];
	char *tok;
	int ntoks;
	int i;
	int havesep;
	int cllen;
	char *cmdline;

	/* Tokenize their input. */
	ntoks = 0;
	if (input)
		for (tok = strtok(input, " \t"); tok; tok = strtok(0, " \t"))
			toks[ntoks++] = tok;

	/* Figure out if they specified a "--". */
	havesep = 0;
	for (i = 0; i < ntoks; i++)
		if (strcmp(toks[i], "--") == 0) {
			havesep = 1;
			break;
		}
	
	/*
	 * Toggle off any bootopts that they specified.
	 * We only have to do this if there is a --, since otherwise
	 * they didn't specify any bootopts.
	 */
	if (havesep) {
		for (i = 0; i < ntoks; i++) {
			int j;

			if (strcmp(toks[i], "--") == 0)
				break;
			for (j = 0; j < NDEFAULT_ARGS && default_args[j]; j++)
				if (strcmp(toks[i], default_args[j]) == 0) {
					toks[i] = 0;
					default_args[j] = 0;
				}
		}
	}

	/* Figure out how big the command-line will be and allocate it. */
	cllen = strlen(progname) + 1;
	cllen += strlen("-retaddr 0x00ff00ff") + 1;
	for (i = 0; i < NDEFAULT_ARGS; i++)
		if (default_args[i])
			cllen += strlen(default_args[i]) + 1;
	if (! havesep)
		cllen += strlen("--") + 1;
	for (i = 0; i < ntoks; i++)
		if (toks[i])
			cllen += strlen(toks[i]) + 1;
	cllen += 1;			/* for NUL */

	cmdline = mustcalloc(cllen,1);
	assert(cmdline);

	/*
	 * Now fill it in.
	 */
	strcpy(cmdline, progname);
	strcat(cmdline, " ");

	sprintf(cmdline + strlen(progname) + 1, "-retaddr 0x%08x",
		reboot_stub_loc);
	strcat(cmdline, " ");

	for (i = 0; i < NDEFAULT_ARGS; i++)
		if (default_args[i]) {
			strcat(cmdline, default_args[i]);
			strcat(cmdline, " ");
		}

	if (! havesep) {
		strcat(cmdline, "--");
		strcat(cmdline, " ");
	}
	
	for (i = 0; i < ntoks; i++)
		if (toks[i]) {
			strcat(cmdline, toks[i]);
			strcat(cmdline, " ");
		}

	return cmdline;
}

int
main(int argc, char *argv[])
{
	char buf[CMD_LINE_LEN];
	int cmdlinelen;
	oskit_error_t err;
	int done = 0;
	struct kerninfo kinfo;
	char *input = buf;
	char *cmdline = NULL;
	char *default_args[NDEFAULT_ARGS+1] = {0};
	struct multiboot_info *mbi;
	char *argstring;

	oskit_clientos_init();
	printf("\n%s\n", version);
	printf("%s\n", build_info);

	start_clock();
	start_net_devices();
	start_fs_bmod();
	timer_init();
	err = net_init();
	if (err)
		panic("Couldn't initialize network: 0x%08x", err);

	printf("I am %s", hostname);
	if (domain)
		printf(".%s", domain);
	printf(" (IP: %s, mask: %s)\n", ipaddr, netmask);
	if (gateway)
		printf(" Gateway: %s\n", gateway);
	if (nameservers && nameservers[0])
		printf(" Nameserver: %s\n", nameservers[0]);
	
	init_default_args(oskit_bootargc, oskit_bootargv, default_args);
	
	/*
	 * Figure out our rootserver, dir to mount, and file to load.
	 * Then try to get it.
	 */
reprompt:
	input = buf; /* In case we moved input forward to eat spaces */
	cmdlinelen = CMD_LINE_LEN;
	printf("NetBoot %d> ", boot_count);
	fgets(input, 512, stdin);
	input[strlen(input) - 1] = '\0';	/* chop \n */
	while (input[0] && isspace(input[0])) {
		input++;
		cmdlinelen--; /* record the amount of space avail. in buffer */
	}
	if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
		done = 1;
		goto done;
	}
	if (input[0] == '\0')
		goto reprompt;
	
	boot_count++;
        patch_clone(&boot_count, sizeof boot_count);
	if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
		print_help(default_args);
		goto reprompt;
	}
	if (strcmp(input, "!!") == 0) {
                /* get the previous command */
                strcpy(buf,prev_cmd);
                input = buf;
        } else {
                /* save the command for next time round */
                strcpy(prev_cmd,input);
                patch_clone(&prev_cmd, sizeof prev_cmd);
        }                
	if (! parse_cmdline(input,
			    cmdlinelen,
			    &kinfo.ki_transport,
			    &kinfo.ki_ip.s_addr,
			    &kinfo.ki_dirname,
			    &kinfo.ki_filename,
			    &argstring)) {
		printf("Bad command line, type \"help\" for help.\n");
		goto reprompt;
	}
	cmdline = build_cmdline(kinfo.ki_filename, default_args, argstring);
#ifdef DEBUG
	netprintf("Root server: %I, protocol: %s,\r\n"
		  "  dir: %s, file: %s,\r\n"
		  "  cmdline: `%s'\r\n",
		  kinfo.ki_ip.s_addr,
		  kinfo.ki_transport == XPORT_NFS ? "NFS" : "TFTP",
		  kinfo.ki_dirname, kinfo.ki_filename, cmdline);
#endif
	if (getkernel_net(&kinfo) != 0) {
		printf("Can't get the kernel, try again.\n");
		free(cmdline);
		goto reprompt;
	}

	/*
	 * Clean some stuff up before we load the new image.
	 * In particular we don't want the timer hammering it,
	 * and don't want it using some bogus GDT (we may overwrite ours
	 * with the new image).
	 */
 done:
	net_shutdown();
	timer_shutdown();
	copy_base_gdt();

	if (done)
		exit(0);

	/*
	 * Reserve the memory where the kernel will eventually reside.
	 * Future malloc's, like the one for the multiboot_info struct
	 * we pass to the new kernel, will stay out of this region.
	 *
	 * IMPORTANT NOTE: We must do this AFTER the shutdown stuff
	 * because all lmm_remove_free does is alloc all the free
	 * blocks in the given range.
	 * If things were already allocated in that range but get freed
	 * after the lmm_remove_free, they can be allocated again and
	 * then get clobbered by the kernel copy in do_boot and Bad Things
	 * will happen.
	 */
	lmm_remove_free(&malloc_lmm,
			(void *)phystokv(kinfo.ki_mbhdr.load_addr),
			kerninfo_imagesize(&kinfo));

	/*
	 * Allocate and fill in a multiboot_info struct we will pass.
	 */
	mbi = kinfo.ki_mbinfo = mustcalloc(sizeof(struct multiboot_info), 1);
	mbi->mem_lower = boot_info.mem_lower;
	mbi->mem_upper = boot_info.mem_upper;
	mbi->flags |= MULTIBOOT_MEMORY;
	if (cmdline) {
		mbi->cmdline = kvtophys(strdup(cmdline));
		mbi->flags |= MULTIBOOT_CMDLINE;
	}
	
	/* Load it. */
	loadkernel(&kinfo);
	panic("loadkernel returned");
	return 0; /* eat the warning */
}

void
print_help(char **default_args)
{
	int i;
	
	printf("\n");
	printf("%s\n", version);
	printf("%s\n", build_info);
	printf("\n");
	printf("To boot, tell me the kernel file name and args in the form\n"
	       "\t<IPaddr|host>:<path> [<bootopts and env vars> --] "
	       "[<program args>]\n"
	       "For example:\n"
	       "\t155.22.33.44:/home/foo/kernel -f -h DISPLAY=host:0 -- -wow\n"
	       "or\n"
	       "\tmyhost:/home/foo/kernel -x -y file1 file2\n"
	       "The directory must be an NFS exported directory.\n"
	       "The kernel must be a MultiBoot kernel.\n");
	printf("\n");
	printf("Type \"quit\" or \"exit\" to %s.\n",
	       return_address? "return to caller" : "reboot");
	if (default_args[0]) {
		printf("\n");
		printf("The default bootopts are as follows "
		       "and act as toggles:\n");
		printf("\t");
		for (i = 0; i < NDEFAULT_ARGS && default_args[i]; i++)
			printf("%s ", default_args[i]);
		printf("\n");
	}
	printf("\n");
}
