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

/*
 * UDP via the PXE
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/error.h>

#include "boot.h"
#include "pxe.h"
#include "decls.h"


int
udp_open(uint32_t src_ip)
{
	t_PXENV_UDP_OPEN	udp_open_params;

	memset(&udp_open_params, 0, sizeof(udp_open_params));
	udp_open_params.src_ip = src_ip;

	pxe_invoke(PXENV_UDP_OPEN, &udp_open_params);
	if (udp_open_params.Status) {
		printf(__FUNCTION__ ": failed. 0x%x\n",
		       udp_open_params.Status);
		return OSKIT_E_FAIL;
	}
	
	return 0;
}

int
udp_close(void)
{
	t_PXENV_UDP_CLOSE	udp_close_params;

	memset(&udp_close_params, 0, sizeof(udp_close_params));

	pxe_invoke(PXENV_UDP_CLOSE, &udp_close_params);
	if (udp_close_params.Status) {
		printf(__FUNCTION__ ": failed. 0x%x\n",
		       udp_close_params.Status);
		return OSKIT_E_FAIL;
	}
	
	return 0;
}

static int
real_udp_read(uint32_t dst_ip, int dst_port, void *buffer, int bufsize, 
	 int *out_actual, int *src_port, uint32_t *src_ip)
{
	t_PXENV_UDP_READ	udp_read_params;

	memset(&udp_read_params, 0, sizeof(udp_read_params));
	udp_read_params.dest_ip     = dst_ip;
	udp_read_params.d_port      = htons(dst_port);
	udp_read_params.buffer_size = bufsize;
	udp_read_params.buffer.segment = PXESEG(buffer);
	udp_read_params.buffer.offset  = PXEOFF(buffer);
	
	pxe_invoke(PXENV_UDP_READ, &udp_read_params);
	if (udp_read_params.Status)
		return OSKIT_E_FAIL;

	*src_ip        = udp_read_params.src_ip;
	*src_port      = ntohs(udp_read_params.s_port);
	*out_actual    = udp_read_params.buffer_size;

	return 0;
}

int
udp_read(uint32_t dst_ip, int dst_port, void *buffer, int bufsize, 
	 int *out_actual, int *src_port, uint32_t *src_ip)
{
	return real_udp_read(dst_ip, dst_port, buffer, bufsize,
			     out_actual, src_port, src_ip);
}

static int
real_udp_write(uint32_t dst_ip, uint32_t gw_ip, int src_port, int dst_port,
	  void *buffer, int bufsize)
{
	t_PXENV_UDP_WRITE	udp_write_params;

	memset(&udp_write_params, 0, sizeof(udp_write_params));
	udp_write_params.ip       = dst_ip;
	udp_write_params.gw       = gw_ip;
	udp_write_params.src_port = htons(src_port);
	udp_write_params.dst_port = htons(dst_port);
	udp_write_params.buffer_size = bufsize;
	udp_write_params.buffer.segment = PXESEG(buffer);
	udp_write_params.buffer.offset  = PXEOFF(buffer);
	
	pxe_invoke(PXENV_UDP_WRITE, &udp_write_params);
	if (udp_write_params.Status) {
		printf(__FUNCTION__ ": failed. 0x%x\n",
		       udp_write_params.Status);
		return OSKIT_E_FAIL;
	}

	return 0;
}

int
udp_write(uint32_t dst_ip, uint32_t gw_ip, int src_port, int dst_port,
	  void *buffer, int bufsize)
{
	return real_udp_write(dst_ip, gw_ip, src_port, dst_port,
			      buffer, bufsize);
}

#if 0
void
udp_printstats(void)
{
	t_PXENV_UNDI_GET_STATISTICS undi_getstats;

	{
		t_PXENV_UNDI_GET_INFORMATION undi_getinfo;
		static int called;

		if (called == 0) {
			called = 1;
			memset(&undi_getinfo, 0, sizeof(undi_getinfo));
			pxe_invoke(PXENV_UNDI_GET_INFORMATION, &undi_getinfo);
			if (undi_getinfo.Status) {
				printf(__FUNCTION__ ": failed. 0x%x\n",
				       undi_getinfo.Status);
			} else {
				printf("IOaddr=%x, IRQ=%d, MTU=%d, HW=%d, rxbufs=%d, txbufs=%d\n",
				       undi_getinfo.BaseIo,
				       undi_getinfo.IntNumber,
				       undi_getinfo.MaxTranUnit,
				       undi_getinfo.HwType,
				       undi_getinfo.RxBufCt,
				       undi_getinfo.TxBufCt);
			}
		}
	}
	memset(&undi_getstats, 0, sizeof(undi_getstats));
	pxe_invoke(PXENV_UNDI_GET_STATISTICS, &undi_getstats);
	if (undi_getstats.Status) {
		printf(__FUNCTION__ ": failed. 0x%x\n",
		       undi_getstats.Status);
	} else {
		printf("xmit=%d, recv=%d, CRCerr=%d, reserr=%d\n",
		       undi_getstats.XmitGoodFrames,
		       undi_getstats.RcvGoodFrames,
		       undi_getstats.RcvCRCErrors,
		       undi_getstats.RcvResourceErrors);
	}
#if 0
	{
		t_PXENV_UNDI_CLEAR_STATISTICS undi_clearstats;

		memset(&undi_clearstats, 0, sizeof(undi_clearstats));
		pxe_invoke(PXENV_UNDI_CLEAR_STATISTICS, &undi_clearstats);
		if (undi_clearstats.Status) {
			printf(__FUNCTION__ ": failed. 0x%x\n",
			       undi_clearstats.Status);
		}
	}
#endif
}
#endif
