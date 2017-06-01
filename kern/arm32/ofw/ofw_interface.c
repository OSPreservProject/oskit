/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Interface to the OFW. A bunch of routines that invoke the single OFW
 * entrypoint with the proper argument vector. Not much to it ...
 *
 * Derived from netbsd:/usr/src/sys/arch/arm32/ofw/openfirm.c
 */

#include <oskit/machine/ofw/ofw.h>
#include <stdarg.h>

/*
 * Each OFW request gets one of these suckers.
 */
typedef struct ofwRequest {
	char	*request;
	int	numArgs;
	int	numResults;
	void	*args[10];
} ofwRequest;

/*
 * The OFW entrypoint.
 */
static int		(*OF_entrypoint)(void *);
#define CALLOFW(reqp)	OF_entrypoint((void *) reqp)

/*
 * Init the OFW by remembering the entry handle that is passed into the
 * kernel at boot time.
 */
void
OF_init(int (*entrypoint)(void *))
{
	OF_entrypoint = entrypoint;
}

/*
 * Reboot the machine.
 */
void
OF_exit(void)
{
	static ofwRequest request = { "exit", 0, 0 };
	
	CALLOFW(&request);
}

void
OF_boot(char *bootspec)
{
	static ofwRequest request = { "boot", 1, 0 };

	request.args[0] = bootspec;
	
	if (CALLOFW(&request) == -1)
		OF_exit();
	
	while (1);			/* just in case */
}

/*
 * Find the named device node.
 */
int
OF_finddevice(char *name)
{
	static ofwRequest request = { "finddevice", 1, 1 };

	request.args[0] = name;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[1];
}

int
OF_getprop(int handle, char *prop, void *buf, int buflen)
{
	static ofwRequest request = { "getprop", 4, 1 };

	request.args[0] = (void *) handle;
	request.args[1] = prop;
	request.args[2] = buf;
	request.args[3] = (void *) buflen;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[4];
}

int
OF_getproplen(int handle, char *prop)
{
	static ofwRequest request = { "getproplen", 2, 1 };

	request.args[0] = (void *) handle;
	request.args[1] = prop;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[2];
}

/*
 * Read from a device.
 */
int
OF_read(int handle, void *buf, int buflen)
{
	static ofwRequest request = { "read", 3, 1 };

	request.args[0] = (void *) handle;
	request.args[1] = buf;
	request.args[2] = (void *) buflen;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[3];
}

/*
 * Write to a device.
 */
int
OF_write(int handle, void *buf, int buflen)
{
	static ofwRequest request = { "write", 3, 1 };

	request.args[0] = (void *) handle;
	request.args[1] = buf;
	request.args[2] = (void *) buflen;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[3];
}

/*
 * Call a method indirectly. Variable number of arguments.
 */
int
OF_call_method_1(char *method, int ihandle, int nargs, ...)
{
	static ofwRequest request = { "call-method" };
	va_list		  ap;
	int		  *ip, n;
	
	if (nargs > 6)
		return -1;

	request.numArgs    = nargs + 2;
	request.numResults = 2;
	request.args[0]    = method;
	request.args[1]    = (void *) ihandle;

	va_start(ap, nargs);
	n = nargs;
	for (ip = (int *) &request.args[2 + n]; --n >= 0; )
		*--ip = va_arg(ap, int);
	va_end(ap);
	
	if (CALLOFW(&request) == -1)
		return -1;

	/*
	 * First result is success/failure?
	 */
	if (request.args[request.numArgs])
		return -1;
	
	return (int) request.args[request.numArgs + 1];
}

/*
 * Call a method indirectly. Variable number of arguments. Result values
 * are returned to the caller in place.
 */
int
OF_call_method(char *method, int ihandle, int nargs, int nreturns, ...)
{
	static ofwRequest request = { "call-method" };
	va_list		  ap;
	int		  *ip, n;
	
	if (nargs > 6)
		return -1;

	request.numArgs    = 2 + nargs;
	request.numResults = 1 + nreturns;
	request.args[0]    = method;
	request.args[1]    = (void *) ihandle;

	va_start(ap, nreturns);
	n = nargs;
	for (ip = (int *) &request.args[2 + n]; --n >= 0; )
		*--ip = va_arg(ap, int);
	va_end(ap);

/*	printf("cm: %s %d %d %p %p %p %p %p %p\n",
	       request.request, request.numArgs, request.numResults,
	       request.args[0], request.args[1], request.args[2],
	       request.args[3], request.args[4], request.args[5]); */
	
	if (CALLOFW(&request) == -1)
		return -1;

	/*
	 * First result is success/failure?
	 */
	if (request.args[request.numArgs])
		return (int) request.args[request.numArgs];
	/*
	 * Copy back results.
	 */
	n = nreturns;
	for (ip = (int *) &request.args[request.numArgs + request.numResults];
	     --n >= 0; )
		*va_arg(ap, int *) = *--ip;

	return 0;
}

/*
 * Huh?
 */
int
OF_instance_to_package(int ihandle)
{
	static ofwRequest request = { "instance-to-package", 1, 1 };

	request.args[0] = (void *) ihandle;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[1];
}

int
OF_package_to_path(int phandle, char *buf, int buflen)
{
	static ofwRequest request = { "package-to-path", 3, 1 };

	request.args[0] = (void *) phandle;
	request.args[1] = (void *) buf;
	request.args[2] = (void *) buflen;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[3];
}

int
OF_open(char *dname)
{
	static ofwRequest request = { "open", 1, 1 };

	request.args[0] = (void *) dname;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[1];
}

int
OF_peer(int phandle)
{
	static ofwRequest request = { "peer", 1, 1 };

	request.args[0] = (void *) phandle;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[1];
}

int
OF_child(int phandle)
{
	static ofwRequest request = { "child", 1, 1 };

	request.args[0] = (void *) phandle;

	if (CALLOFW(&request) == -1)
		return -1;

	return (int) request.args[1];
}

/*
 * Convert from an OFW encoded integer to a regular integer.
 */
unsigned int
OF_decode_int(const unsigned char *p)
{
#if 0
	unsigned int i = *p++;

	i = (i << 8) | *p++;
	i = (i << 8) | *p++;
	i = (i << 8) | *p;

	return i;
#else
	unsigned int i = *p++ << 8;
	i = (i + *p++) << 8;
	i = (i + *p++) << 8;
	return (i + *p);
#endif	
}

