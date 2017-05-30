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
 * Definition of the COM oskit_ttystream interface,
 * which represents a POSIX.1 open terminal stream.
 *
 * The termios structure and flag definitions below
 * happen to be a compatible subset of those defined in FreeBSD.
 */
#ifndef _OSKIT_IO_TTYSTREAM_H_
#define _OSKIT_IO_TTYSTREAM_H_

#include <oskit/com.h>
#include <oskit/types.h>
#include <oskit/com/stream.h>


/*
 * The following are all unsigned integral types,
 * in accordance with the Unix specification.
 */
typedef oskit_u32_t	oskit_tcflag_t;
typedef oskit_u8_t	oskit_cc_t;
typedef oskit_u32_t	oskit_speed_t;


/* Indexes into control characters array (c_cc) */
#define	OSKIT_VEOF	0
#define	OSKIT_VEOL	1
#define	OSKIT_VERASE	3
#define OSKIT_VKILL	5
#define OSKIT_VINTR	8
#define OSKIT_VQUIT	9
#define OSKIT_VSUSP	10
#define OSKIT_VSTART	12
#define OSKIT_VSTOP	13
#define OSKIT_VMIN	16
#define OSKIT_VTIME	17
#define	OSKIT_NCCS	20

/* Magic control character value to disable the associated feature */
#define OSKIT_VDISABLE	0xff

/* Input flags (oskit_termios.iflag) */
#define	OSKIT_IGNBRK	0x00000001	/* ignore BREAK condition */
#define	OSKIT_BRKINT	0x00000002	/* map BREAK to SIGINTR */
#define	OSKIT_IGNPAR	0x00000004	/* ignore (discard) parity errors */
#define	OSKIT_PARMRK	0x00000008	/* mark parity and framing errors */
#define	OSKIT_INPCK	0x00000010	/* enable checking of parity errors */
#define	OSKIT_ISTRIP	0x00000020	/* strip 8th bit off chars */
#define	OSKIT_INLCR	0x00000040	/* map NL into CR */
#define	OSKIT_IGNCR	0x00000080	/* ignore CR */
#define	OSKIT_ICRNL	0x00000100	/* map CR to NL (ala CRMOD) */
#define	OSKIT_IXON	0x00000200	/* enable output flow control */
#define	OSKIT_IXOFF	0x00000400	/* enable input flow control */
#define	OSKIT_IXANY	0x00000800	/* any char will restart after stop */

/* Output flags (oskit_termios.oflag) */
#define	OSKIT_OPOST	0x00000001	/* enable following output processing */
#define OSKIT_ONLCR	0x00000002	/* map NL to CR-NL (ala CRMOD) */

/* Control flags (oskit_termios.cflag) */
#define OSKIT_CSIZE	0x00000300	/* character size mask */
#define OSKIT_CS5	0x00000000	    /* 5 bits (pseudo) */
#define OSKIT_CS6	0x00000100	    /* 6 bits */
#define OSKIT_CS7	0x00000200	    /* 7 bits */
#define OSKIT_CS8	0x00000300	    /* 8 bits */
#define OSKIT_CSTOPB	0x00000400	/* send 2 stop bits */
#define OSKIT_CREAD	0x00000800	/* enable receiver */
#define OSKIT_PARENB	0x00001000	/* parity enable */
#define OSKIT_PARODD	0x00002000	/* odd parity, else even */
#define OSKIT_HUPCL	0x00004000	/* hang up on last close */
#define OSKIT_CLOCAL	0x00008000	/* ignore modem status lines */

/* Local flags (oskit_termios.lflag) */
#define	OSKIT_ECHOE	0x00000002	/* visually erase chars */
#define	OSKIT_ECHOK	0x00000004	/* echo NL after line kill */
#define OSKIT_ECHO	0x00000008	/* enable echoing */
#define	OSKIT_ECHONL	0x00000010	/* echo NL even if ECHO is off */
#define	OSKIT_ISIG	0x00000080	/* enable signals INTR, QUIT, [D]SUSP */
#define	OSKIT_ICANON	0x00000100	/* canonicalize input lines */
#define	OSKIT_IEXTEN	0x00000400	/* enable DISCARD and LNEXT */
#define OSKIT_TOSTOP	0x00400000	/* stop background jobs from output */
#define	OSKIT_NOFLSH	0x80000000	/* don't flush after interrupt */

/*
 * Standard POSIX terminal I/O parameters structure.
 * The member names defined here don't include the standard c_ prefix,
 * because that prefix is reserved by POSIX for #define'd symbols,
 * and using it here could make this header file conflict
 * with real termios.h headers that do funny things with struct termios.
 */
struct oskit_termios {
	oskit_tcflag_t	iflag;
	oskit_tcflag_t	oflag;
	oskit_tcflag_t	cflag;
	oskit_tcflag_t	lflag;
	oskit_cc_t	cc[OSKIT_NCCS];
	oskit_speed_t	ispeed;
	oskit_speed_t	ospeed;
};
typedef struct oskit_termios oskit_termios_t;


/* Action codes for ttystream->setattr() */
#define	OSKIT_TCSANOW	0		/* make change immediate */
#define	OSKIT_TCSADRAIN	1		/* drain output, then change */
#define	OSKIT_TCSAFLUSH	2		/* drain output, flush input */

/* Queue selector codes for ttystream->flush() */
#define	OSKIT_TCIFLUSH	1		/* flush data received but not read */
#define	OSKIT_TCOFLUSH	2		/* flush data written but not sent */
#define OSKIT_TCIOFLUSH	3		/* flush both input and output */

/* Action codes for ttystream->flow() */
#define	OSKIT_TCOOFF	1		/* suspend output */
#define	OSKIT_TCOON	2		/* restart output */
#define OSKIT_TCIOFF	3		/* transmit a STOP character */
#define OSKIT_TCION	4		/* transmit a START character */


/*
 * Basic tty stream object interface,
 * IID 4aa7df9c-7c74-11cf-b500-08000953adc2.
 * This interface extends the oskit_file interface.
 */
struct oskit_ttystream {
	struct oskit_ttystream_ops *ops;
};
typedef struct oskit_ttystream oskit_ttystream_t;

struct oskit_ttystream_ops {

	/*** Operations inherited from IUnknown interface ***/
	OSKIT_COMDECL	(*query)(oskit_ttystream_t *s,
				 const struct oskit_guid *iid,
				 void **out_ihandle);
	OSKIT_COMDECL_U	(*addref)(oskit_ttystream_t *s);
	OSKIT_COMDECL_U	(*release)(oskit_ttystream_t *s);

	/*** Operations inherited from IStream interface ***/
	OSKIT_COMDECL	(*read)(oskit_ttystream_t *s, void *buf, oskit_u32_t len,
				oskit_u32_t *out_actual);
	OSKIT_COMDECL	(*write)(oskit_ttystream_t *s, const void *buf,
				 oskit_u32_t len, oskit_u32_t *out_actual);
	OSKIT_COMDECL	(*seek)(oskit_ttystream_t *s, oskit_s64_t ofs,
				oskit_seek_t whence, oskit_u64_t *out_newpos);
	OSKIT_COMDECL	(*setsize)(oskit_ttystream_t *s, oskit_u64_t new_size);
	OSKIT_COMDECL	(*copyto)(oskit_ttystream_t *s, oskit_ttystream_t *dst,
				  oskit_u64_t size,
				   oskit_u64_t *out_read,
				   oskit_u64_t *out_written);
	OSKIT_COMDECL	(*commit)(oskit_ttystream_t *s,
				  oskit_u32_t commit_flags);
	OSKIT_COMDECL	(*revert)(oskit_ttystream_t *s);
	OSKIT_COMDECL	(*lockregion)(oskit_ttystream_t *s,
					oskit_u64_t offset, oskit_u64_t size,
					oskit_u32_t lock_type);
	OSKIT_COMDECL	(*unlockregion)(oskit_ttystream_t *s,
					 oskit_u64_t offset, oskit_u64_t size,
					 oskit_u32_t lock_type);
	OSKIT_COMDECL	(*stat)(oskit_ttystream_t *s,
				oskit_stream_stat_t *out_stat,
				oskit_u32_t stat_flags);
	OSKIT_COMDECL	(*clone)(oskit_ttystream_t *s,
				 oskit_ttystream_t **out_stream);

	/*** Operations derived from POSIX.1-1990 ***/
	OSKIT_COMDECL	(*getattr)(oskit_ttystream_t *s,
				     struct oskit_termios *out_attr);
	OSKIT_COMDECL	(*setattr)(oskit_ttystream_t *s, int actions,
				     const struct oskit_termios *attr);
	OSKIT_COMDECL	(*sendbreak)(oskit_ttystream_t *f, oskit_u32_t duration);
	OSKIT_COMDECL	(*drain)(oskit_ttystream_t *s);
	OSKIT_COMDECL	(*flush)(oskit_ttystream_t *s, int queue_selector);
	OSKIT_COMDECL	(*flow)(oskit_ttystream_t *s, int action);
	OSKIT_COMDECL	(*getpgrp)(oskit_ttystream_t *s, oskit_pid_t *out_pid);
	OSKIT_COMDECL	(*setpgrp)(oskit_ttystream_t *s, oskit_pid_t pid);

	/*** X/Open CAE 1994 ***/
	OSKIT_COMDECL	(*ttyname)(oskit_ttystream_t *s,
				   char **out_name);
	OSKIT_COMDECL	(*getsid)(oskit_ttystream_t *s, oskit_pid_t *out_pid);
};

/* GUID for oskit_ttystream interface */
extern const struct oskit_guid oskit_ttystream_iid;
#define OSKIT_TTYSTREAM_IID OSKIT_GUID(0x4aa7df9c, 0x7c74, 0x11cf, \
			0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_ttystream_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_ttystream_t *)(s), (iid), (out_ihandle)))
#define oskit_ttystream_addref(s) \
	((s)->ops->addref((oskit_ttystream_t *)(s)))
#define oskit_ttystream_release(s) \
	((s)->ops->release((oskit_ttystream_t *)(s)))
#define oskit_ttystream_read(s, buf, len, out_actual) \
	((s)->ops->read((oskit_ttystream_t *)(s), (buf), (len), (out_actual)))
#define oskit_ttystream_write(s, buf, len, out_actual) \
	((s)->ops->write((oskit_ttystream_t *)(s), (buf), (len), (out_actual)))
#define oskit_ttystream_seek(s, ofs, whence, out_newpos) \
	((s)->ops->seek((oskit_ttystream_t *)(s), (ofs), (whence), (out_newpos)))
#define oskit_ttystream_setsize(s, new_size) \
	((s)->ops->setsize((oskit_ttystream_t *)(s), (new_size)))
#define oskit_ttystream_copyto(s, dst, size, out_read, out_written) \
	((s)->ops->copyto((oskit_ttystream_t *)(s), (dst), (size), (out_read), (out_written)))
#define oskit_ttystream_commit(s, commit_flags) \
	((s)->ops->commit((oskit_ttystream_t *)(s), (commit_flags)))
#define oskit_ttystream_revert(s) \
	((s)->ops->revert((oskit_ttystream_t *)(s)))
#define oskit_ttystream_lockregion(s, offset, size, lock_type) \
	((s)->ops->lockregion((oskit_ttystream_t *)(s), (offset), (size), (lock_type)))
#define oskit_ttystream_unlockregion(s, offset, size, lock_type) \
	((s)->ops->unlockregion((oskit_ttystream_t *)(s), (offset), (size), (lock_type)))
#define oskit_ttystream_stat(s, out_stat, stat_flags) \
	((s)->ops->stat((oskit_ttystream_t *)(s), (out_stat), (stat_flags)))
#define oskit_ttystream_clone(s, out_stream) \
	((s)->ops->clone((oskit_ttystream_t *)(s), (out_stream)))
#define oskit_ttystream_getattr(s, out_attr) \
	((s)->ops->getattr((oskit_ttystream_t *)(s), (out_attr)))
#define oskit_ttystream_setattr(s, actions, attr) \
	((s)->ops->setattr((oskit_ttystream_t *)(s), (actions), (attr)))
#define oskit_ttystream_sendbreak(f, duration) \
	((f)->ops->sendbreak((oskit_ttystream_t *)(f), (duration)))
#define oskit_ttystream_drain(s) \
	((s)->ops->drain((oskit_ttystream_t *)(s)))
#define oskit_ttystream_flush(s, queue_selector) \
	((s)->ops->flush((oskit_ttystream_t *)(s), (queue_selector)))
#define oskit_ttystream_flow(s, action) \
	((s)->ops->flow((oskit_ttystream_t *)(s), (action)))
#define oskit_ttystream_getpgrp(s, out_pid) \
	((s)->ops->getpgrp((oskit_ttystream_t *)(s), (out_pid)))
#define oskit_ttystream_setpgrp(s, pid) \
	((s)->ops->setpgrp((oskit_ttystream_t *)(s), (pid)))
#define oskit_ttystream_ttyname(s, out_name) \
	((s)->ops->ttyname((oskit_ttystream_t *)(s), (out_name)))
#define oskit_ttystream_getsid(s, out_pid) \
	((s)->ops->getsid((oskit_ttystream_t *)(s), (out_pid)))


#endif /* _OSKIT_IO_TTYSTREAM_H_ */
