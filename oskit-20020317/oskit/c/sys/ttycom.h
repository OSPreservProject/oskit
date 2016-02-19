#ifndef _OSKIT_C_SYS_TTYCOM_H_
#define _OSKIT_C_SYS_TTYCOM_H_

#include <oskit/c/sys/ioccom.h>

#define	TIOCFLUSH	_IOW('t', 16, int)	/* flush buffers */
#define	TIOCGETA	_IOR('t', 19, struct termios) /* get termios struct */
#define	TIOCSETA	_IOW('t', 20, struct termios) /* set termios struct */
#define	TIOCSETAW	_IOW('t', 21, struct termios) /* drain output, set */
#define	TIOCSETAF	_IOW('t', 22, struct termios) /* drn out, fls in, set */
#define	TIOCDRAIN	_IO('t', 94)		/* wait till output drained */

#define	TIOCSTART	_IO('t', 110)		/* start output, like ^Q */
#define	TIOCSTOP	_IO('t', 111)		/* stop output, like ^S */
#define TIOCNOTTY	_IO('t', 113)          /* void tty association */

#endif /* _OSKIT_C_SYS_TTYCOM_H_ */
