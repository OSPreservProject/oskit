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
 * This file implements some of the methods for "console" fd's
 * using the CQ console interface implemented by Roland. That interface
 * does not provide enough to do a complete TTY stream interface, but
 * it does provide true interrupt driven I/O, so the asyncio interface
 * can be properly implemented, and so select on the simple console
 * will work.
 *
 * We essentially create an oskit_ttystream_t wrapper around an
 * oskit_stream_t, and pass along the stuff that makes sense and
 * don't bother with the stuff that we know we cannot do at this level.
 * If you need more functonality, use the freebsd TTY stuff.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <oskit/com/stream.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/ttystream.h>
#include <oskit/io/posixio.h>
#include <oskit/tty.h>
#include <oskit/clientos.h>
#include <oskit/c/fd.h>
#include <oskit/dev/osenv_irq.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>

static struct oskit_asyncio_ops		asyncio_ops;
static struct oskit_ttystream_ops	ttystream_ops;
static struct oskit_posixio_ops		posixio_ops;

static struct oskit_stream     *wrapped_stream;
static struct oskit_asyncio    *wrapped_asyncio;
static struct oskit_ttystream  console_ttystream  = { &ttystream_ops };
static struct oskit_asyncio    console_asyncio    = { &asyncio_ops };
static struct oskit_posixio    console_posixio    = { &posixio_ops };

static OSKIT_COMDECL
query(oskit_ttystream_t *si, const struct oskit_guid *iid, void **out_ihandle)
{
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_ttystream_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &console_ttystream;
		return 0;
	}

	if (memcmp(iid, &oskit_posixio_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &console_posixio;
		return 0;
	}

	if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
		oskit_error_t	rc = 0;
		
		if (! wrapped_asyncio)
			rc = oskit_stream_query(wrapped_stream,
				&oskit_asyncio_iid, (void **)&wrapped_asyncio);

		if (rc == 0) {
			*out_ihandle = &console_asyncio;
			return 0;
		}
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
addref(oskit_ttystream_t *si)
{
	return 1;
}

static OSKIT_COMDECL_U
release(oskit_ttystream_t *si)
{
	return 1;
}

static OSKIT_COMDECL
read(oskit_ttystream_t *si,
     void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	int		i = 0, c, actual;
	char		*str = (char *) buf;
	char		tmp;
	oskit_error_t	rc;

#define ECHOCHAR(c) ({						\
	char	__c = (c);					\
	oskit_stream_write(wrapped_stream, &__c, 1, &actual);	\
	})

	if (len == 0)
		return 0;

	/* implement simple line discipline */
	while (i < len) {
		rc = oskit_stream_read(wrapped_stream, &tmp, 1, &actual);
		if (rc) {
			if (i == 0)
				return rc;
			break;
		}

		c = tmp;
		if (c == base_cooked_termios.c_cc[VEOF]) {
			if (i == 0)
				return NULL;
			break;
		}
		if (c == '\r')
			c = '\n';
		else if (c == '\b') {
			if (i > 0) {
				if (base_cooked_termios.c_lflag & ECHO) {
					ECHOCHAR(c);
					ECHOCHAR(' ');
					ECHOCHAR(c);
				}
				i--;
			}
			continue;
		}
		else if (c == '\025') {		/* ^U -- kill line */
			while (i) {
				if (base_cooked_termios.c_lflag & ECHO) {
					ECHOCHAR('\b');
					ECHOCHAR(' ');
					ECHOCHAR('\b');
				}
				i--;
			}
			str[0] = '\0';
			continue;
		}
		if (base_cooked_termios.c_lflag & ECHO)
			ECHOCHAR(c);
		str[i++] = c;
		if (c == '\n')
			break;
	}
	if (i < len)
		str[i] = '\0';

	*out_actual = i;
	return 0;
}

static OSKIT_COMDECL
write(oskit_ttystream_t *si, const void *buf, oskit_u32_t nb,
      oskit_u32_t *out_actual)
{
	return oskit_stream_write(wrapped_stream, buf, nb, out_actual);
}

static OSKIT_COMDECL
seek(oskit_ttystream_t *si, oskit_s64_t ofs, oskit_seek_t whence,
		  oskit_u64_t *out_newpos)
{
	return OSKIT_ESPIPE;
}

static OSKIT_COMDECL
setsize(oskit_ttystream_t *si, oskit_u64_t new_size)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
copyto(oskit_ttystream_t *s, oskit_stream_t *dst,
       oskit_u64_t size,
       oskit_u64_t *out_read,
       oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
commit(oskit_ttystream_t *si, oskit_u32_t commit_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
revert(oskit_ttystream_t *si)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
lockregion(oskit_ttystream_t *si, oskit_u64_t offset,
			 oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
unlockregion(oskit_ttystream_t *si, oskit_u64_t offset,
			   oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
stat(oskit_ttystream_t *si, oskit_stream_stat_t *out_stat,
		  oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
clone(oskit_ttystream_t *si, oskit_stream_t **out_stream)
{
	return OSKIT_E_NOTIMPL;
}

/* TTY Stream Ops */

static OSKIT_COMDECL
getattr(oskit_ttystream_t *si, struct oskit_termios *out_attr)
{
	/*
	 * The termios definitions common to the FreeBSD and OSKit interfaces
	 * have been defined to have identical values. Ain't that a peach.
	 */
	out_attr->iflag  = base_cooked_termios.c_iflag;
	out_attr->oflag  = base_cooked_termios.c_oflag;
	out_attr->cflag  = base_cooked_termios.c_cflag;
	out_attr->lflag  = base_cooked_termios.c_lflag;
	out_attr->ispeed = base_cooked_termios.c_ispeed;
	out_attr->ospeed = base_cooked_termios.c_ospeed;
	memcpy(out_attr->cc, base_cooked_termios.c_cc, NCCS);

	return 0;
}

static OSKIT_COMDECL
setattr(oskit_ttystream_t *si, int actions, const struct oskit_termios *attr)
{
	base_cooked_termios.c_iflag  = attr->iflag;
	base_cooked_termios.c_oflag  = attr->oflag;
	base_cooked_termios.c_cflag  = attr->cflag;
	base_cooked_termios.c_lflag  = attr->lflag;
	base_cooked_termios.c_ispeed = attr->ispeed;
	base_cooked_termios.c_ospeed = attr->ospeed;
	memcpy(base_cooked_termios.c_cc, attr->cc, NCCS);

	return 0;
}

/* ARGSUSED */
static OSKIT_COMDECL
sendbreak(oskit_ttystream_t *si, oskit_u32_t duration)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
drain(oskit_ttystream_t *si)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
flush(oskit_ttystream_t *si, int queue_selector)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
flow(oskit_ttystream_t *si, int action)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
getpgrp(oskit_ttystream_t *si, oskit_pid_t *out_pid)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
setpgrp(oskit_ttystream_t *si, oskit_pid_t new_pid)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
ttyname(oskit_ttystream_t *si, char **out_name)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
getsid(oskit_ttystream_t *si, oskit_pid_t *out_pid)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_ttystream_ops ttystream_ops = {
	query, addref, release,
	read, write, seek, setsize, copyto,
	commit, revert, lockregion, unlockregion, stat, clone,
	getattr, 
	setattr, 
	sendbreak, 
	drain, 
	flush, 
	flow, 
	getpgrp, 
	setpgrp,
	ttyname, 
	getsid
};

static OSKIT_COMDECL
posix_stat(oskit_posixio_t *pio, struct oskit_stat *st)
{
	memset(st, 0, sizeof *st);
	st->ino = 1;
        st->mode = OSKIT_S_IFCHR | OSKIT_S_IRWXG | OSKIT_S_IRWXU | OSKIT_S_IRWXO;
	return 0;
}

static OSKIT_COMDECL
setstat(oskit_posixio_t *pio, oskit_u32_t mask, const struct oskit_stat *stats)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
pathconf(oskit_posixio_t *pio, oskit_s32_t option, oskit_s32_t *out_val)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_posixio_ops posixio_ops = {
	query, addref, release,
	posix_stat, setstat, pathconf
};

/*
 * Pass asyncio operations through.
 */
static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
	return oskit_asyncio_poll(wrapped_asyncio);
}

/* add a listener */
static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
	oskit_s32_t mask)
{
	return oskit_asyncio_add_listener(wrapped_asyncio, l, mask);
}

/* remove a listener */
static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	return oskit_asyncio_remove_listener(wrapped_asyncio, l0);
}

/* how many bytes can be read */
static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
	return oskit_asyncio_readable(wrapped_asyncio);
}

static struct oskit_asyncio_ops asyncio_ops = {
	query, addref, release,	/* will generate three leg. warnings */
	asyncio_poll,
	asyncio_add_listener,
	asyncio_remove_listener,
	asyncio_readable
};

void
start_cq_console(void)
{
	oskit_error_t	rc;
	extern int	serial_console;		/* XXX */
	oskit_stream_t  *newstream;

	if (serial_console) {
		rc = cq_com_console_init(1, &base_cooked_termios,
					 oskit_create_osenv_irq(),
					 oskit_create_osenv_intr(),
					 oskit_create_osenv_sleep(NULL),
					 &newstream);
	}
	else {
		rc = cq_direct_console_init(oskit_create_osenv_irq(),
					    oskit_create_osenv_intr(),
					    oskit_create_osenv_sleep(NULL),
					    &newstream);
	}
	if (rc)
		panic("start_cq_console");

	wrapped_stream = newstream;

	/*
	 * Okay, now replace the default console stream.
	 */
	oskit_clientos_setconsole(&console_ttystream);

	/*
	 * Okay, tell the C library to switch over!
	 */
	printf("Reconstituting stdio streams to console TTY...\n");
	oskit_console_reinit();
}
