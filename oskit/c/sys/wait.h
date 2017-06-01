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
/*
 * POSIX-defined <sys/wait.h>
 */
#ifndef _OSKIT_C_SYS_WAIT_H
#define _OSKIT_C_SYS_WAIT_H

#include <oskit/compiler.h>


/* Upper 8 bits are return code, lowest 7 are status bits */
#define _WSTATMASK            0177
#define _WEXITSTAT(stat_val) ((stat_val) & _WSTATMASK)

/* Evaluates to nonzero if child terminated normally. */
#define WIFEXITED(stat_val)   (_WEXITSTAT(stat_val) == 0)

/* Evaulates to nonzero if child terminated due to unhandled signal */
#define WIFSIGNALED(stat_val) ((_WEXITSTAT(stat_val) != _WSTATMASK) && (_WEXITSTAT(stat_val) != 0))

/* Evaluates to nonzero if child process is currently stopped. */
#define WIFSTOPPED(stat_val)  (_WEXITSTAT(stat_val) == _WSTATMASK)

/*
 * If WIFEXITED(stat_val) is nonzero, this evaulates to the return code of the
 * child process.
 */
#define WEXITSTATUS(stat_val) ((stat_val) >> 8)

/*
 * If WIFSIGNALED(stat_val) is nonzero, this evaluates to the signal number
 * that caused the termination.
 */
#define WTERMSIG(stat_val)    (_WEXITSTAT(stat_val))

/*
 * If WIFSTOPPED(stat_val) is nonzero, this evaluates to the signal number
 * that caused the child to stop.
 */
#define WSTOPSIG(stat_val)    ((stat_val) >> 8)

/*
 * Macros to create a death status word given an exit code or termination
 * signal, or stop status word given a stop signal.
 */
#define	W_EXITCODE(ret, sig)	((ret) << 8 | (sig))
#define	W_STOPCODE(sig)		((sig) << 8 | 0177)



/*
 * bitwise OR-able option flags to waitpid().  waitpid() will not suspend the
 * caller if WNOHANG is given and there is no process status immediately
 * available.  WUNTRACED causes waitpid() to report stopped processes whose
 * status has not yet been reported since they stopped.
 */
#define WNOHANG    1
#define WUNTRACED  2


OSKIT_BEGIN_DECLS

pid_t wait(int *stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

OSKIT_END_DECLS

#endif
