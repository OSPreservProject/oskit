/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * A simple bounded queue of characters presented as a COM stream.
 */

#include <oskit/com/charqueue.h>
#include <oskit/com/stream.h>
#include <oskit/io/asyncio.h>
#include <oskit/com/listener_mgr.h>
#include <oskit/error.h>

#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct char_queue_stream {
	oskit_stream_t streami;
	oskit_asyncio_t asyncioi;

	unsigned int refs;

	unsigned int flags;
	oskit_size_t size;

	struct listener_mgr *read_listeners, *write_listeners;

	oskit_size_t head, tail;
	unsigned char buf[0];
};

#define queue_init(q) 	((q)->head = (q)->tail = 0)
#define queue_next(q,i)	(((i) + 1) % (q)->size)
#define queue_full(q) 	(queue_next((q),(q)->tail) == q->head)
#define queue_empty(q)	((q)->head == (q)->tail)


static OSKIT_COMDECL
query(oskit_stream_t *si, const struct oskit_guid *iid, void **out_ihandle)
{
	struct char_queue_stream *s = (struct char_queue_stream*)si;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0) {
		s->refs++;
		*out_ihandle = &s->streami;
		return 0;
	}
	if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
		s->refs++;
		*out_ihandle = &s->asyncioi;
		return 0;
	}

	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
addref(oskit_stream_t *si)
{
	struct char_queue_stream *s = (struct char_queue_stream*)si;
	return ++s->refs;
}

static OSKIT_COMDECL_U
release(oskit_stream_t *si)
{
	struct char_queue_stream *s = (struct char_queue_stream*)si;
	if (--s->refs > 0)
		return s->refs;
	if (s->read_listeners)
		oskit_destroy_listener_mgr(s->read_listeners);
	if (s->write_listeners)
		oskit_destroy_listener_mgr(s->write_listeners);
	sfree(s, sizeof *s - sizeof s->buf + s->size);
	return 0;

}

static OSKIT_COMDECL
read(oskit_stream_t *si, void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct char_queue_stream *s = (struct char_queue_stream*)si;
	int was_full;

	if (s->tail == s->head)
		return OSKIT_EWOULDBLOCK;

	was_full = queue_full(s);

	if (s->head > s->tail) {
		oskit_size_t copy = len;
		if (copy > s->size - s->head)
			copy = s->size - s->head;
		memcpy(buf, &s->buf[s->head], copy);
		s->head = (s->head + copy) % s->size;
		*out_actual = copy;
	}
	else
		*out_actual = 0;

	if (s->head < s->tail) {
		oskit_size_t copy = len - *out_actual;
		if (copy > s->tail - s->head)
			copy = s->tail - s->head;
		memcpy(buf, &s->buf[*out_actual + s->head], copy);
		s->head += copy;
		*out_actual += copy;
	}

	if (was_full && s->write_listeners)
		oskit_listener_mgr_notify(s->write_listeners);

	return 0;
}

static OSKIT_COMDECL
write(oskit_stream_t *si, const void *buf, oskit_u32_t len,
      oskit_u32_t *out_actual)
{
	struct char_queue_stream *s = (struct char_queue_stream*)si;
	int notify = s->read_listeners && queue_empty(s);

	*out_actual = 0;
	do {
		if (s->tail >= s->head) {
			oskit_size_t copy = len;
			if (copy > s->size - s->tail)
				copy = s->size - s->tail;
			memcpy(&s->buf[s->tail], buf, copy);
			s->tail = (s->tail + copy) % s->size;
			buf += copy;
			len -= copy;
			*out_actual += copy;
		}
		if (len > 0 && s->tail < s->head) {
			oskit_size_t copy = len;
			if (copy > s->head - s->tail)
				copy = s->head - s->tail;
			memcpy(&s->buf[s->tail], buf, copy);
			s->tail = (s->tail + copy) % s->size;
			buf += copy;
			len -= copy;
			*out_actual += copy;
		}

		if (len == 0)
			break;
		if (s->tail != s->head)
			continue;

		switch (s->flags) {
		case OSKIT_CHARQUEUE_FULL_ERROR:
			if (*out_actual == 0)
				return OSKIT_EWOULDBLOCK;
			break;
		case OSKIT_CHARQUEUE_FULL_DROP:
			*out_actual += len;
			break;
		case OSKIT_CHARQUEUE_FULL_REPLACE:
			if (len < s->size - s->head)
				s->head += len;
			else
				s->head = 0;
			continue;
		}
	} while (0);

	if (notify && !queue_empty(s))
		oskit_listener_mgr_notify(s->read_listeners);

	return 0;
}

static OSKIT_COMDECL
seek(oskit_stream_t *si, oskit_s64_t ofs, oskit_seek_t whence,
		  oskit_u64_t *out_newpos)
{
	return OSKIT_ESPIPE;
}

static OSKIT_COMDECL
setsize(oskit_stream_t *si, oskit_u64_t new_size)
{
	return OSKIT_EINVAL;
}

static OSKIT_COMDECL
copyto(oskit_stream_t *si, oskit_stream_t *dst,
	oskit_u64_t size, oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;	/*XXX*/
}

static OSKIT_COMDECL
commit(oskit_stream_t *si, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
revert(oskit_stream_t *si)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
lock_region(oskit_stream_t *si, oskit_u64_t offset,
			 oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
unlock_region(oskit_stream_t *si, oskit_u64_t offset,
			   oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
stat(oskit_stream_t *si, oskit_stream_stat_t *out_stat,
		  oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;	/*XXX*/
}

static OSKIT_COMDECL
clone(oskit_stream_t *si, oskit_stream_t **out_stream)
{
	return OSKIT_E_NOTIMPL;	/*XXX*/
}

static struct oskit_stream_ops char_queue_stream_ops = {
	query, addref, release,
	read, write, seek, setsize,
	copyto,
	commit, revert, lock_region, unlock_region, stat, clone
};


/*
 **********************************************************************
 * Async IO interface,
 */
static OSKIT_COMDECL
asyncio_query(oskit_asyncio_t *f,
		   const struct oskit_guid *iid, void **out_ihandle)
{
	struct char_queue_stream *s = (void *) (f - 1);

	return query(&s->streami, iid, out_ihandle);
}

static OSKIT_COMDECL_U
asyncio_addref(oskit_asyncio_t *f)
{
	struct char_queue_stream *s = (void *) (f - 1);

	return addref(&s->streami);
}

static OSKIT_COMDECL_U
asyncio_release(oskit_asyncio_t *f)
{
	struct char_queue_stream *s = (void *) (f - 1);

	return release(&s->streami);
}


static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
	struct char_queue_stream *s = (void *) (f - 1);

	if (queue_empty(s))
		return OSKIT_ASYNCIO_WRITABLE;
	if (queue_full(s))
		return OSKIT_ASYNCIO_READABLE;
	return OSKIT_ASYNCIO_READABLE|OSKIT_ASYNCIO_WRITABLE;
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
asyncio_add_listener(oskit_asyncio_t *f,
		     struct oskit_listener *l, oskit_s32_t mask)
{
	struct char_queue_stream *s = (void *) (f - 1);

	if (mask & OSKIT_ASYNCIO_READABLE) {
		if (!s->read_listeners) {
			s->read_listeners = oskit_create_listener_mgr
				((oskit_iunknown_t *)f);
			if (!s->read_listeners)
				return OSKIT_E_OUTOFMEMORY;
		}
		oskit_listener_mgr_add(s->read_listeners, l);
	}

	if (mask & OSKIT_ASYNCIO_WRITABLE) {
		if (!s->write_listeners) {
			s->write_listeners = oskit_create_listener_mgr
				((oskit_iunknown_t *)f);
			if (!s->write_listeners)
				return OSKIT_E_OUTOFMEMORY;
		}
		oskit_listener_mgr_add(s->write_listeners, l);
	}

        return asyncio_poll(f);
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l)
{
	struct char_queue_stream *s = (void *) (f - 1);

	/*
	 * we don't know where was added - if at all - so let's check
	 * both lists
	 */
	if (oskit_listener_mgr_remove(s->read_listeners, l) &&
	    oskit_listener_mgr_remove(s->write_listeners, l))
		return OSKIT_E_INVALIDARG;

	return 0;
}

static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
	struct char_queue_stream *s = (void *) (f - 1);

	if (s->head == s->tail)
		return 0;
	if (s->head < s->tail)
		return s->tail - s->head;
	return s->size - s->head + s->tail;
}

static struct oskit_asyncio_ops asyncio_ops =
{
	asyncio_query,
	asyncio_addref,
	asyncio_release,
	asyncio_poll,
	asyncio_add_listener,
	asyncio_remove_listener,
	asyncio_readable
};


oskit_stream_t *
oskit_charqueue_create(oskit_size_t size,
		       unsigned int flags)
{
	struct char_queue_stream *s;

	s = smalloc(sizeof *s - sizeof s->buf + size);
	if (s == 0)
		return 0;

	s->streami.ops = &char_queue_stream_ops;
	s->asyncioi.ops = &asyncio_ops;
	s->refs = 1;
	s->flags = flags;
	s->size = size;
	s->head = s->tail = 0;
	s->read_listeners = s->write_listeners = 0;

	return &s->streami;
}
