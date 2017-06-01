/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * External definitions for simple IPC.
 */
#ifndef	_OSKIT_THREADS_IPC_H_
#define _OSKIT_THREADS_IPC_H_

#include <oskit/types.h>
#include <oskit/error.h>

oskit_error_t	oskit_ipc_send(pthread_t dst, void *msg,
			oskit_size_t msg_size, oskit_s32_t timeout);

oskit_error_t   oskit_ipc_recv(pthread_t src, void *msg,
			oskit_size_t msg_size, oskit_size_t *actual,
			oskit_s32_t timeout);

oskit_error_t	oskit_ipc_wait(pthread_t *src, void *msg,
			oskit_size_t msg_size, oskit_size_t *actual,
			oskit_s32_t timeout);

oskit_error_t	oskit_ipc_call(pthread_t dst, void *sendmsg,
			oskit_size_t sendmsg_size, void *recvmsg,
			oskit_size_t recvmsg_size, oskit_size_t *actual,
			oskit_s32_t timeout);

oskit_error_t	oskit_ipc_reply(pthread_t src, void *msg,
			oskit_size_t msg_size);

#endif


