#ifndef _OSKIT_C_SYS_IOCCOM_H_
#define _OSKIT_C_SYS_IOCCOM_H_

#include <oskit/compiler.h>

/*
 * Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 */
#define IOCPARM_MASK    0x1fff          /* parameter length, at most 13 bits */
#define IOCPARM_LEN(x)  (((x) >> 16) & IOCPARM_MASK)
#define IOCBASECMD(x)   ((x) & ~(IOCPARM_MASK << 16))
#define IOCGROUP(x)     (((x) >> 8) & 0xff)

#define IOC_VOID        0x20000000      /* no parameters */
#define IOC_OUT         0x40000000      /* copy out parameters */
#define IOC_IN          0x80000000      /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
#define IOC_DIRMASK     0xe0000000      /* mask for IN/OUT/VOID */

#define _IOC(inout,group,num,len) \
        (inout | ((len & IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define _IO(g,n)        _IOC(IOC_VOID,  (g), (n), 0)
#define _IOR(g,n,t)     _IOC(IOC_OUT,   (g), (n), sizeof(t))
#define _IOW(g,n,t)     _IOC(IOC_IN,    (g), (n), sizeof(t))
/* this should be _IORW, but stdio got there first */
#define _IOWR(g,n,t)    _IOC(IOC_INOUT, (g), (n), sizeof(t))

OSKIT_BEGIN_DECLS
int     ioctl (int, unsigned long, ...);
OSKIT_END_DECLS

#endif /* _OSKIT_C_SYS_IOCCOM_H_ */
