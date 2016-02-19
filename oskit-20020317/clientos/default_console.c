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
 * This file implements some of the methods for "console" fd's.
 * It is intended to serve as the simplest console possible for a
 * default client OS. The posix library contains a linktime dependency
 * on this console stream to allow for printf to work from the getgo.
 *
 * Note that this implementation contans linktime dependencies on the
 * kernel library (or the unix library) for console_putchar and friends.
 */

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <oskit/io/asyncio.h>
#include <oskit/io/ttystream.h>
#include <oskit/io/posixio.h>
#include <oskit/console.h>
#include <oskit/c/fd.h>

static struct oskit_asyncio_ops		asyncio_ops;
static struct oskit_ttystream_ops	ttystream_ops;
static struct oskit_posixio_ops		posixio_ops;

static struct oskit_ttystream  console_ttystream  = { &ttystream_ops };
static struct oskit_asyncio    console_asyncio    = { &asyncio_ops };
static struct oskit_posixio    console_posixio    = { &posixio_ops };

/*
 * I doubt is makes any sense to do tty processing here, but this is
 * intended to be a super simple console interface, and we need to do
 * a little bit of line discipline stuff to make life a little nicer.
 *
 * There is only one copy since there is only one "tty." The right
 * thing to do might be to export an interface from the base console
 * code to support changing its tty values. It should be noted that
 * virtually none of this stuff can be changed in this simple console,
 * but at least we can support a couple of simple things, like turning
 * of echo. 
 */
struct termios my_termios =
{ {
	ICRNL | IXON,			/* input flags */
	OPOST,				/* output flags */
	CS8,				/* control flags */
	ECHO | ECHOE | ECHOK | ICANON,	/* local flags */
	{	'D'-64,			/* VEOF */
		_POSIX_VDISABLE,	/* VEOL */
		0,
		'H'-64,			/* VERASE */
		0,
		'U'-64,			/* VKILL */
		0,
		0,
		'C'-64,			/* VINTR */
		'\\'-64,		/* VQUIT */
		'Z'-64,			/* VSUSP */
		0,
		'Q'-64,			/* VSTART */
		'S'-64,			/* VSTOP */
	},
	B9600,				/* input speed */
	B9600,				/* output speed */
} };

/*
 * This is a common symbol. If the program is linked with the startup
 * library, this will become the definition. Otherwise, the posix
 * library will use its internal default, which exists to ask someone
 * else for a console at runtime.
 */
struct oskit_stream *default_console_stream =
			(struct oskit_stream *) &console_ttystream;

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
		*out_ihandle = &console_asyncio;
		return 0;
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
read(oskit_ttystream_t *si, void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	int	i = 0, c;
	char	*str = (char *) buf;

	/* implement simple line discipline */
	while (i < len) {
		c = console_getchar();
		if (c == EOF)
			break;

		if (my_termios.c_lflag & ICANON) {
			if (c == '\r')
				c = '\n';
			else if (c == '\b') {
				if (i > 0) {
					if (my_termios.c_lflag & ECHO) {
						console_putchar(c);
						console_putchar(' ');
						console_putchar(c);
					}
					i--;
				}
				continue;
			}
			else if (c == '\025') {		/* ^U -- kill line */
				while (i) {
					if (my_termios.c_lflag & ECHO) {
						console_putchar('\b');
						console_putchar(' ');
						console_putchar('\b');
					}
					i--;
				}
				str[0] = '\0';
				continue;
			}
		}
		if (my_termios.c_lflag & ECHO)
			console_putchar(c);
		str[i++] = c;
		if (c == '\n' && (my_termios.c_lflag & ICANON))
			break;
	}
	if (i < len && (my_termios.c_lflag & ICANON))
		str[i] = '\0';

	*out_actual = i;
	return 0;
}

static OSKIT_COMDECL
write(oskit_ttystream_t *si, const void *buf, oskit_u32_t nb,
      oskit_u32_t *out_actual)
{
	extern int console_putbytes(const char *, int length);

	console_putbytes((const char *) buf, (int) nb);

	*out_actual = nb;
	return 0;
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
	return OSKIT_EINVAL;
}

static OSKIT_COMDECL
copyto(oskit_ttystream_t *s, oskit_ttystream_t *dst,
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
clone(oskit_ttystream_t *si, oskit_ttystream_t **out_stream)
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
	out_attr->iflag  = my_termios.c_iflag;
	out_attr->oflag  = my_termios.c_oflag;
	out_attr->cflag  = my_termios.c_cflag;
	out_attr->lflag  = my_termios.c_lflag;
	out_attr->ispeed = my_termios.c_ispeed;
	out_attr->ospeed = my_termios.c_ospeed;
	memcpy(out_attr->cc, my_termios.c_cc, NCCS);

	return 0;
}

static OSKIT_COMDECL
setattr(oskit_ttystream_t *si, int actions, const struct oskit_termios *attr)
{
	my_termios.c_iflag  = attr->iflag;
	my_termios.c_oflag  = attr->oflag;
	my_termios.c_cflag  = attr->cflag;
	my_termios.c_lflag  = attr->lflag;
	my_termios.c_ispeed = attr->ispeed;
	my_termios.c_ospeed = attr->ospeed;
	memcpy(my_termios.c_cc, attr->cc, NCCS);

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
	query, addref, release, /* will generate three leg. warnings */
	posix_stat, setstat, pathconf
};

/*
 * this should trigger applications who select on a console fd to
 * go into read->getchar or write->putchar immediately.
 */
static OSKIT_COMDECL
asyncio_poll(oskit_asyncio_t *f)
{
	return OSKIT_ASYNCIO_WRITABLE | OSKIT_ASYNCIO_READABLE;
}

/* add a listener */
static OSKIT_COMDECL
asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
	oskit_s32_t mask)
{
	return OSKIT_E_NOTIMPL;
}

/* remove a listener */
static OSKIT_COMDECL
asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	return OSKIT_E_NOTIMPL;
}

/* how many bytes can be read */
static OSKIT_COMDECL
asyncio_readable(oskit_asyncio_t *f)
{
	return 0;
}

static struct oskit_asyncio_ops asyncio_ops = {
	query, addref, release,	/* will generate three leg. warnings */
	asyncio_poll,
	asyncio_add_listener,
	asyncio_remove_listener,
	asyncio_readable
};
