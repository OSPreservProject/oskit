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
#ifndef _OSKIT_C_MATH_H_
#define _OSKIT_C_MATH_H_

#include <oskit/compiler.h>

#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#define	MAXFLOAT	((float)3.40282346638528860e+38)
#define HUGE_VAL	1e500			/* positive infinity */

extern int signgam;

double acos(double x) OSKIT_PURE;
double asin(double x) OSKIT_PURE;
double atan(double x) OSKIT_PURE;
double atan2(double y, double x) OSKIT_PURE;
double ceil(double x) OSKIT_PURE;
double cos(double x) OSKIT_PURE;
double cosh(double x) OSKIT_PURE;
double exp(double x) OSKIT_PURE;
double fabs(double x) OSKIT_PURE;
double floor(double x) OSKIT_PURE;
double fmod(double x, double y) OSKIT_PURE;
double frexp(double value, int *exp);
double ldexp(double value, int exp) OSKIT_PURE;
double log(double x) OSKIT_PURE;
double log10(double x) OSKIT_PURE;
double modf(double value, double *iptr);
double pow(double x, double y) OSKIT_PURE;
double sin(double x) OSKIT_PURE;
double sinh(double x) OSKIT_PURE;
double sqrt(double x) OSKIT_PURE;
double tan(double x) OSKIT_PURE;
double tanh(double x) OSKIT_PURE;

double erf(double x) OSKIT_PURE;
double erfc(double x) OSKIT_PURE;
double gamma(double x);
double hypot(double x, double y) OSKIT_PURE;
double j0(double x) OSKIT_PURE;
double j1(double x) OSKIT_PURE;
double jn(int n, double x) OSKIT_PURE;
double lgamma(double x);
double y0(double x) OSKIT_PURE;
double y1(double x) OSKIT_PURE;
double yn(int n, double x) OSKIT_PURE;
int isnan(double x) OSKIT_PURE;

double acosh(double x) OSKIT_PURE;
double asinh(double x) OSKIT_PURE;
double atanh(double x) OSKIT_PURE;
double cbrt(double x) OSKIT_PURE;
double expm1(double x) OSKIT_PURE;
int ilogb(double x) OSKIT_PURE;
double log1p(double x) OSKIT_PURE;
double logb(double x) OSKIT_PURE;
double nextafter(double x, double y) OSKIT_PURE;
double remainder(double x, double y) OSKIT_PURE;
double rint(double l) OSKIT_PURE;
double scalb(double x, double n) OSKIT_PURE;

#endif /* _OSKIT_C_MATH_H_ */
