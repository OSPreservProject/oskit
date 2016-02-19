#include <stdio.h>
#include <stdarg.h>


#define NeedVarargsPrototypes 1

#if NeedVarargsPrototypes
void
VErrorF(f, args)
    char *f;
    va_list args;
{
    vfprintf(stderr, f, args);
}
#endif

/*VARARGS1*/
void
ErrorF(
#if NeedVarargsPrototypes
    char * f, ...)
#else
 f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
    fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif
	}

/*VARARGS1*/
void
FatalError(
#if NeedVarargsPrototypes
    char *f, ...)
#else
f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
#endif
{
#if NeedVarargsPrototypes
    va_list args;
#endif
    ErrorF("\nFatal server error:\n");
#if NeedVarargsPrototypes
    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
#else
    ErrorF(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
#endif
    ErrorF("\n");
#ifdef DDXOSFATALERROR
    OsVendorFatalError();
#endif
#ifndef OSKIT
    AbortServer();
#else
    exit();
#endif
/*NOTREACHED*/
}

