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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>

#include <oskit/com.h>
#include <oskit/io/ttystream.h>
#include <oskit/io/asyncio.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/freebsd.h>
#include <oskit/com/listener_mgr.h>

#include "glue.h"


/*** TTY Stream Interface ***/

struct strm {
	oskit_ttystream_t strmi;
	oskit_asyncio_t	ioa;
	int refs;
	int major;
	dev_t dev;
	int flags;
	int io_flag;
	struct tty *tp;

	struct listener_mgr *readers;	/* listeners for asyncio READ */
	struct listener_mgr *writers;	/* listeners for asyncio WRITE*/
};

static OSKIT_COMDECL
query(oskit_ttystream_t *si, const struct oskit_guid *iid, void **out_ihandle)
{
	struct strm *s = (struct strm*)si;

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_stream_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_ttystream_iid, sizeof(*iid)) == 0) {
		s->refs++;
		*out_ihandle = &s->strmi;
		return 0;
	}

        if (memcmp(iid, &oskit_asyncio_iid, sizeof(*iid)) == 0) {
		s->refs++;
                *out_ihandle = &s->ioa;
                return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
add_ref(oskit_ttystream_t *si)
{
	struct strm *s = (struct strm*)si;

	return ++s->refs;
}

static OSKIT_COMDECL_U
release(oskit_ttystream_t *si)
{
	struct strm *s = (struct strm*)si;
	int newcount = --s->refs;

	if (newcount < 0)
		panic("fdev_freebsd: mangled reference count");
	if (newcount == 0) {
		struct proc p;

		OSKIT_FREEBSD_CREATE_CURPROC(p);
		cdevsw[s->major].d_close(s->dev, s->flags, S_IFCHR, curproc);
		OSKIT_FREEBSD_DESTROY_CURPROC(p);
		oskit_destroy_listener_mgr(s->readers);
		oskit_destroy_listener_mgr(s->writers);
		osenv_mem_free(s, 0, sizeof(*s));
	}
	return newcount;
}

static OSKIT_COMDECL
read(oskit_ttystream_t *si, void *buf, oskit_u32_t len, oskit_u32_t *out_actual)
{
	struct strm *s = (struct strm*)si;
	struct iovec iov;
	struct uio uio;
	struct proc p;
	int rc;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	iov.iov_base = buf;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = len;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_READ;
	uio.uio_procp = curproc;

	rc = cdevsw[s->major].d_read(s->dev, &uio, s->io_flag);

	OSKIT_FREEBSD_DESTROY_CURPROC(p);

	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	*out_actual = len - uio.uio_resid;
	return 0;
}

static OSKIT_COMDECL
write(oskit_ttystream_t *si, const void *buf, oskit_u32_t len,
      oskit_u32_t *out_actual)
{
	struct strm *s = (struct strm*)si;
	struct iovec iov;
	struct uio uio;
	struct proc p;
	int rc;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	iov.iov_base = (char*)buf;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = len;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_WRITE;
	uio.uio_procp = curproc;

	rc = cdevsw[s->major].d_write(s->dev, &uio, s->io_flag);

	OSKIT_FREEBSD_DESTROY_CURPROC(p);

	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	*out_actual = len - uio.uio_resid;
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
ttystream_copy_to(oskit_ttystream_t *si, oskit_stream_t *dst,
	oskit_u64_t size, oskit_u64_t *out_read, oskit_u64_t *out_written)
{
	return OSKIT_E_NOTIMPL;	/*XXX*/
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
lock_region(oskit_ttystream_t *si, oskit_u64_t offset,
			 oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
unlock_region(oskit_ttystream_t *si, oskit_u64_t offset,
			   oskit_u64_t size, oskit_u32_t lock_type)
{
	return OSKIT_E_NOTIMPL;
}

static OSKIT_COMDECL
stat(oskit_ttystream_t *si, oskit_stream_stat_t *out_stat,
		  oskit_u32_t stat_flags)
{
	return OSKIT_E_NOTIMPL;	/*XXX*/
}

static OSKIT_COMDECL
clone(oskit_ttystream_t *si, oskit_ttystream_t **out_stream)
{
	return OSKIT_E_NOTIMPL;	/*XXX*/
}

static oskit_error_t
ioctl(struct strm *s, int cmd, caddr_t data)
{
	struct proc p;
	int rc;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	rc = cdevsw[s->major].d_ioctl(s->dev, cmd, data, s->flags, curproc);

	OSKIT_FREEBSD_DESTROY_CURPROC(p);

	if (rc)
		return oskit_freebsd_xlate_errno(rc);

	return rc;
}

#define CIFLAG	(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP |	\
		 INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY)
#define COFLAG	(OPOST | ONLCR)
#define CCFLAG	(CSIZE | CSTOPB | CREAD | PARENB | PARODD | HUPCL | CLOCAL);
#define CLFLAG	(ECHOE | ECHOK | ECHO | ECHONL | ISIG | ICANON |	\
		 IEXTEN | TOSTOP | NOFLSH)

static OSKIT_COMDECL
getattr(oskit_ttystream_t *si, struct oskit_termios *out_attr)
{
	struct strm *s = (struct strm*)si;
	struct termios t;
	oskit_error_t rc;

	rc = ioctl(s, TIOCGETA, (caddr_t)&t);
	if (rc)
		return rc;

	/*
	 * * The termios definitions common to the FreeBSD and OSKit interfaces
	 * have been defined to have identical values,
	 * which makes it easy to convert between the two.
	 * We still need to mask out non-common or undefined bits though.
	 */
	out_attr->iflag = t.c_iflag & CIFLAG;
	out_attr->oflag = t.c_oflag & COFLAG;
	out_attr->cflag = t.c_cflag & CCFLAG;
	out_attr->lflag = t.c_lflag & CLFLAG;

	if (NCCS != OSKIT_NCCS)
		panic("fdev_freebsd: NCCS mismatch");
	memcpy(out_attr->cc, t.c_cc, NCCS);

	out_attr->ispeed = t.c_ispeed;
	out_attr->ospeed = t.c_ospeed;

	return 0;
}

static OSKIT_COMDECL
setattr(oskit_ttystream_t *si, int actions, const struct oskit_termios *attr)
{
	struct strm *s = (struct strm*)si;
	int cmd;
	struct termios t;

	switch (actions) {
		case OSKIT_TCSANOW:	cmd = TIOCSETA;		break;
		case OSKIT_TCSADRAIN:	cmd = TIOCSETAW;	break;
		case OSKIT_TCSAFLUSH:	cmd = TIOCSETAF;	break;
		default:		return OSKIT_EINVAL;
	}

	t.c_iflag = attr->iflag & CIFLAG;
	t.c_oflag = attr->oflag & COFLAG;
	t.c_cflag = attr->cflag & CCFLAG;
	t.c_lflag = attr->lflag & CLFLAG;

	if (NCCS != OSKIT_NCCS)
		panic("fdev_freebsd: NCCS mismatch");
	memcpy(t.c_cc, attr->cc, NCCS);

	t.c_ispeed = attr->ispeed;
	t.c_ospeed = attr->ospeed;

	return ioctl(s, cmd, (caddr_t)&t);
}

/* ARGSUSED */
static OSKIT_COMDECL
sendbreak(oskit_ttystream_t *si, oskit_u32_t duration)
{
	struct strm *s = (struct strm*)si;
	struct proc p;
	int rc;

	OSKIT_FREEBSD_CREATE_CURPROC(p);

	rc = cdevsw[s->major].d_ioctl(s->dev, TIOCSBRK, 0, s->flags, curproc);
	if (rc != 0)
		goto out;

	/* Sleep for four tenths of a second */
	tsleep(&rc, PZERO, "sendbreak", hz * 4 / 10);

	rc = cdevsw[s->major].d_ioctl(s->dev, TIOCCBRK, 0, s->flags, curproc);

out:
	OSKIT_FREEBSD_DESTROY_CURPROC(p);
	if (rc)
		return oskit_freebsd_xlate_errno(rc);
	return 0;
}

static OSKIT_COMDECL
drain(oskit_ttystream_t *si)
{
	struct strm *s = (struct strm*)si;

	return ioctl(s, TIOCDRAIN, 0);
}

static OSKIT_COMDECL
flush(oskit_ttystream_t *si, int queue_selector)
{
	struct strm *s = (struct strm*)si;
	int com;

	switch (queue_selector) {
		case OSKIT_TCIFLUSH:	com = FREAD;		break;
		case OSKIT_TCOFLUSH:	com = FWRITE;		break;
		case OSKIT_TCIOFLUSH:	com = FREAD | FWRITE;	break;
		default:		return OSKIT_EINVAL;
	}

	return ioctl(s, TIOCFLUSH, (caddr_t)&com);
}

static OSKIT_COMDECL
flow(oskit_ttystream_t *si, int action)
{
	struct strm *s = (struct strm*)si;

	switch (action) {
	case OSKIT_TCOOFF:
		return ioctl(s, TIOCSTOP, 0);
	case OSKIT_TCOON:
		return ioctl(s, TIOCSTART, 0);
	case OSKIT_TCION:
	case OSKIT_TCIOFF:
		{
			struct tty *tp = cdevsw[s->major].d_devtotty(s->dev);
			char *p = &tp->t_cc[action == OSKIT_TCIOFF
					    ? VSTOP : VSTART];
			oskit_u32_t actual;

			return write(si, p, 1, &actual);
		}
	default:
		return OSKIT_EINVAL;
	}
}

static OSKIT_COMDECL
getpgrp(oskit_ttystream_t *si, oskit_pid_t *out_pid)
{
	struct strm *s = (struct strm*)si;
	oskit_error_t rc;
	int pid;

	rc = ioctl(s, TIOCGPGRP, (caddr_t)&pid);
	if (rc)
		return rc;

	*out_pid = pid;
	return 0;
}

static OSKIT_COMDECL
setpgrp(oskit_ttystream_t *si, oskit_pid_t new_pid)
{
	struct strm *s = (struct strm*)si;
	int pid = new_pid;

	return ioctl(s, TIOCGPGRP, (caddr_t)&pid);
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

struct oskit_ttystream_ops ops = {
	query, add_ref, release,
	read, write, seek, setsize, 
	ttystream_copy_to,	/* copy_to is global.h'ed ! */
	commit, revert, lock_region, unlock_region, stat, clone,
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

/*******************************************************/
/******* Implementation of the oskit_asyncio_t if *******/
/*******************************************************/

/*
 * XXX I put this in tty.c so as not to duplicate code here.
 */
unsigned	ttyconds(struct tty *);

static OSKIT_COMDECL
char_asyncio_query(oskit_asyncio_t *f, const struct oskit_guid *iid,
	void **out_ihandle)
{
	oskit_ttystream_t *si = (oskit_ttystream_t *)(f - 1);
	
	return query(si, iid, out_ihandle);
}

static OSKIT_COMDECL_U
char_asyncio_addref(oskit_asyncio_t *f)
{
	oskit_ttystream_t *si = (oskit_ttystream_t *)(f - 1);
	
	return add_ref(si);
}

static OSKIT_COMDECL_U
char_asyncio_release(oskit_asyncio_t *f)
{
	oskit_ttystream_t *si = (oskit_ttystream_t *)(f - 1);
	
	return release(si);
}

/*
 * Poll for currently pending asynchronous I/O conditions.
 * If successful, returns a mask of the OSKIT_ASYNC_IO_* flags above,
 * indicating which conditions are currently present.
 */
static OSKIT_COMDECL
char_asyncio_poll(oskit_asyncio_t *f)
{
	struct strm	*strm = (struct strm *)(f - 1);
        oskit_u32_t     conds;
        int		s;
	
	s = spltty();
	conds = ttyconds(strm->tp);
        splx(s);

        return conds;
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
char_asyncio_add_listener(oskit_asyncio_t *f, struct oskit_listener *l,
	oskit_s32_t mask)
{
	struct strm	*strm = (struct strm *)(f - 1);
	struct tty	*tp = strm->tp;
        oskit_u32_t     conds;
        int		s;
	struct proc 	p;

	OSKIT_FREEBSD_CREATE_CURPROC(p)
        s = spltty();
	conds = ttyconds(strm->tp);

	/* for read and exceptional conditions */
	if (mask & (OSKIT_ASYNCIO_READABLE | OSKIT_ASYNCIO_EXCEPTION))
	{
		oskit_listener_mgr_add(strm->readers, l);
		curproc->p_sel = strm->readers;
		selrecord(curproc, &tp->t_rsel);
		curproc->p_sel = 0;
	}

	/* for write */
	if (mask & OSKIT_ASYNCIO_WRITABLE)
	{
		oskit_listener_mgr_add(strm->writers, l);
		curproc->p_sel = strm->writers;
		selrecord(curproc, &tp->t_wsel);
		curproc->p_sel = 0;
	}
	
        splx(s);
	OSKIT_FREEBSD_DESTROY_CURPROC(p)
        return conds;
}

/*
 * Remove a previously registered listener callback object.
 * Returns an error if the specified callback has not been registered.
 */
static OSKIT_COMDECL
char_asyncio_remove_listener(oskit_asyncio_t *f, struct oskit_listener *l0)
{
	struct strm	*strm = (struct strm *)(f - 1);
	struct tty	*tp = strm->tp;
	oskit_error_t	rc1, rc2;
	int		s;

	s = spltty();

	/*
	 * we don't know where was added - if at all - so let's check
	 * both lists
	 *
	 * turn off notifications if no listeners left
	 */
	rc1 = oskit_listener_mgr_remove(strm->readers, l0);
	if (oskit_listener_mgr_count(strm->readers) == 0) {
		tp->t_rsel.si_sel = 0;
		tp->t_rsel.si_pid = 0;
	}

	rc2 = oskit_listener_mgr_remove(strm->writers, l0);
	if (oskit_listener_mgr_count(strm->writers) == 0) {
		tp->t_wsel.si_sel = 0;
		tp->t_wsel.si_pid = 0;
	}

	splx(s);

	/* flag error if both removes failed */
	return (rc1 && rc2) ? OSKIT_E_INVALIDARG : 0;	/* is that right ? */
}

/*
 * return the number of bytes that can be read, basically ioctl(FIONREAD)
 */
static OSKIT_COMDECL
char_asyncio_readable(oskit_asyncio_t *f)
{
	struct strm	*strm = (struct strm *)(f - 1);
	int		count;

	ioctl(strm, FIONREAD, (caddr_t)&count);
	
	return count;
}

static struct oskit_asyncio_ops asyncioops =
{
	char_asyncio_query,
	char_asyncio_addref,
	char_asyncio_release,
	char_asyncio_poll,
	char_asyncio_add_listener,
	char_asyncio_remove_listener,
	char_asyncio_readable
};


oskit_error_t 
oskit_freebsd_chardev_open(int major, int minor, int flags,
				    struct oskit_ttystream **out_ttystream)
{
	struct strm *s;
	struct proc p;
	int bsd_rc;

	*out_ttystream = NULL;

	if (major >= nchrdev || cdevsw[major].d_open == NULL)
		return OSKIT_ENXIO;

	s = osenv_mem_alloc(sizeof(*s), 0, 0);
	if (s == NULL)
		return OSKIT_ENOMEM;
	s->strmi.ops = &ops;
	s->refs = 1;
	s->major = major;
	s->dev = makedev(major, minor);
	s->flags = flags;
	s->io_flag = (flags & O_NONBLOCK) ? IO_NDELAY : 0;
	
	OSKIT_FREEBSD_CREATE_CURPROC(p);

	bsd_rc = cdevsw[major].d_open(s->dev, flags, S_IFCHR, curproc);

	OSKIT_FREEBSD_DESTROY_CURPROC(p);

	if (bsd_rc) {
		osenv_mem_free(s, 0, sizeof(*s));
		return oskit_freebsd_xlate_errno(bsd_rc);
	}

	s->ioa.ops = &asyncioops;
	s->readers = oskit_create_listener_mgr((oskit_iunknown_t *)&s->ioa);
	s->writers = oskit_create_listener_mgr((oskit_iunknown_t *)&s->ioa);
	s->tp      = cdevsw[major].d_devtotty(s->dev);

	*out_ttystream = &s->strmi;
	return 0;
}

