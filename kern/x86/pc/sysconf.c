/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
/*
 * this module implements a simple query facility for properties
 *
 * just add a bootmodule "sysconfig", make it contains lines of the
 * form KEY=VALUE, and use 'char *get_sysconfig("KEY")' to query.
 *
 * It's as simple as that!
 */

#include <oskit/x86/pc/base_multiboot.h>
#include <oskit/x86/multiboot.h>
#include <oskit/x86/base_vm.h>
#include <oskit/c/stdio.h>
#include <string.h>

#define DEFAULT_SYSCONFIG	"/sysconfig"

#define VERBOSITY 0

const char *
get_sysconfig_bis(char *bmodname, char *key)
{
	struct multiboot_module * m;
	char	*start = 0, *end = 0, *p;
	int	i;

#if VERBOSITY > 0
	printf(__FUNCTION__"(`%s', `%s') called\n", bmodname, key);
#endif
	if (!(boot_info.flags & MULTIBOOT_MODS))
		return 0;	/* no boot modules */

	m = (struct multiboot_module *)phystokv(boot_info.mods_addr);
	for (i = 0; i < boot_info.mods_count; i++, m++) {
		if (!strcmp((char *)phystokv(m->string), bmodname)) {
			start = (char *)phystokv(m->mod_start);
			end = (char *)phystokv(m->mod_end);
			break;
		}
	}

	/* no boot module with that particular name */
#if VERBOSITY > 0
	if (!start)
		printf("didn't find bm `%s'\n", bmodname);
#endif
	if (!start)
		return 0;

	/* replace newlines with zeros */
	p = start;
	while (p < end) {
		if (*p == '\n')
			*p = '\0';
		p++;
	}

	/* now the file is a set of strings */
	while (start < end) {

		/* skip newlines and comment lines */
		if (*start == '#' || *start == '\0') 
			goto nextline;
			
		p = strchr(start, '=');
		if (!p)
			goto nextline;

		if (!strncmp(key, start, strlen(key)))
			return p+1;

	nextline:
		start += strlen(start) + 1;
	}
	return 0;
}

const char *
get_sysconfig(char *key)
{
	return get_sysconfig_bis(DEFAULT_SYSCONFIG, key);
}

