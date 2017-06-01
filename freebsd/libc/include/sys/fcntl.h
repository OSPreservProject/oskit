/*
 * Use the oskit bits which are different from the BSD bits.
 */

#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

#include <sys/types.h>
#include <oskit/c/fcntl.h>

/* XXX no locks */
#define O_EXLOCK 0
#define O_SHLOCK 0

/* XXX flock crap stolen from freebsd/src/sys/sys/fcntl.h
   so unixy things compile.  fcntl will return ENOSYS if
   these are actually used.  */

/* record locking flags (F_GETLK, F_SETLK, F_SETLKW) */
#define	F_RDLCK		1		/* shared or read lock */
#define	F_UNLCK		2		/* unlock */
#define	F_WRLCK		3		/* exclusive or write lock */

/*
 * Advisory file segment locking data type -
 * information passed to system by user
 */
struct flock {
	off_t	l_start;	/* starting offset */
	off_t	l_len;		/* len = 0 means until end of file */
	pid_t	l_pid;		/* lock owner */
	short	l_type;		/* lock type: read/write, etc. */
	short	l_whence;	/* type of l_start */
};


#ifndef _POSIX_SOURCE
/* lock operations for flock(2) */
#define	LOCK_SH		0x01		/* shared file lock */
#define	LOCK_EX		0x02		/* exclusive file lock */
#define	LOCK_NB		0x04		/* don't block when locking */
#define	LOCK_UN		0x08		/* unlock file */
#endif

#endif /* _SYS_FCNTL_H_ */

