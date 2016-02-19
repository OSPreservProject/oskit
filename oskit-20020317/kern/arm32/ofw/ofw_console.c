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
 * IStream wrapper for OFW console
 */

#include <oskit/com/stream.h>
#include <oskit/arm32/ofw/ofw_cons.h>
#include <oskit/arm32/ofw/ofw_console.h>

struct strm {
	oskit_stream_t strmi;
	int refs;
};

static OSKIT_COMDECL
query(oskit_stream_t *si, const struct oskit_guid *iid, void **out_ihandle)
{
	struct strm *s = (struct strm*)si;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0) {
		s->refs++;
		*out_ihandle = &s->strmi;
		return 0;
	}

	*out_ihandle = 0;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
add_ref(oskit_stream_t *si)
{
	struct strm *s = (struct strm*)si;

	return ++s->refs;
}

static OSKIT_COMDECL_U
release(oskit_stream_t *si)
{
	struct strm *s = (struct strm*)si;
	int newcount = --s->refs;

	return newcount;
}

static OSKIT_COMDECL
read(oskit_stream_t *si, void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	int i;
	for(i=0; i < len; ++i) {
		((char*)buf)[i] = ofw_cons_getchar();
	}
	*out_actual = i;
	return 0;
}

static OSKIT_COMDECL
write(oskit_stream_t *si, const void *buf, oskit_u32_t len,
      oskit_u32_t *out_actual)
{
	int i=0;
	for(i=0; i < len; ++i) {
		ofw_cons_putchar(((char*)buf)[i]);
	}
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

static
struct oskit_stream_ops ops = {
	query, add_ref, release,
	read, write, seek, setsize, 
	copyto,
	commit, revert, lock_region, unlock_region, stat, clone
};

static struct strm ofw_console_impl;

oskit_error_t
ofw_console_init(struct termios *com_params, oskit_stream_t **out_stream)
{
	if (ofw_console_impl.refs != 0)
		return 0;
	
	ofw_console_impl.strmi.ops = &ops;
	ofw_console_impl.refs = 1;

	ofw_cons_init(com_params);

	*out_stream = &ofw_console_impl.strmi;
	return 0;
}


/*
 * Temp hack so printf works right from the start.
 */
#include <oskit/arm32/shark/base_console.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>

void
ofw_base_console_init(void)
{
	ofw_console_init(&base_cooked_termios, &console);
}
