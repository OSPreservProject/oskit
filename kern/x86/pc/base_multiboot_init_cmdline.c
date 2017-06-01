/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/base_multiboot.h>
#include <oskit/lmm.h>
#include <oskit/x86/pc/phys_lmm.h>

extern char **environ;

char **oskit_bootargv;
int oskit_bootargc = 0;

static char *null_args[] = {"kernel", 0};

static const char delim[] = " \f\n\r\t\v";

/*
 * The command-line comes to us as a string and contains booting-options,
 * environment-variable settings, and arguments to main().
 * The format is like this:
 *	progname [<booting-options and foo=bar> --] <args to main>
 * For example
 *	kernel DISPLAY=host:0 -d -- -rf foo bar
 * which would be converted into
 *	environ = {"DISPLAY=host:0", 0}
 *	oskit_bootargv = {"-d", 0}
 *	argv = {"kernel", "-rf", "foo", "bar", 0}
 * Actually, those would be pointers into
 *	{"kernel", "-rf", "foo", "bar", 0, "DISPLAY=host:0", 0, "-d", 0}
 * If there is no "--" in the command line, then the entire thing is parsed
 * into argv, no environment vars or booting options are set.
 */
void
base_multiboot_init_cmdline(int *argcp, char ***argvp)
{
	int argc;
	char **argv;

	if (boot_info.flags & MULTIBOOT_CMDLINE) {
		char *cl = (char*)phystokv(boot_info.cmdline);
		unsigned cllen = strlen(cl);
		char *toks[1 + cllen];
		unsigned ntoks, nargs, nvars, nbops;
		char *tok;
		unsigned i, dashdashi;
		char **argbuf;
		unsigned envc;

		/*
		 * Parse out the tokens in the command line.
		 * XXX Might be good to handle quotes.
		 */
		ntoks = 0;
		for (tok = strtok(cl, delim); tok; tok = strtok(0, delim))
			toks[ntoks++] = tok;

		/* After this we assume at least one arg (progname). */
		if (ntoks == 0)
			goto nocmdline;

		/* Look for a "--" and record its index. */
		dashdashi = 0;
		for (i = 0; i < ntoks; i++)
			if (strcmp(toks[i], "--") == 0) {
				dashdashi = i;
				break;
			}

		/* Count number of args, env vars, and bootopts. */
		nargs = 1;		/* for progname */
		nvars = 0;
		nbops = 0;
		for (i = 1; i < dashdashi; i++)
			if (strchr(toks[i], '='))
				nvars++;
			else
				nbops++;
		for (i = dashdashi + 1; i < ntoks; i++)
			nargs++;

		/*
		 * Now we know how big our argbuf is.
		 * argv, environ, and oskit_bootargv will point into this.
		 */
		argbuf = lmm_alloc(&malloc_lmm,
				   sizeof(char *) * (nargs + 1) +
				   sizeof(char *) * (nvars + 1) +
				   sizeof(char *) * (nbops + 1),
				   0);
		assert(argbuf);

		/*
		 * Set up the pointers into argbuf, then fill them in.
		 */
		argv = argbuf;
		environ = argv + nargs + 1;
		oskit_bootargv = environ + nvars + 1;

		argc = 0;
		argv[argc++] = toks[0];
		envc = 0;
		oskit_bootargc = 0;
		for (i = 1; i < dashdashi; i++)
			if (strchr(toks[i], '='))
				environ[envc++] = toks[i];
			else
				oskit_bootargv[oskit_bootargc++] = toks[i];
		for (i = dashdashi + 1; i < ntoks; i++)
			argv[argc++] = toks[i];

		argv[argc] = 0;
		environ[envc] = 0;
		oskit_bootargv[oskit_bootargc] = 0;
	}
	else
	{
nocmdline:
		/* No command line.  */
		argc = 1;
		argv = null_args;
		environ = null_args + 1;
		oskit_bootargc = 0;
		oskit_bootargv = environ;
	}

	*argcp = argc;
	*argvp = argv;
}

