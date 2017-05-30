/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_X86_PC_DOS_IO_H_
#define _OSKIT_X86_PC_DOS_IO_H_

#include <oskit/debug.h>
#include <oskit/x86/types.h>
#include <oskit/x86/pc/base_real.h>

struct stat;
struct timeval;
struct timezone;
struct termios;
struct trap_state;

typedef int dos_fd_t;

/*
 * Maximum number of bytes we can read or write with one DOS call
 * to or from memory not in the low 1MB accessible to DOS.
 * Must be less than 64KB.
 * Try to keep this size on a sector (512-byte) boundary for performance.
 */
#define DOS_BUF_SIZE 0x1000

/*
 * The dos_buf is a bounce buffer in the low 1MB of physical memory
 * used to transfer data to or from the DOS I/O system calls.
 */
char dos_buf[DOS_BUF_SIZE];

int dos_check_err(struct trap_state *ts);

int dos_open(const char *s, int flags, int create_mode, dos_fd_t *out_fh);
int dos_close(dos_fd_t fd);
int dos_read(dos_fd_t fd, void *buf, oskit_size_t size, oskit_size_t *out_actual);
int dos_write(dos_fd_t fd, const void *buf, oskit_size_t size, oskit_size_t *out_actual);
int dos_seek(dos_fd_t fd, oskit_addr_t offset, int whence, oskit_addr_t *out_newpos);
int dos_fstat(dos_fd_t fd, struct stat *st);
int dos_tcgetattr(dos_fd_t fd, struct termios *t);
int dos_rename(const char *oldpath, const char *newpath);
int dos_unlink(const char *filename);

int dos_gettimeofday(struct timeval *tv, struct timezone *tz);

#endif /* _OSKIT_X86_PC_DOS_IO_H_ */
