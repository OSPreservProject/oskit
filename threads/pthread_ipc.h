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

#ifndef	_PTHREAD_IPC_H_
#define _PTHREAD_IPC_H_

#include <oskit/threads/ipc.h>

/*
 * Prototypes.
 */
struct pthread_thread;
struct schedmsg;

void		pthread_init_ipc(void);
void		pthread_ipc_cancel(struct pthread_thread *pthread);

#ifdef CPU_INHERIT
void		pthread_ipc_send_schedmsg(struct pthread_thread *pscheduler,
			struct pthread_thread *pthread, struct schedmsg *msg);
pthread_t	pthread_ipc_recv_wait(struct pthread_thread *pthread,
			void *msg, oskit_size_t msize);
pthread_t	pthread_ipc_recv_unwait(struct pthread_thread *pthread);
#endif

#endif /* _PTHREAD_IPC_H_ */
