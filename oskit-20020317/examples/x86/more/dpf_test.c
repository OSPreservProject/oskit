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
 * Copyright (c) 1997 M.I.T.
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
 *      This product includes software developed by MIT.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <string.h>          /* for memset */
#include <stdlib.h>       
#include <oskit/clientos.h>
#include <oskit/startup.h>

/* stopwatch not compatible with oskit -LKW*/
/* #include "stopwatch.h" */
#include <oskit/dpf.h>

#include "../../dpf/src/dpf/demand.h"
/*#define demand(x,y) */

#define DPF_DEBUG 0
#if DPF_DEBUG
#define debug fprintf
#else
#define debug (void)
#endif

void
mk_ip(char *ip_addr, unsigned char ip1, unsigned char ip2, 
      unsigned char ip3, unsigned char ip4)
{
	ip_addr[0] = ip1;
	ip_addr[1] = ip2;
	ip_addr[2] = ip3;
	ip_addr[3] = ip4;
}
/*extern int atoi(char *); */
extern struct elem *getfilter(int *sz);

/* pathfinder message (fixed header) */
static unsigned short path_short1[22];
static unsigned short path_short2[22];
static unsigned short path[22] =
{
	0,	/* 0 */
	0,	/* 1 */
    	0xed,	/* 2 */
    	0x20,	/* 3 */
    	(6 << 8), /* 4 */
	0,	/* 5 */
    	0x4501,	/* 6 */
    	 0xc00c,	/* 7 */
	0,	/* 8 */
	0,	/* 9 */
    	1234,	/* 10 */
    	4321	/* 11 */
};

static unsigned udp[22] =
{
    0x242b0008,			/* 0:0 */
    0x8a42d,			/* 4:1 */
    0x92b0142b,			/* 2:8 */
    0x450008,			/* 3:12 */
    0x2000,			/* 4:16 */
    0x11ff0000,			/* 5:20 */
    0,				/* 6:24 */
    0,				/* 7:28 */
    0x5c00,			/* 8:32 */
    0x9919,			/* 9:36 */
    0x10000,			/* 10:40 */
};

void mk_tcp_string(char *new, unsigned short src_port, short src_ip, unsigned short dst_port, short dst_ip);
void *mk_tcp_short(int *sz, int dst_port);
void *mk_tcp_long(int *sz, int src_port, int src_ip, int dst_port, short dst_ip);

struct test_vector {
	long    msg[1024];	/* some big size */
	int     pid;		/* the pid that should accept it */
}       tv[512];

static void 
check_accept(int pidmap[1024], int n, int v)
{
	int     i, got, pid, error;

	for (error = i = 0; i < n; i++) {
		if (!(pid = pidmap[i]))
			continue;

		demand(pid < 256, bogus pid);
		got = oskit_dpf_iptr((unsigned char *)&tv[pid].msg[0], 1024);
		if (got != pid) {
			printf("got pid %d, %d, expecting %d\n", got, oskit_dpf_iptr((void *)tv[pid].msg, 1024), pid);
			oskit_dpf_iptr((unsigned char *)&tv[pid].msg[0], 1024);
			
			error++;
		}
	}
	if (error) {
		for (i = 0; i < n; i++)
			if (pidmap[i])
				printf("pid %d\n", pidmap[i]);
		oskit_dpf_output();
		demand(!error, got an error);
	}
}

/* insert a udp filter */
static void 
mk_udp_test_vector(int *pid, int src_port, int dst_port)
{
	void *    filter;
	int     sz;

	filter = oskit_dpf_mk_udp(&sz, src_port, dst_port);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	tv[*pid].pid = *pid;
	udp[8] = 0x5c00 | (src_port << 16);
	udp[9] = dst_port;
	memcpy(tv[*pid].msg, udp, sizeof udp);
}

static void 
mk_rarp_test_vector(int *pid, int src_port, int dst_port, int ip3)
{
	void *    filter;
	/* dre */
	char    ip_addr[8];
	int     sz;

	/* dre */
	memset(ip_addr, 0, sizeof ip_addr);

	/* creat rarp and accept string */
	mk_ip(ip_addr, 16, 32, ip3, 0);

	filter = oskit_dpf_mk_rarp(&sz, (void *)ip_addr);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);

	mk_rarp_string((void *) tv[*pid].msg, (void *)ip_addr);
}

static void 
mk_arp_test_vector(int *pid, int src_port, int dst_port, int ip3)
{
	char    ip_addr[8];
	void *    filter;
	int     sz;

	memset(ip_addr, 0, sizeof ip_addr);
	/* creat arp and accept string */
	mk_ip(ip_addr, 16, 32, ip3, 0);

	filter = oskit_dpf_mk_arp_req(&sz, ip_addr);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);

	mk_arp_string((void *) tv[*pid].msg, ip_addr);
}

static void 
mk_udp2_test_vector(int *pid, int src_port, int dst_port, int ip3)
{
	char    udp_dst[4], udp_src[4];
	void *    filter;
	int     sz;

	/* creat upd and accept string */
	mk_ip(udp_dst, 16, 32, ip3, 0);
	mk_ip(udp_src, 16, 32, 0, ip3);

	filter = oskit_dpf_mk_udpc(&sz, src_port, udp_src, dst_port, udp_dst);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);

	oskit_dpf_mk_udpc_string((void *) tv[*pid].msg, src_port, udp_src, dst_port, udp_dst);
}

static void 
mk_udp3_test_vector(int *pid, int src_port, int ip3)
{
	char    udp_src[4];
	void *    filter;
	int     sz;

	/* creat upd and accept string */
	mk_ip(udp_src, 18, 26, 0, ip3);

	filter = oskit_dpf_mk_udpu(&sz, src_port, udp_src);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);

	mk_udpc_string((void *) tv[*pid].msg, src_port, udp_src,
	    src_port, udp_src);
}

/* insert pathfinder filter */
static void 
mk_path_test_vector(int *pid, int src_port, int dst_port)
{
	void *    filter;
	int     sz;

	filter = oskit_dpf_mk_pathfinder(&sz, src_port, dst_port);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	tv[*pid].pid = *pid;
	path[10] = src_port;
	path[11] = dst_port;
	memcpy(tv[*pid].msg, path, sizeof path);
}

static void 
mk_tcp_test_vector(int *pid, int src_port, int dst_port)
{
	void *    filter;
	int     sz;
	short src_ip = 0xfefe, dst_ip = 0xbbbb;

	filter = mk_tcp_long(&sz, src_port, src_ip, dst_port, dst_ip);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);

	tv[*pid].pid = *pid;
	mk_tcp_string((char *)tv[*pid].msg, src_port, src_ip, dst_port, dst_ip);
}

static void
mk_short_path_test_vector2(int *pid, int dst_port)
{
	void *    filter;
	int     sz;

	filter = oskit_dpf_mk_short_path2(&sz, dst_port);
	if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
		demand(0, bogus insertion);
	tv[*pid].pid = *pid;
	path_short2[10] = 0;
	path_short2[11] = dst_port;
	memcpy(tv[*pid].msg, path_short2, sizeof path_short2);
}

static void
mk_short_path_test_vector(int *pid, int src_port)
{
        void *    filter;
        int     sz;

        filter = oskit_dpf_mk_short_path(&sz, src_port);
        if ((*pid = oskit_dpf_insert(filter, sz)) < 0)
                demand(0, bogus insertion);
        if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
                demand(0, bogus insertion);
        if (oskit_dpf_insert(filter, sz) != OSKIT_DPF_OVERLAP)
                demand(0, bogus insertion);
        tv[*pid].pid = *pid;
        path_short1[10] = src_port;
        path_short1[11] = 0;
        memcpy(tv[*pid].msg, path_short1, sizeof path_short1);
}

/* driver */
int
main(int argc, char *argv[])
{
	int     i, n, v, f, d, pidmap[1024], filters, id, ndelete, made_arp_p;

	oskit_clientos_init();
	oskit_dpf_init();

	n = 1;
	v = 0;
	f = 1;
	d = 1;
	memset(pidmap, 0, sizeof pidmap);

	srand(time(0));
#if 0
	/* get options */
	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-n") == 0)
			n = atoi(argv[++i]);
		else if (strcmp(argv[i], "-f") == 0)
			f = atoi(argv[++i]);
		else if (strcmp(argv[i], "-d") == 0)
			d = atoi(argv[++i]);
		else if (strcmp(argv[i], "-v") == 0) {
			v = 1;
			oskit_dpf_verbose(1);
		} else {
			puts("Options:");
			puts("\t-n #: number of timed trials.");
			puts("\t-f #: the number of filters.");
			puts("\t-d #: the number of test trials.");
			puts("\t-v:  verbosity.");
			return 0;
		}
#else /* 0 - options processing */
	n = 200;
	f = 200;
	d = 1000;
#endif /* 0 - options processing */


      loop:
	ndelete = filters = rand() % f + 1;
	printf("Starting trial %d: doing %d filters.\n", d, filters);

	for (made_arp_p = i = 0; i < filters; i++) {
	      retry:
#if 0
		switch (/* rand() % 8 */ rand() % 2 == 0 ? 6 : 2) {
#endif
		switch (rand() % 8) {
		case 0:
			mk_path_test_vector(&pidmap[i], i + 10, 4321);
			debug(stdout, "path_test %d\n",pidmap[i]);

			/* do an overlap(?) (instead of collision). */
			/* this is total bullshit. */
			if((i+1) < filters && rand() % 2) {
				check_accept(pidmap, i+1, v);
				i++;
				if(rand()%2)
                        		mk_short_path_test_vector(&pidmap[i], i + 10);
				else 
                        		mk_short_path_test_vector2(&pidmap[i], i + 10);
                       		debug(stdout, "short_path_test %d\n",pidmap[i]);
			}
			break;
		case 1:	/* randomly delete a filter */
			ndelete--;	/* one less created */
			if (!i)
				break;
			id = rand() % i;	/* who's the lucky one? */
			if (!pidmap[id])
				break;
			ndelete--;	/* one less to delete */
			debug(stdout, "deleting %d\n", pidmap[id]);
			if (oskit_dpf_delete(pidmap[id]) < 0)
				demand(0, bogus deletion);
			if (oskit_dpf_delete(pidmap[id]) >= 0)
				demand(0, cannot delete twice !);
			pidmap[id] = 0;
			break;
		case 2:
			debug(stdout, "rarp_test\n");
			mk_rarp_test_vector(&pidmap[i], i + 111, 3993, i);
			break;
		case 3:
			debug(stdout, "udp2_test\n");
			mk_udp2_test_vector(&pidmap[i], i + 111, 3993, i);
			break;
		case 4:
                       	debug(stdout, "short_path_test\n");
                        mk_short_path_test_vector(&pidmap[i], i + 10);
#if 0
                        mk_path_test_vector(&pidmap[i], 1234, 4321);
#endif
                        break;
		case 5:
			debug(stdout, "udp3_test\n");
			mk_udp3_test_vector(&pidmap[i], i + 0x1388, i + 130);
			break;
		case 6:
			/* create one arp */
			if (made_arp_p)
				goto retry;
			debug(stdout, "doing arp\n");
			made_arp_p = 1;
			mk_arp_test_vector(&pidmap[i], 111, 39193, 0);
			break;
		case 7:
			debug(stdout, "udp_test\n");
			mk_udp_test_vector(&pidmap[i], i + 111, 39193);
			break;

#if 0
		case 0:
			debug(stdout, "tcp_test\n");
			mk_tcp_test_vector(&pidmap[i], i + 10, 1222);
			debug(stdout, "tcp_test %d\n",pidmap[i]);

			/* do a collision. */
			if((i+1) < filters && rand() % 2) {
				check_accept(pidmap, i+1, v);
				i++;
                        	mk_short_tcp_test_vector(&pidmap[i], i + 9);
                       		debug(stdout, "short_path_test %d\n",pidmap[i]);
			}
#endif
			break;

		default:
			demand(0, bogus test);
		}
		/* make sure that accept is working */
		check_accept(pidmap, i+1, v);
	}

	/* delete everything and start over */
	if (d-- > 0) {
		check_accept(pidmap, filters, v);
		for (i = 0; i < ndelete; i++) {
			int     id;

			do {
				id = rand() % filters;
			} while (!pidmap[id]);
			debug(stdout, "deleting %d\n", pidmap[id]);

			if (oskit_dpf_delete(pidmap[id]) < 0)
				demand(0, bogus deletion);
			if (oskit_dpf_delete(pidmap[id]) >= 0)
				demand(0, cannot delete twice !);
			pidmap[id] = 0;
			/* make sure we didn't lose anyone */
			check_accept(pidmap, filters, v);
		}
		check_accept(pidmap, filters, v);	/* make sure reset is happening */
		for (i = 0; i < filters; i++)
			if (oskit_dpf_delete(pidmap[i]) >= 0)
				demand(0, cannot delete twice !);
		goto loop;
	}
	while (n-- > 0)
		check_accept(pidmap, f * 2, v);

	printf("Success!\n");
	return 0;
}
