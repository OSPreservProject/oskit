/*
 * Copyright (c) 1994-2001 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_STDIO_H
#define _OSKIT_C_STDIO_H

#include <oskit/types.h>
#include <oskit/compiler.h>

/* This is a very naive standard I/O implementation
   which simply chains to the low-level I/O routines
   without doing any buffering or anything.  */

#ifndef NULL
#define NULL 0
#endif

/*
 * This stuff comes from the FreeBSD C library header file. It is
 * duplicated here so that the min C library is binary compatible
 * with the FreeBSD C library
 */
#define __P(protos)	protos

typedef oskit_off_t	fpos_t;

/* stdio buffers */
struct __sbuf {
	unsigned char *_base;
	int	_size;
};

typedef	struct __sFILE {
	unsigned char *_p;	/* current position in (some) buffer */
	int	_r;		/* read space left for getc() */
	int	_w;		/* write space left for putc() */
	short	_flags;		/* flags, below; this FILE is free if 0 */
	short	_file;		/* fileno, if Unix descriptor, else -1 */
	struct	__sbuf _bf;	/* the buffer (at least 1 byte, if !NULL) */
	int	_lbfsize;	/* 0 or -_bf._size, for inline putc */

	/* operations */
	void	*_cookie;	/* cookie passed to io functions */
	int	(*_close) __P((void *));
	int	(*_read)  __P((void *, char *, int));
	fpos_t	(*_seek)  __P((void *, fpos_t, int));
	int	(*_write) __P((void *, const char *, int));

	/* separate buffer for long sequences of ungetc() */
	struct	__sbuf _ub;	/* ungetc buffer */
	unsigned char *_up;	/* saved _p when _p is doing ungetc data */
	int	_ur;		/* saved _r when _r is counting ungetc data */

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char _ubuf[3];	/* guarantee an ungetc() buffer */
	unsigned char _nbuf[1];	/* guarantee a getc() buffer */

	/* separate buffer for fgetln() when line crosses buffer boundary */
	struct	__sbuf _lb;	/* buffer for fgetln() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int	_blksize;	/* stat.st_blksize (may be != _bf._size) */
	fpos_t	_offset;	/* current lseek offset (see WARNING) */
} FILE;

extern FILE __sF[];

#define	__SEOF		0x0020		/* found EOF */
#define	__SERR		0x0040		/* found error */

#define	stdin		(&__sF[0])
#define	stdout		(&__sF[1])
#define	stderr		(&__sF[2])

#define	feof(p)		(((p)->_flags & __SEOF) != 0)
#define	ferror(p)	(((p)->_flags & __SERR) != 0)
#define	clearerr(p)	((void)((p)->_flags &= ~(__SERR|__SEOF)))
#define	fileno(p)	((p)->_file)

/*
 * End of FreeBSD duplicated stuff.
 */

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifndef EOF
#define EOF -1
#endif

#ifndef BUFSIZ
#define BUFSIZ	1024
#endif

OSKIT_BEGIN_DECLS

int putchar(int __c);
int puts(const char *__str);
int printf(const char *__format, ...);
int vprintf(const char *__format, oskit_va_list __vl);
int sprintf(char *__dest, const char *__format, ...);
int snprintf(char *__dest, int __size, const char *__format, ...);
int vsprintf(char *__dest, const char *__format, oskit_va_list __vl);
int vsnprintf(char *__dest, int __size, const char *__format, oskit_va_list __vl);
int getchar(void);
char *gets(char *__str);
char *fgets(char *__str, int __size, FILE *__stream);
FILE *fopen(const char *__path, const char *__mode);
FILE *fdopen(int fd, const char *__mode);
int fflush(FILE *stream);
int fclose(FILE *__stream);
int fread(void *__buf, int __size, int __count, FILE *__stream);
int fwrite(void *__buf, int __size, int __count, FILE *__stream);
int fputc(int __c, FILE *__stream);
int fputs(const char *str, FILE *stream);
int fgetc(FILE *__stream);
int fprintf(FILE *__stream, const char *__format, ...);
int vfprintf(FILE *__stream, const char *__format, oskit_va_list __vl);
int fscanf(FILE *__stream, const char *__format, ...);
int sscanf(const char *__str, const char *__format, ...);
int fseek(FILE *__stream, long __offset, int __whence);
long ftell(FILE *__stream);
void rewind(FILE *__stream);
int rename(const char *__from, const char *__to);
int remove(const char *__path);
void dohexdump(void *__base, void *__buf, int __len, int __bytes);
#define hexdumpb(base, buf, nbytes) dohexdump(base, buf, nbytes, 0)
#define hexdumpw(base, buf, nwords) dohexdump(base, buf, nwords, 1)
void perror(const char *__string);

#define putc(c, stream) fputc(c, stream)

struct oskit_stream;		/* see <oskit/com/stream.h> */
int com_printf(struct oskit_stream *__stream, const char *__format, ...);
int com_vprintf(struct oskit_stream *__stream, const char *__format,
		oskit_va_list __vl);

#include <oskit/io/absio.h>
oskit_error_t fd_get_absio(int fd, oskit_absio_t **out_absio);

OSKIT_END_DECLS

#endif /* _OSKIT_C_STDIO_H */
