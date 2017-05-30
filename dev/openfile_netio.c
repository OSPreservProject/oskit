/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Generic oskit_openfile adapter for netio devices
 * in an oskit device environment.
 */
#include <oskit/dev/dev.h>

#include <oskit/fs/openfile.h>
#include <oskit/io/netio.h>
#include <oskit/com/queue.h>
#include <oskit/error.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>

struct openfile {
	oskit_openfile_t 	ofi;
	oskit_u32_t 		count;
	oskit_oflags_t		flags;
	oskit_netio_t		*send_nio;
	oskit_netio_t		*recv_nio;
	oskit_file_t		*file;
	oskit_queue_t		*queue;
	oskit_listener_t	*notify_on_dump;
	osenv_sleeprec_t		sr;
};

#if 0
#define DMARK osenv_log(OSENV_LOG_DEBUG, __FILE__ ":%d: " __FUNCTION__ "\n", __LINE__)
#else
#define DMARK ((void)0)
#endif

static oskit_error_t
openfile_query(oskit_openfile_t *f,
	       const oskit_iid_t *iid, void **out_ihandle)
{
	struct openfile *of = (void *) f;
	osenv_assert(of->count);

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_openfile_iid, sizeof(*iid)) == 0) {
                *out_ihandle = of;
                ++of->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U 
openfile_addref(oskit_openfile_t *f)
{
	struct openfile *of = (void *) f;
	osenv_assert (of->count);
	return ++of->count;
}

static oskit_u32_t
openfile_release(oskit_openfile_t *f)
{
	struct openfile *of = (void *) f;

	osenv_assert(of->count);

	if (--of->count)
		return of->count;

	oskit_queue_release(of->queue);
	if (of->file)
		oskit_file_release(of->file);
	if (of->notify_on_dump)
		oskit_listener_addref(of->notify_on_dump);
	osenv_mem_free(of, 0, sizeof *of);
	return 0;
}


/*** Operations inherited from oskit_stream interface ***/

static OSKIT_COMDECL
openfile_read(oskit_openfile_t *f, void *buf, oskit_u32_t len,
	      oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;
	oskit_bufio_t	*p;
	oskit_error_t   rc;
	DMARK;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_WRONLY)
		return OSKIT_EBADF;

	/* XXX: handle "O_NONBLOCK" here sometime ... */

	/* wait on a condition variable... */
	osenv_intr_disable();
	while (oskit_queue_size(of->queue) == 0) {
		osenv_sleep_init(&of->sr);
		osenv_intr_enable();
		osenv_sleep(&of->sr);
		osenv_intr_disable();
	}

	rc = oskit_queue_dequeue(of->queue, &p);
	osenv_assert(rc == sizeof p);

	osenv_intr_enable();

	/* copy it out and release it */
	rc = oskit_bufio_read(p, buf, 0, len, out_actual);
	oskit_bufio_release(p);

	return rc;
}

static OSKIT_COMDECL
openfile_write(oskit_openfile_t *f, const void *buf,
	       oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct openfile *of = (void *) f;
	oskit_error_t rc;
	oskit_bufio_t *b;

	if ((of->flags & OSKIT_O_ACCMODE) == OSKIT_O_RDONLY)
		return OSKIT_EBADF;

	b = oskit_bufio_create(len);
	rc = oskit_bufio_write(b, buf, 0, len, out_actual);
	if (!rc) 
		rc = oskit_netio_push(of->send_nio, b, len);
	oskit_bufio_release(b);
	return rc;
}

static OSKIT_COMDECL
openfile_seek(oskit_openfile_t *f, oskit_s64_t offset,
	      oskit_seek_t whence, oskit_u64_t *out_newpos)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_setsize(oskit_openfile_t *f, oskit_u64_t new_size)
{
	return OSKIT_E_NOTIMPL;
}


static OSKIT_COMDECL
openfile_copy_to(oskit_openfile_t *f, oskit_stream_t *dst, oskit_u64_t size,
		 oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_commit(oskit_openfile_t *f, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_revert(oskit_openfile_t *f)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_lock_region(oskit_openfile_t *f, oskit_u64_t offset, 
		     oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_unlock_region(oskit_openfile_t *f, oskit_u64_t offset, 
		       oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_stat(oskit_openfile_t *f, oskit_stream_stat_t *out_stat,
	      oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
openfile_clone(oskit_openfile_t *f, oskit_openfile_t **out_stream)
{
	return OSKIT_E_NOTIMPL;
}

/*** Operations specific to oskit_openfile ***/

static OSKIT_COMDECL
openfile_getfile(oskit_openfile_t *f, struct oskit_file **out_file)
{
	struct openfile *of = (void *) f;

	if (!of->file)
		return OSKIT_E_NOTIMPL;

	oskit_file_addref(of->file);
	*out_file = of->file;
	return 0;
}

static struct oskit_openfile_ops openfile_ops = {
    openfile_query,
    openfile_addref,
    openfile_release,
    openfile_read,
    openfile_write,
    openfile_seek,
    openfile_setsize,
    openfile_copy_to,
    openfile_commit,
    openfile_revert,
    openfile_lock_region,
    openfile_unlock_region,
    openfile_stat,
    openfile_clone,
    openfile_getfile
};

/*
 * I think we can ignore pkt_size
 */
static oskit_error_t 
net_receive(void *data, oskit_bufio_t *b, oskit_size_t pkt_size)
{
	struct openfile *of = data;
	oskit_error_t rc = oskit_queue_enqueue(of->queue, &b, sizeof b);

	if (rc) {
		if (of->notify_on_dump)
			oskit_listener_notify(of->notify_on_dump, 
				(oskit_iunknown_t *)b);
	} else {
		oskit_bufio_addref(b);
		osenv_wakeup(&of->sr, OSENV_SLEEP_WAKEUP);
	}
	return 0;
}

/*
 * Sole user entry point for this module.
 *
 * Please refer to oskit/dev/soa.h for more explanation.
 *
 * The style in oskit/fs/soa.h would have called this
 * oskit_soa_openfile_from_netio, but we look to the future.
 */
OSKIT_COMDECL 
create_openfile_from_netio(oskit_netio_t *(*exchange)(void *, oskit_netio_t *f),
		void *exchange_arg,
	        int queuelength,
		oskit_oflags_t flags,
		oskit_listener_t        *notify_on_dump,
		struct oskit_file	*file,
		struct oskit_openfile **out_openfile)
{
	struct openfile *of;

	DMARK;
	if (flags &~ (OSKIT_O_ACCMODE|OSKIT_O_APPEND))
		return OSKIT_ENOTSUP;

	DMARK;
	of = osenv_mem_alloc (sizeof *of, 0, 0);
	if (! of) {
		return OSKIT_ENOMEM;
	}
	memset(of, 0, sizeof *of);

	of->queue = create_bounded_queue_with_fixed_size_items(
		queuelength, 
		sizeof (oskit_bufio_t *), 
		0, /* don't need notifications after all */
		0  /* don't drop last */);

	if (!of->queue) {
		osenv_mem_free (of, 0, sizeof *of);
		return OSKIT_ENOMEM;
	}

	of->ofi.ops = &openfile_ops;
	of->count = 1;
	of->flags = flags;
	if ((of->file = file) != NULL)
		oskit_file_addref(file);

	if ((of->notify_on_dump = notify_on_dump) != NULL)
		oskit_listener_addref(notify_on_dump);

	of->recv_nio = oskit_netio_create(net_receive, of);
	of->send_nio = exchange(exchange_arg, of->recv_nio);
	*out_openfile = &of->ofi;
	return 0;
}
