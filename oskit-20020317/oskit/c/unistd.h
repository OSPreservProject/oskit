/*
 * Copyright (c) 1994-1999, 2001 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_UNISTD_H_
#define _OSKIT_C_UNISTD_H_

#include <oskit/types.h>
#include <oskit/compiler.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef oskit_size_t size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef oskit_ssize_t ssize_t;
#endif

#ifndef NULL
#define NULL		0
#endif

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* access function */
#define F_OK            0       /* test for existence of file */
#define X_OK            0x01    /* test for execute or search permission */
#define W_OK            0x02    /* test for write permission */
#define R_OK            0x04    /* test for read permission */

OSKIT_BEGIN_DECLS

/* POSIX unistd.h prototypes implemented in the OSKit */
void _exit(int __rc) OSKIT_NORETURN;
int access(const char *__name, oskit_mode_t __mode);
int chdir(const char *__path);
int chown(const char *__path, oskit_uid_t __owner, oskit_gid_t __group);
int close(int __fd);
int dup(int __fd);
int dup2(int __oldfd, int __newfd);
int link(const char *__path1, const char *__path2);
oskit_off_t lseek(int __fd, oskit_off_t __offset, int __whence);
ssize_t read(int __fd, void *__buf, size_t __n);
int rmdir(const char *__path);
int unlink(const char *__path);
ssize_t write(int __fd, const void *__buf, size_t __n);

/* POSIX unistd.h prototypes not implemented in the OSKit */
unsigned int alarm(unsigned int);
int execl(const char *, const char *, ...);
int execle(const char *, const char *, ...);
int execlp(const char *, const char *, ...);
int execv(const char *, char * const *);
int execve(const char *, char * const *, char * const *);
int execvp(const char *, char * const *);
int fdatasync(int);
oskit_pid_t fork(void);
long fpathconf(int, int);
int fsync(int);
int ftruncate(int, oskit_off_t);
char *getcwd(char *, size_t);
oskit_gid_t getegid(void);
oskit_uid_t geteuid(void);
oskit_gid_t getgid(void);
int getgroups(int, oskit_gid_t[]);
char *getlogin(void);
int gethostname(char *, int);
oskit_pid_t getpgrp(void);
oskit_pid_t getpid(void);
oskit_pid_t getppid(void);
oskit_uid_t getuid(void);
int isatty(int);
long pathconf(const char *, int);
int pause(void);
int pipe(int *);
int setgid(oskit_gid_t);
int setpgid(oskit_pid_t, oskit_pid_t);
oskit_pid_t setsid(void);
int setuid(oskit_uid_t);
unsigned int sleep(unsigned int);
int usleep(unsigned int);
long sysconf(int);
oskit_pid_t tcgetpgrp(int);
int tcsetpgrp(int, oskit_pid_t);
char *ttyname(int);

/* CAE BASE Issue 4, Version 2 unistd.h prototypes implemented in the OSKit */
extern char *optarg;
extern int optind, optopt, opterr, optreset;

int fchdir(int __fd);
int fchown(int __fd, oskit_uid_t __owner, oskit_gid_t __group);
int getopt(int argc, char * const argv[], const char *optstring);
int lchown(const char *__path, oskit_uid_t __owner, oskit_gid_t __group);
int readlink(const char  *__path, void *__buf, size_t __n);
int symlink(const char *__path1, const char *__path2);

/* ``Traditional'' BSD unistd.h prototypes implemented in the OSKit */
int mknod(const char *__name, oskit_mode_t __mode, oskit_dev_t __dev);
int swapon(const char *path);
int swapoff(const char *path);

/* OSKit specific prototypes */
extern void (*oskit_libc_exit)(int);
extern void oskit_init_libc(void);

OSKIT_END_DECLS

#endif /* _OSKIT_C_UNISTD_H_ */
