/*
 * Copyright (c) 1997-1998, 2000, 2001 University of Utah and the Flux Group.
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

#ifndef unix_support_h
#define unix_support_h

#include <signal.h>  /* Host OS signal.h NOT OSKit signal.h */

struct sockaddr;

extern sigset_t		oskitunix_sigtraps;

int oskitunix_set_signal_handler(int sig, 
			 void (*handler)(int, int, struct sigcontext *));

int oskitunix_open_eth(char *ifname, char *myaddr,
		       unsigned int *buflen, int no_ieee802_3);
void oskitunix_close_eth(int fd);

int oskitunix_set_async_fd(int fd);
int oskitunix_unset_async_fd(int fd);

void oskitunix_register_async_fd(int fd, unsigned iotype,
				 void (*callback)(void *), void *arg);

void oskitunix_unregister_async_fd(int fd);


/**
 * NOTE: base_irq_softint_handler is always given a NULL pointer in
 * this code.  That's okay because either (1) the pthread-version
 * (from threads/pthread_init.c) of base_irq_softint_handler() is used
 * and that version doesn't pay attention to its argument OR (2) (for
 * the non-pthread case) the version OSKit/UNIX version is used (from
 * unix/base_irq_softint_handler.o) and that one doesn't do anything.
 *
 * The "correct" thing might be to cons-up a bogus trap_state.  Or,
 * perhaps we should be calling out to something a bit higher-level
 * than base_irq_softint_handler().
 *
 * XXX there's no clean header file to include to get this prototype.
 * (base_irq_softint_handler is normally defined in libkern)
 */
struct trap_state;
extern void (*base_irq_softint_handler)(struct trap_state *ts);


#define IOTYPE_READ	1
#define IOTYPE_WRITE	2

int oskitunix_threaded_read(int fd, void* buf, size_t len);
int oskitunix_threaded_write(int fd, const void* buf, size_t len);
int oskitunix_threaded_connect(int fd, struct sockaddr* addr, size_t len);
int oskitunix_threaded_accept(int fd, struct sockaddr* addr, size_t* len);
int oskitunix_threaded_recvfrom(int fd, void* buf, size_t len, int flags,
				struct sockaddr* from, int* fromlen);
int oskitunix_threaded_sendto(int fd, const void* buf, size_t len, int flags,
			      struct sockaddr* to, int tolen);

#endif /* unix_support_h */
