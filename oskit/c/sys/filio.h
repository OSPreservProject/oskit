#ifndef _OSKIT_C_SYS_FILIO_H_
#define _OSKIT_C_SYS_FILIO_H_

#include <oskit/c/sys/ioccom.h>

/* Generic file-descriptor ioctl's. */
#define FIOASYNC        _IOW('f', 125, int)     /* set/clear async i/o */
#define FIONBIO         _IOW('f', 126, int)     /* set/clear non-blocking i/o */
#define FIONREAD        _IOR('f', 127, int)     /* get # bytes to read */

#endif /* _OSKIT_C_SYS_FILIO_H_ */
