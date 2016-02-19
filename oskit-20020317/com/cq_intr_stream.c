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
 * An adapter to construct a full-fledged stream/asyncio object around
 * a simpler output stream and an interrupt-driven input source, using
 * proper osenv-based blocking behavior and interrupt protection.
 */

#include <oskit/com/stream.h>
#include <oskit/io/asyncio.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/soa.h>
#include <oskit/com/charqueue.h>

#include <malloc.h>
#include <stddef.h>
#include <string.h>

struct cq_intr_stream {
	oskit_stream_t streami;
	oskit_asyncio_t asyncioi;

	unsigned int refs;

	oskit_stream_t *output;	/* supplied at creation */
	oskit_stream_t *inq;	/* from oskit_charqueue_create */
	oskit_asyncio_t *inq_aio; /* likewise */

	oskit_osenv_intr_t *intr;
	oskit_osenv_sleep_t *sleep;
};
#define OSENV(if, call, args...) oskit_osenv_##if##_##call(s->if ,##args)


static OSKIT_COMDECL
query(oskit_stream_t *si, const struct oskit_guid *iid, void **out_ihandle)
{
	struct cq_intr_stream *s = (struct cq_intr_stream*)si;

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
	struct cq_intr_stream *s = (struct cq_intr_stream*)si;
	return ++s->refs;
}

static OSKIT_COMDECL_U
release(oskit_stream_t *si)
{
	struct cq_intr_stream *s = (struct cq_intr_stream*)si;
	if (--s->refs > 0)
		return s->refs;

	/* Caller is responsible for making sure we get no more interrupts! */
	oskit_stream_release(s->output);
	oskit_stream_release(s->inq);
	oskit_asyncio_release(s->inq_aio);
	oskit_osenv_intr_release(s->intr);
	oskit_osenv_sleep_release(s->sleep);

	sfree(s, sizeof *s);

	return 0;
}

static OSKIT_COMDECL
read(oskit_stream_t *si, void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct cq_intr_stream *s = (struct cq_intr_stream*)si;
	oskit_error_t rc;

	OSENV(intr, disable);
	rc = oskit_stream_read(s->inq, buf, len, out_actual);
	OSENV(intr, enable);
	if (rc == OSKIT_EWOULDBLOCK) {
		oskit_sleep_t *sl;
		oskit_listener_t *li;
		rc = OSENV(sleep, create, &sl);
		if (rc)
			return rc;
		li = oskit_sleep_create_listener(sl);
		if (!li) {
			oskit_sleep_release(sl);
			return OSKIT_E_OUTOFMEMORY;
		}
		OSENV(intr, disable);
		rc = oskit_asyncio_add_listener(s->inq_aio, li,
						OSKIT_ASYNCIO_READABLE);
		OSENV(intr, enable);
		if (OSKIT_FAILED(rc)) {
			oskit_listener_release(li);
			return rc;
		}
		if (rc & OSKIT_ASYNCIO_READABLE)
			goto woken;
		do {
			rc = oskit_sleep_sleep(sl);
			if (rc == OSKIT_SLEEP_CANCELED) {
				rc = OSKIT_ECANCELED;
				break;
			}
		woken:
			OSENV(intr, disable);
			rc = oskit_stream_read(s->inq, buf, len, out_actual);
			OSENV(intr, enable);
		} while (rc == OSKIT_EWOULDBLOCK);
		OSENV(intr, disable);
		oskit_asyncio_remove_listener(s->inq_aio, li);
		OSENV(intr, enable);
		oskit_listener_release(li);
		oskit_sleep_release(sl);
	}

	return rc;
}

static OSKIT_COMDECL
write(oskit_stream_t *si, const void *buf, oskit_u32_t len,
      oskit_u32_t *out_actual)
{
	struct cq_intr_stream *s = (struct cq_intr_stream*)si;

	return oskit_stream_write(s->output, buf, len, out_actual);
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
	struct cq_intr_stream *s = (void *) (f - 1);

	return query(&s->streami, iid, out_ihandle);
}

static OSKIT_COMDECL_U
asyncio_addref(oskit_asyncio_t *f)
{
	struct cq_intr_stream *s = (void *) (f - 1);

	return addref(&s->streami);
}

static OSKIT_COMDECL_U
asyncio_release(oskit_asyncio_t *f)
{
	struct cq_intr_stream *s = (void *) (f - 1);

	return release(&s->streami);
}


static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
	struct cq_intr_stream *s = (void *) (f - 1);

	return OSKIT_ASYNCIO_WRITABLE | oskit_asyncio_poll(s->inq_aio);
}

static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f,
		     struct oskit_listener *l, oskit_s32_t mask)
{
	struct cq_intr_stream *s = (void *) (f - 1);
	oskit_s32_t rc;

	if (mask & OSKIT_ASYNCIO_READABLE) {
		rc = oskit_asyncio_add_listener(s->inq_aio, l,
						OSKIT_ASYNCIO_READABLE);
		if (OSKIT_FAILED(rc))
			return rc;
	}
	else
		rc = 0;

	return rc | (mask & OSKIT_ASYNCIO_WRITABLE);
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l)
{
	struct cq_intr_stream *s = (void *) (f - 1);

	OSENV(intr, disable);
	oskit_asyncio_remove_listener(s->inq_aio, l);
	OSENV(intr, enable);
	return 0;
}

static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
	struct cq_intr_stream *s = (void *) (f - 1);
	oskit_s32_t n;

	OSENV(intr, disable);
	n = oskit_asyncio_readable(s->inq_aio);
	OSENV(intr, enable);
	return n;
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


OSKIT_COMDECL
oskit_charqueue_intr_stream_create(oskit_size_t size,
				   oskit_stream_t *stream_for_output,
				   oskit_osenv_intr_t *intr,
				   oskit_osenv_sleep_t *sleep,
				   oskit_stream_t **out_stream,
				   oskit_stream_t **out_upcall_stream)
{
	struct cq_intr_stream *s;
	oskit_error_t rc;

	s = smalloc(sizeof *s);
	if (s == 0)
		return OSKIT_E_OUTOFMEMORY;

	s->inq = oskit_charqueue_create(size, OSKIT_CHARQUEUE_FULL_ERROR);
	if (s->inq == 0) {
		sfree(s, sizeof *s);
		return OSKIT_E_OUTOFMEMORY;
	}
	rc = oskit_stream_query(s->inq, &oskit_asyncio_iid,
				(void **) &s->inq_aio);
	if (rc) {		/* ??? */
		oskit_stream_release(s->inq);
		sfree(s, sizeof *s);
		return rc;
	}

	s->streami.ops = &char_queue_stream_ops;
	s->asyncioi.ops = &asyncio_ops;
	s->refs = 1;
	s->output = stream_for_output;
	oskit_stream_addref(s->output);
	s->intr = intr;
	oskit_osenv_intr_addref(intr);
	s->sleep = sleep;
	oskit_osenv_sleep_addref(sleep);

	*out_upcall_stream = s->inq;
	oskit_stream_addref(s->inq);
	*out_stream = &s->streami;

	return 0;
}
