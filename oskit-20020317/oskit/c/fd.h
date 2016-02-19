/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * This header file defines common types and functions used by
 * the minimal C library's default POSIX file descriptor layer implementation
 * (read, write, seek, connect, accept, etc.).
 * This header file is NOT a standard POSIX or Unix header file;
 * instead its purpose is to expose the implementation of this facility
 * so that the client can fully control it and use it in arbitrary contexts.
 *
 * Since the minimal C library does not know the context it is used in,
 * you must initialize it with references to appropriate COM objects
 * which provide the actual underlying services.
 */
#ifndef _OSKIT_C_FD_H_
#define _OSKIT_C_FD_H_

#include <oskit/compiler.h>
#include <oskit/com.h>
#include <oskit/com/stream.h>
#include <oskit/io/ttystream.h>
#include <oskit/io/asyncio.h>
#include <oskit/net/socket.h>
#include <oskit/fs/dir.h>
#include <oskit/fs/openfile.h>
#include <oskit/c/errno.h>

#ifdef THREAD_SAFE
#include <oskit/threads/pthread.h>
#endif

/*
 * Structure representing a file descriptor table entry.
 * A file descriptor is basically just a generic COM object reference;
 * for efficiency it can also hold cached refs to specific interfaces.
 */
struct fd {
	/*
	 * This is the primary object reference in the file descriptor;
	 * it is non-null if the fd is in use, or null if not.
	 */
	struct oskit_iunknown	*obj;

	/*
	 * Each of these can only be non-null if obj is non-null;
	 * if they are non-null, they are simply cached references
	 * to particular interfaces previously queried for on 'obj'.
	 * If any new cached interface pointers are added here,
	 * be sure and update fd_free() to release them.
	 */
	struct oskit_stream	*stream;
	struct oskit_posixio	*posixio;
	struct oskit_openfile	*openfile;
	struct oskit_socket	*socket;
	struct oskit_ttystream	*ttystream;
	struct oskit_asyncio	*asyncio;
	struct oskit_bufio	*bufio;

	/* flags for fcntl */
	oskit_u32_t		flags;

	/*
	 * For select, need to know the select flags per fd, and the
	 * listener object.
	 */
	oskit_s32_t		selecting_on;
	struct oskit_listener	*listener;
	void			*select_handle;

#ifdef THREAD_SAFE
	/*
	 * Each entry in the fd_array is protected by its own mutex
	 * to prevent concurrent access. Note that because of signals
	 * and interrupts, it must be a recursive mutex.
	 */
	pthread_mutex_t		*lock;

	/*
	 * Read/Write locking is accomplished using the above mutex,
	 * and this condition variable. As with above, we must support
	 * recursive locking, so use counts to determine recursion level.
	 */
	pthread_cond_t		*readcond;
	pthread_cond_t		*writecond;
	pthread_t		reader;
	int			readcount;
	int			readwait;
	pthread_t		writer;
	int			writecount;
	int			writewait;
	spin_lock_t		spinlock;

#ifdef  DEBUG
	/*
	 * A sanity check.
	 */
	pthread_t		creator;
#endif
#endif
};
typedef struct fd fd_t;

/* This is the array in which we keep the file descriptors */
extern fd_t *fd_array;
extern int   fd_arraylen;


OSKIT_BEGIN_DECLS

/*** Internal functions ***/
/*
 * Lock to protect the fd_array.
 */
#ifdef THREAD_SAFE
extern pthread_mutex_t		libc_fd_array_mutex;
#define fd_array_lock()		pthread_mutex_lock(&libc_fd_array_mutex);
#define fd_array_unlock()	pthread_mutex_unlock(&libc_fd_array_mutex);
#else
#define fd_array_lock()
#define fd_array_unlock()
#endif

/*
 * Protect each fd entry with its own spinlock.
 */
#ifdef THREAD_SAFE
static inline void fd_lock(int fd) 
{ 
	while (! spin_try_lock(&fd_array[(fd)].spinlock))	
		sched_yield();					
}

static inline int fd_trylock(fd) 
{
	return (spin_try_lock(&fd_array[(fd)].spinlock));
}

static inline void fd_unlock(fd) 
{ 
	spin_unlock(&fd_array[(fd)].spinlock); 
}

#else
static inline void fd_lock(int fd) 
{ 
}

static inline int fd_trylock(fd) 
{
        return 1;
}

static inline void fd_unlock(fd) 
{ 
}
#endif

/*
 * Define the access locks. The access lock is requested when the fd
 * lock is already held. After granting the requested access lock, the
 * fd lock is released. When giving up the access lock, the fd lock
 * should not be held.
 */

#define FD_READ			0x1
#define FD_WRITE		0x2
#define FD_RDWR			(FD_READ|FD_WRITE)

#ifdef THREAD_SAFE
int	fd_access_rawlock(int, int, int);
void	fd_access_unlock(int, int);

static inline int	fd_access_lock(int fd, int t)
{
        return fd_access_rawlock(fd,t,1);
}
static inline int	fd_access_trylock(int fd, int t)
{
        return fd_access_rawlock(fd,t,0);
}
#else
static inline int	fd_access_lock(int fd, int t)
{
        return 1;
}
static inline int	fd_access_trylock(int fd, int t)
{
        return 1;
}
static inline void	fd_access_unlock(int fd, int t)
{
}
#endif /* !THREAD_SAFE */

/*
 * Initialize the fd layer.
 */
void	fd_init(void);

/*
 * Allocate a new file descriptor and return the descriptor number.
 * The descriptor number allocated will be at least min_fd
 * (this is to support F_DUPFD; just pass 0 to allocate any descriptor).
 * Returns -1 and errno set correctly if no fd can be allocated.
 * FD layer must be locked throughout.
 */
int	fd_alloc(oskit_iunknown_t *obj, int min_fd);

/*
 * Free a file descriptor by releasing and zeroing all its object references.
 * FD layer must be locked throughout.
 */
int	fd_free(int fd);

/*
 * test whether fd is invalid or not open
 */

#define FD_BAD(fd) (fd < 0 || fd >= fd_arraylen || fd_array[fd].obj == 0)

/*
 * Check validity of an fd, and lock it. Then check that there is an
 * actual com object associated with. The lock is exclusive; nothing
 * else can get this fd for any reason. Once the caller determines that
 * the fd is valid, it will indicate what kind of lock it wants (read
 * or write). This seems valid, since we would have to lock the descriptor
 * anyway to muck with it, whether a read or write lock is requested.
 */
static inline int fd_check(int fd)
{
	if (fd < 0 || fd >= fd_arraylen) {		
		errno = EBADF;				
		return -1;				
	}						
	fd_lock(fd);					
	if (fd_array[fd].obj == 0) {			
		fd_unlock(fd);				
		errno = EBADF;				
		return -1;				
	}
        return 0;
}


/*
 * check that the underlying object supports a particular interface,
 * querying for it if needed
 */
#define FD_HAS_INTERFACE(fd, interface)					\
	(fd_array[fd].interface != 0 ||					\
	    oskit_iunknown_query(fd_array[fd].obj,			\
			&oskit_##interface##_iid,			\
			(void**)&fd_array[fd].interface) == 0) 		\

#define FD_CHECK_INTERFACE(fd, interface, error)			\
	if (fd_check(fd)) return -1;					\
	if (!FD_HAS_INTERFACE(fd, interface)) {				\
		errno = error;						\
		return -1;						\
	}

/*
 * Verify that a *valid* file descriptor's underlying object supports a
 * particular interface.  If the macro completes (as opposed to returning
 * an error), then the appropriate cached interface pointer is valid.
 * In the THREAD_SAFE version, the requested access lock is taken first.
 *
 */
static inline int fd_check_stream(int fd)
{
        FD_CHECK_INTERFACE(fd, stream, EBADF);
        return 0;
}
static inline int fd_check_openfile(int fd)
{
        FD_CHECK_INTERFACE(fd, openfile, EBADF);
        return 0;
}
static inline int fd_check_socket(int fd)
{
        FD_CHECK_INTERFACE(fd, socket, ENOTSOCK);
        return 0;
}
static inline int fd_check_ttystream(int fd)
{
        FD_CHECK_INTERFACE(fd, ttystream, ENOTTY);
        return 0;
}
static inline int fd_check_asyncio(int fd)
{
        FD_CHECK_INTERFACE(fd, ttystream, ENOTTY);
        return 0;
}

/*
 * a posixio can be obtained using two different ways: it is either
 * supported by the object itself, or, if the object supports the openfile
 * interface, it may be supported by the corresponding file object
 */
static inline int fd_check_posixio(int fd)
{
	if (fd_check(fd))
		return -1;							
	if (!FD_HAS_INTERFACE(fd, posixio)) {				
		if (!FD_HAS_INTERFACE(fd, openfile) ||			
		    oskit_openfile_getfile(fd_array[fd].openfile, 	
		   (struct oskit_file **)&fd_array[fd].posixio) != 0)	
		{							
			fd_unlock(fd);					
			errno = EBADF;					
			return -1;					
		}							
	}
        return 0;
}


OSKIT_END_DECLS

#endif /* _OSKIT_C_FD_H_ */
