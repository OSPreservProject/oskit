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
#ifndef _OSKIT_C_TERMIOS_H_
#define _OSKIT_C_TERMIOS_H_

#include <oskit/compiler.h>
#include <oskit/io/ttystream.h>

/* Indexes into control characters array (c_cc) */
#define	VEOF		OSKIT_VEOF
#define	VEOL		OSKIT_VEOL
#define	VERASE		OSKIT_VERASE
#define VKILL		OSKIT_VKILL
#define VINTR		OSKIT_VINTR
#define VQUIT		OSKIT_VQUIT
#define VSUSP		OSKIT_VSUSP
#define VSTART		OSKIT_VSTART
#define VSTOP		OSKIT_VSTOP
#define VMIN		OSKIT_VMIN
#define VTIME		OSKIT_VTIME
#define	NCCS		OSKIT_NCCS
/* We avoid using OSKIT_VDISABLE here so that this definition
   of _POSIX_VDISABLE is textually identical to the one in unistd.h.  */
#define _POSIX_VDISABLE	0xff

/* Input flags (c_iflag) */
#define	IGNBRK		OSKIT_IGNBRK	/* ignore BREAK condition */
#define	BRKINT		OSKIT_BRKINT	/* map BREAK to SIGINTR */
#define	IGNPAR		OSKIT_IGNPAR	/* ignore (discard) parity errors */
#define	PARMRK		OSKIT_PARMRK	/* mark parity and framing errors */
#define	INPCK		OSKIT_INPCK	/* enable checking of parity errors */
#define	ISTRIP		OSKIT_ISTRIP	/* strip 8th bit off chars */
#define	INLCR		OSKIT_INLCR	/* map NL into CR */
#define	IGNCR		OSKIT_IGNCR	/* ignore CR */
#define	ICRNL		OSKIT_ICRNL	/* map CR to NL (ala CRMOD) */
#define	IXON		OSKIT_IXON	/* enable output flow control */
#define	IXOFF		OSKIT_IXOFF	/* enable input flow control */
#ifndef _POSIX_SOURCE
#define	IXANY		OSKIT_IXANY	/* any char will restart after stop */
#endif  /*_POSIX_SOURCE */

/* Output flags (c_oflag) */
#define	OPOST		OSKIT_OPOST	/* enable following output processing */
#define ONLCR		OSKIT_ONLCR	/* map NL to CR-NL (ala CRMOD) */

/* Control flags (c_cflag) */
#define CSIZE		OSKIT_CSIZE	/* character size mask */
#define     CS5		    OSKIT_CS5	    /* 5 bits (pseudo) */
#define     CS6		    OSKIT_CS6	    /* 6 bits */
#define     CS7		    OSKIT_CS7	    /* 7 bits */
#define     CS8		    OSKIT_CS8	    /* 8 bits */
#define CSTOPB		OSKIT_CSTOPB	/* send 2 stop bits */
#define CREAD		OSKIT_CREAD	/* enable receiver */
#define PARENB		OSKIT_PARENB	/* parity enable */
#define PARODD		OSKIT_PARODD	/* odd parity, else even */
#define HUPCL		OSKIT_HUPCL	/* hang up on last close */
#define CLOCAL		OSKIT_CLOCAL	/* ignore modem status lines */

/* Local flags (c_lflag) */
#define	ECHOE		OSKIT_ECHOE	/* visually erase chars */
#define	ECHOK		OSKIT_ECHOK	/* echo NL after line kill */
#define ECHO		OSKIT_ECHO	/* enable echoing */
#define	ECHONL		OSKIT_ECHONL	/* echo NL even if ECHO is off */
#define	ISIG		OSKIT_ISIG	/* enable signals INTR, QUIT, [D]SUSP */
#define	ICANON		OSKIT_ICANON	/* canonicalize input lines */
#define	IEXTEN		OSKIT_IEXTEN	/* enable DISCARD and LNEXT */
#define TOSTOP		OSKIT_TOSTOP	/* stop background jobs from output */
#define	NOFLSH		OSKIT_NOFLSH	/* don't flush after interrupt */

typedef oskit_tcflag_t	tcflag_t;
typedef oskit_cc_t	cc_t;
typedef oskit_speed_t	speed_t;

/*
 * Just use struct oskit_termios as the actual termios structure.
 * Since C doesn't provide a typedef equivalent for structure tags,
 * and #define'ing termios to oskit_termios causes other problems,
 * we have to incorporate oskit_termios as a substructure
 * and magically #define the individual members...
 */
struct termios {
	oskit_termios_t c_termios;
};
#define c_iflag		c_termios.iflag
#define c_oflag		c_termios.oflag
#define c_cflag		c_termios.cflag
#define c_lflag		c_termios.lflag
#define c_cc		c_termios.cc
#define c_ispeed	c_termios.ispeed
#define c_ospeed	c_termios.ospeed

/* Action codes for tcsetattr() */
#define	TCSANOW		OSKIT_TCSANOW	/* make change immediate */
#define	TCSADRAIN	OSKIT_TCSADRAIN	/* drain output, then change */
#define	TCSAFLUSH	OSKIT_TCSAFLUSH	/* drain output, flush input */

/* Standard speeds */
#define B0	0
#define B50	50
#define B75	75
#define B110	110
#define B134	134
#define B150	150
#define B200	200
#define B300	300
#define B600	600
#define B1200	1200
#define	B1800	1800
#define B2400	2400
#define B4800	4800
#define B9600	9600
#define B19200	19200
#define B38400	38400
#ifndef _POSIX_SOURCE
#define B7200	7200
#define B14400	14400
#define B28800	28800
#define B57600	57600
#define B76800	76800
#define B115200	115200
#define B230400	230400
#endif  /* !_POSIX_SOURCE */

/* Queue selector codes for tcflush() */
#define	TCIFLUSH	OSKIT_TCIFLUSH
#define	TCOFLUSH	OSKIT_TCOFLUSH
#define TCIOFLUSH	OSKIT_TCIOFLUSH

/* Action codes for tcflow() */
#define	TCOOFF		OSKIT_TCOOFF
#define	TCOON		OSKIT_TCOON
#define TCIOFF		OSKIT_TCIOFF
#define TCION		OSKIT_TCION


OSKIT_BEGIN_DECLS

speed_t	cfgetispeed(const struct termios *);
speed_t	cfgetospeed(const struct termios *);
int	cfsetispeed(struct termios *, speed_t);
int	cfsetospeed(struct termios *, speed_t);
int	tcgetattr(int, struct termios *);
int	tcsetattr(int, int, const struct termios *);
int	tcdrain(int);
int	tcflow(int, int);
int	tcflush(int, int);
int	tcsendbreak(int, int);

OSKIT_END_DECLS

#endif /* _OSKIT_C_TERMIOS_H_ */
