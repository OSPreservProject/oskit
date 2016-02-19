/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Pipes. A pipe is two stream-like objects connected together via a
 * couple of shared buffers. The implementation is truely UNIDIRECTIONAL!
 * If you try to use them bidirectionally, it will almost certainly
 * deadlock. If you need something so fancy as true bidirectional pipes,
 * I suggest using socketpair() in the C/Posix library instead (although
 * that requires linking with the network stack).
 */
#include <oskit/com/pipe.h>
#include <oskit/com/stream.h>
#include <oskit/io/asyncio.h>
#include <oskit/com/services.h>
#include <oskit/com/lock_mgr.h>
#include <oskit/com/lock.h>
#include <oskit/com/condvar.h>
#include <oskit/com/listener_mgr.h>
#include <oskit/queue.h>
#include <oskit/error.h>
#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/*
 * A pipe has an input buffer and a connection to the other side of
 * the pipe. A write to a pipe writes to the input buffer on the other
 * side, and a read comes from the local side. In either case, there
 * must be resource available or the caller will sleep until there is
 * sufficient resource.
 *
 * Pipe semantics dictate that the pipe stays valid even after one
 * side has closed so that the other side can read the remaining
 * buffered data. Both sides of the pipe will be deallocated only
 * when both counts go to zero.
 *
 * Note that the lock/condvar pair is *shared* between both sides of
 * the pipe. 
 */
struct pipe {
	oskit_pipe_t		pipei;		/* Pipe COM interface */
	oskit_asyncio_t		pipea;		/* Asyncio COM interface */
	oskit_u32_t		count;		/* Reference count */
	struct pipe	       *sister;		/* Other side of the pipe */
	oskit_u32_t		flags;		/* Flags */
	queue_head_t		pipeq;		/* Queue of data buffers */
	int			dataready;	/* Amount of data ready */
	oskit_lock_t	       *lock;		/* Thread lock (shared) */
	oskit_condvar_t	       *condvar;	/* Thread condvar */
	struct listener_mgr    *readers;	/* listeners for asyncio */
	struct listener_mgr    *writers;	/* listeners for asyncio */
};

/*
 * Pipe buffers are queues of mbuf like thingies, each with an associated
 * buffer, offset and size.
 */
struct pbuf {
	queue_chain_t		chain;		/* Queueing element */
	int			size;		/* Original size for dealloc */
	int			count;		/* Amount of data ready */
	char		       *bufp;		/* Current pointer into data */
	char			data[0];	/* Buffer space for data */
};

/*
 * The maximum size that a pipe buffer can grow to.
 */
/*#define MAXPIPEBUF		(1024 * 64) */
#define MAXPIPEBUF		512

/*
 * Flags.
 */
#define	PIPE_CLOSED		0x01
#define PIPE_SLEEPREAD		0x02
#define PIPE_SLEEPWRITE		0x04
#define PIPE_WIDOWED		0x08
#define PIPE_READSEL		0x10
#define PIPE_WRITESEL		0x20

/*
 * Thread safe locking. Not sure it makes any sense to use pipes in
 * single threaded application, but whatever ...
 */
#define PLOCK(pipe)		if (pipe->lock) oskit_lock_lock(pipe->lock)
#define PUNLOCK(pipe)		if (pipe->lock) oskit_lock_unlock(pipe->lock)
#define PWAIT(pipe) \
	if (pipe->condvar) oskit_condvar_wait(pipe->condvar, pipe->lock)
#define PSIGNAL(pipe) \
	if (pipe->condvar) oskit_condvar_signal(pipe->condvar)

/*
 * Stream operators
 */
static oskit_error_t
pipe_query(oskit_pipe_t *f, const oskit_iid_t *iid, void **out_ihandle)
{
	struct pipe *pipe = (struct pipe *) f;

	assert(pipe->count);	

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_pipe_iid, sizeof(*iid)) == 0) {
                *out_ihandle = pipe;
                ++pipe->count;
                return 0;
        }

        if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &pipe->pipea;
                ++pipe->count;
                return 0;
	}

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U 
pipe_addref(oskit_pipe_t *f)
{
	struct pipe *pipe = (struct pipe *) f;
	
	assert(pipe->count);	
	return ++pipe->count;
}

static void
pipe_free(struct pipe *pipe)
{
	struct pbuf	*pbuf;

	/*
	 * Free all the buffers and then free the pipe object.
	 */
	while (!queue_empty(&pipe->pipeq)) {
		queue_remove_first(&pipe->pipeq, pbuf, struct pbuf *, chain);
		sfree(pbuf, sizeof(*pbuf) + pbuf->size);
	}
	if (pipe->lock)
		oskit_lock_release(pipe->lock);
	if (pipe->condvar)
		oskit_condvar_release(pipe->condvar);
	oskit_destroy_listener_mgr(pipe->readers);
	oskit_destroy_listener_mgr(pipe->writers);
	free(pipe);
}

static oskit_u32_t
pipe_release(oskit_pipe_t *f)
{
	struct pipe     *pipe = (struct pipe *) f;
	struct pipe     *psis = pipe->sister;
	int		newcount;

	PLOCK(pipe);
	
	assert(pipe->count);

	if ((newcount = --pipe->count) == 0) {
		/*
		 * Both sides of the pipe must be closed before it
		 * can be deallocated.
		 */
		if (psis->flags & PIPE_CLOSED) {
			pipe_free(pipe->sister);
			pipe_free(pipe);
			return 0;
		}

		pipe->flags |= PIPE_CLOSED;
		psis->flags |= PIPE_WIDOWED;

		if (psis->flags & (PIPE_SLEEPREAD|PIPE_SLEEPWRITE))
			PSIGNAL(psis);
	}
	PUNLOCK(pipe);
	return newcount;
}

/*** Operations inherited from oskit_stream interface ***/

static OSKIT_COMDECL
pipe_read(oskit_pipe_t *f, void *buf, oskit_u32_t len,
	  oskit_u32_t *out_actual)
{
	struct pipe     *pipe = (struct pipe *) f;
	struct pipe     *psis = pipe->sister;
	char		*bp   = buf;
	struct pbuf	*pbuf;
	int		count;

	PLOCK(pipe);

	/*
	 * Look for read on closed pipe.
	 */
	if (pipe->flags & PIPE_CLOSED) {
		PUNLOCK(pipe);
		return OSKIT_EBADF;
	}

	*out_actual = 0;
	
	/*
	 * If no data available, sleep until the sister puts some data in.
	 * Look for EOF condition; Nothing to read and pipe is widowed.
	 */
	while (!pipe->dataready) {
		if (pipe->flags & PIPE_WIDOWED) {
			PUNLOCK(pipe);
			return 0;
		}
		
		pipe->flags |= PIPE_SLEEPREAD;
		PWAIT(pipe);
		pipe->flags &= ~PIPE_SLEEPREAD;
	}
		
	/*
	 * Iterate through the buffers, copying out as much data as
	 * possible, up to the maximum specified by the caller.
	 */
	while (!queue_empty(&pipe->pipeq) && len > 0) {
		pbuf = (struct pbuf *) queue_first(&pipe->pipeq);

		/*
		 * For a buffer, copyout only as much data as can fit.
		 */
		count = (pbuf->count > len) ? len : pbuf->count;

		memcpy(bp, pbuf->bufp, count);
		
		pbuf->bufp  += count;
		pbuf->count -= count;
		
		*out_actual += count;
		len         -= count;
		bp          += count;

		/*
		 * If the buffer is empty, dealloc.
		 */
		if (pbuf->count == 0) {
			queue_remove(&pipe->pipeq, pbuf, struct pbuf *, chain);
			sfree(pbuf, sizeof(*pbuf) + pbuf->size);
		}
		
		pipe->dataready -= count;

		/*
		 * Look for a select sleeper on the write side.
		 */
		if (psis->flags & PIPE_WRITESEL) {
			PUNLOCK(pipe);
			oskit_listener_mgr_notify(psis->writers);
			PLOCK(pipe);
		}

		/*
		 * If the sister was waiting to put more data in, then
		 * wake it up.
		 */
		if (psis->flags & PIPE_SLEEPWRITE) {
			psis->flags &= ~PIPE_SLEEPWRITE;
			PSIGNAL(psis);
		}
	}

	PUNLOCK(pipe);
	return 0;
}

static OSKIT_COMDECL
pipe_write(oskit_pipe_t *f, const void *buf,
	   oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct pipe     *pipe = (struct pipe *) f;
	struct pipe     *psis = pipe->sister;
	char		*bp   = (char *) buf;
	struct pbuf	*pbuf;
	int		count;

	PLOCK(pipe);

	/*
	 * Look for write on closed pipe.
	 */
	if (pipe->flags & PIPE_CLOSED) {
		PUNLOCK(pipe);
		return OSKIT_EBADF;
	}

	/*
	 * Look for a broken pipe.
	 */
	if (pipe->flags & PIPE_WIDOWED) {
		PUNLOCK(pipe);
		return OSKIT_EPIPE;
	}
	*out_actual = 0;

	/*
	 * Loop, stuffing stuff into the other side until we run out of
	 * stuff, or reach the maximum allowed. If the other side fills,
	 * up, must sleep until it wakes this side up again. Each time
	 * data is stuffed into the other side, be sure to wake it up.
	 */
	while (len > 0) {
		/*
		 * Is there room for the data? If not, must wait.
		 */
		while (psis->dataready >= MAXPIPEBUF) {
			pipe->flags |= PIPE_SLEEPWRITE;
			PWAIT(pipe);
			pipe->flags &= ~PIPE_SLEEPWRITE;
		}
		
		if (MAXPIPEBUF - psis->dataready > len)
			count = len;
		else
			count = MAXPIPEBUF - psis->dataready;

		if ((pbuf = smalloc(sizeof(*pbuf) + count)) == 0)
			panic("pipe_write: Out of memory");

		memcpy(pbuf->data, bp, count);
		
		pbuf->bufp   = pbuf->data;
		pbuf->count  = count;
		pbuf->size   = count;
		
		*out_actual += count;
		len         -= count;
		bp          += count;

		queue_enter(&psis->pipeq, pbuf, struct pbuf *, chain);
		
		psis->dataready += count;

		/*
		 * Look for a select sleeper on the read side.
		 */
		if (psis->flags & PIPE_READSEL) {
			PUNLOCK(pipe);
			oskit_listener_mgr_notify(psis->readers);
			PLOCK(pipe);
		}

		/*
		 * Wakeup the sister side if it was waiting for data.
		 */
		if (psis->flags & PIPE_SLEEPREAD) {
			psis->flags &= ~PIPE_SLEEPREAD;
			PSIGNAL(psis);
		}
	}
	
	PUNLOCK(pipe);
	return 0;
}

static OSKIT_COMDECL
pipe_seek(oskit_pipe_t *f, oskit_s64_t offset,
	  oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	return OSKIT_ESPIPE;
}

static OSKIT_COMDECL
pipe_setsize(oskit_pipe_t *f, oskit_u64_t new_size)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_copy_to(oskit_pipe_t *f, oskit_stream_t *dst, oskit_u64_t size,
	     oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_commit(oskit_pipe_t *f, oskit_u32_t commit_flags)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_revert(oskit_pipe_t *f)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_lockregion(oskit_pipe_t *f, oskit_u64_t offset, 
		oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_unlockregion(oskit_pipe_t *f, oskit_u64_t offset, 
		oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_stat(oskit_pipe_t *f, oskit_stream_stat_t *out_stat,
	  oskit_u32_t stat_flags)
{
	return OSKIT_ENOTSUP;
}

static OSKIT_COMDECL
pipe_clone(oskit_pipe_t *f, oskit_pipe_t **out_stream)
{
	return OSKIT_ENOTSUP;
}

static struct oskit_pipe_ops pipe_ops = {
    pipe_query,
    pipe_addref,
    pipe_release,
    pipe_read,
    pipe_write,
    pipe_seek,
    pipe_setsize,
    pipe_copy_to,
    pipe_commit,
    pipe_revert,
    pipe_lockregion,
    pipe_unlockregion,
    pipe_stat,
    pipe_clone,
};

/*
 **********************************************************************
 * Async IO interface,
 */
static OSKIT_COMDECL
pipe_asyncio_query(oskit_asyncio_t *f,
		   const struct oskit_guid *iid, void **out_ihandle)
{
	struct pipe     *pipe = (struct pipe *) (f-1);

	return pipe_query(&pipe->pipei, iid, out_ihandle);
}

static OSKIT_COMDECL_U
pipe_asyncio_addref(oskit_asyncio_t *f)
{
	struct pipe     *pipe = (struct pipe *) (f-1);

	return pipe_addref(&pipe->pipei);
}

static OSKIT_COMDECL_U
pipe_asyncio_release(oskit_asyncio_t *f)
{
	struct pipe     *pipe = (struct pipe *) (f-1);

	return pipe_release(&pipe->pipei);
}

/*
 * return a mask with all conditions that currently apply to that socket
 * must be called with splnet()!
 */
static oskit_u32_t
get_pipe_conditions(struct pipe *pipe)
{
	struct pipe     *psis = pipe->sister;
	oskit_u32_t	res = 0;

	if (pipe->dataready)
                res |= OSKIT_ASYNCIO_READABLE;

	if (pipe->flags & (PIPE_CLOSED|PIPE_WIDOWED))
                res |= OSKIT_ASYNCIO_EXCEPTION;

	if (psis->dataready < MAXPIPEBUF)
		res |= OSKIT_ASYNCIO_WRITABLE;

	return res;
}

/*
 * Poll for currently pending asynchronous I/O conditions.
 * If successful, returns a mask of the OSKIT_ASYNC_IO_* flags above,
 * indicating which conditions are currently present.
 */

static OSKIT_COMDECL
pipe_asyncio_poll(oskit_asyncio_t *f)
{
	struct pipe     *pipe = (struct pipe *) (f-1);
        oskit_u32_t      res  = 0;

	PLOCK(pipe);
	res = get_pipe_conditions(pipe);
	PUNLOCK(pipe);

        return res;
}

/*
 * Add a callback object (a "listener" for async I/O events).
 * When an event of interest occurs on this I/O object
 * (i.e., when one of the three I/O conditions becomes true),
 * all registered listeners will be called.
 * Also, if successful, this method returns a mask
 * describing which of the OSKIT_ASYNC_IO_* conditions are already true,
 * which the caller must check in order to avoid missing events
 * that occur just before the listener is registered.
 */
static OSKIT_COMDECL
pipe_asyncio_add_listener(oskit_asyncio_t *f,
			  struct oskit_listener *l, oskit_s32_t mask)
{
	struct pipe     *pipe = (struct pipe *) (f-1);
	oskit_s32_t	cond;

	PLOCK(pipe);
	cond = get_pipe_conditions(pipe);

	/* for read and exceptional conditions */
	if (mask & (OSKIT_ASYNCIO_READABLE|OSKIT_ASYNCIO_EXCEPTION)) {
		oskit_listener_mgr_add(pipe->readers, l);
		pipe->flags |= PIPE_READSEL;
	}

	/* for write */
	if (mask & OSKIT_ASYNCIO_WRITABLE) {
		oskit_listener_mgr_add(pipe->writers, l);
		pipe->flags |= PIPE_WRITESEL;
	}

	PUNLOCK(pipe);
        return cond;
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
pipe_asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	struct pipe     *pipe = (struct pipe *) (f-1);
	oskit_error_t	rc1, rc2;

	PLOCK(pipe);

	/*
	 * we don't know where was added - if at all - so let's check
	 * both lists
	 *
	 * turn off notifications if no listeners left
	 */
	rc1 = oskit_listener_mgr_remove(pipe->readers, l0);
	if (oskit_listener_mgr_count(pipe->readers) == 0) {
		pipe->flags &= ~PIPE_READSEL;
	}

	rc2 = oskit_listener_mgr_remove(pipe->writers, l0);
	if (oskit_listener_mgr_count(pipe->writers) == 0) {
		pipe->flags &= ~PIPE_WRITESEL;
	}

	PUNLOCK(pipe);

	/* flag error if both removes failed */
	return (rc1 && rc2) ? OSKIT_E_INVALIDARG : 0;	/* is that right ? */
}

/*
 * return the number of bytes that can be read, basically ioctl(FIONREAD)
 */
static OSKIT_COMDECL
pipe_asyncio_readable(oskit_asyncio_t *f)
{
	struct pipe     *pipe = (struct pipe *) (f-1);
	oskit_u32_t	count;

	PLOCK(pipe);
	count = pipe->dataready;
	PUNLOCK(pipe);
	return count;
}

static struct oskit_asyncio_ops pipe_asyncioops =
{
	pipe_asyncio_query,
	pipe_asyncio_addref,
	pipe_asyncio_release,
	pipe_asyncio_poll,
	pipe_asyncio_add_listener,
	pipe_asyncio_remove_listener,
	pipe_asyncio_readable
};

#ifdef KNIT
extern oskit_lock_mgr_t *oskit_lock_mgr;
#define lock_mgr oskit_lock_mgr
#endif

/*
 * Sole user entry point for this module. Create an `oskit_pipe' object.
 */
OSKIT_COMDECL 
oskit_create_pipe(oskit_pipe_t **out_pipe0, oskit_pipe_t **out_pipe1)
{
	struct pipe		*pipe0, *pipe1;
#ifndef KNIT
	oskit_lock_mgr_t	*lock_mgr;
#endif
	oskit_lock_t	        *lock;
	oskit_condvar_t	        *condvar;

	if ((pipe0 = malloc(sizeof(struct pipe))) == 0)
		return OSKIT_ENOMEM;
	
	if ((pipe1 = malloc(sizeof(struct pipe))) == 0) {
		free(pipe0);
		return OSKIT_ENOMEM;
	}

	memset(pipe0, 0, sizeof(*pipe0));
	memset(pipe1, 0, sizeof(*pipe1));

	pipe0->pipei.ops  = &pipe_ops;
	pipe1->pipei.ops  = &pipe_ops;
	pipe0->pipea.ops  = &pipe_asyncioops;
	pipe1->pipea.ops  = &pipe_asyncioops;
	pipe0->count      = 1;
	pipe1->count      = 1;
	pipe0->sister     = pipe1;
	pipe1->sister     = pipe0;
	queue_init(&pipe0->pipeq);
	queue_init(&pipe1->pipeq);
	pipe0->readers    = oskit_create_listener_mgr((oskit_iunknown_t *)
						      &pipe0->pipea);
	pipe0->writers    = oskit_create_listener_mgr((oskit_iunknown_t *)
						      &pipe0->pipea);
	pipe1->readers    = oskit_create_listener_mgr((oskit_iunknown_t *)
						      &pipe1->pipea);
	pipe1->writers    = oskit_create_listener_mgr((oskit_iunknown_t *)
						      &pipe1->pipea);

	/*
	 * See if thread-safe locks are required. Note that I don't think it
	 * makes any sense to use pipes in a single threaded kernel, but
	 * at least the program will link and run. It will probably just
	 * deadlock though.
	 */
#ifndef KNIT
	if (oskit_lookup_first(&oskit_lock_mgr_iid, (void *) &lock_mgr))
		panic("oskit_create_pipe: oskit_lookup_first");
#endif

	if (lock_mgr) {
		if (oskit_lock_mgr_allocate_lock(lock_mgr, &lock))
			panic("oskit_create_pipe: lock_mgr_allocate_lock");

		oskit_lock_addref(lock);
		pipe0->lock = lock;
		pipe1->lock = lock;

		if (oskit_lock_mgr_allocate_condvar(lock_mgr, &condvar))
			panic("oskit_create_pipe: lock_mgr_allocate_condvar");

		pipe0->condvar = condvar;

		if (oskit_lock_mgr_allocate_condvar(lock_mgr, &condvar))
			panic("oskit_create_pipe: lock_mgr_allocate_condvar");

		pipe1->condvar = condvar;
	}

	*out_pipe0 = &pipe0->pipei;
	*out_pipe1 = &pipe1->pipei;
	return 0;
}
