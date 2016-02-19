dnl
dnl Copyright (c) 1998, 1999 The University of Utah and the Flux Group.
dnl All rights reserved.
dnl 
dnl This file is part of the Flux OSKit.  The OSKit is free software, also known
dnl as "open source;" you can redistribute it and/or modify it under the terms
dnl of the GNU General Public License (GPL), version 2, as published by the Free
dnl Software Foundation (FSF).  To explore alternate licensing terms, contact
dnl the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
dnl 
dnl The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
dnl WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
dnl FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
dnl received a copy of the GPL along with the OSKit; see the file COPYING.  If
dnl not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
dnl
dnl Macro for a simple compile check to determine an AC_DEFINE setting.
dnl $1 = "checking" message string (for blah blah)
dnl $2 = C code to try compiling inside a function
dnl $3 = macro to AC_DEFINE if it works
dnl $4 = (optional) extra commands to run before that AC_DEFINE
dnl The cache variable name will be `flux_cv_$3', and the $4 code
dnl can reset its value to "no" to prevent the AC_DEFINE($3) from happening.
dnl
AC_DEFUN(flux_CACHED_COMPILE_CHECK,[
AC_CACHE_CHECK($1, flux_cv_$3, [
AC_TRY_COMPILE([], $2, flux_cv_$3=yes, flux_cv_$3=no)])
ifelse($4,,,[dnl
if test [$]flux_cv_$3 = yes; then
	$4
fi
])dnl
if test [$]flux_cv_$3 = yes; then
	AC_DEFINE($3)
fi])dnl

dnl
dnl Macro to handle --enable-foo arguments.
dnl Like AC_ARG_ENABLE, but $3 is not run for --disable-foo.
dnl
AC_DEFUN(flux_ARG_ENABLE,[dnl
AC_ARG_ENABLE([$1], [$2], [
if test ["x${enableval}"] != xno; then
  :
  $3
fi])])dnl

dnl
dnl Macro to find perl (or let the user specify --with-perl)
dnl
AC_DEFUN(flux_PERLINFO,
[AC_ARG_WITH(perl,
[  --with-perl=PATH        Specify perl executable to use
  --without-perl          No perl],
[case "$withval" in
	no) NOPERL="true" ;;
	yes|"") ;;
	*) PERL="$withval" ;;
esac])dnl

if test "$NOPERL" = "true"
then
	echo >&AC_FD_MSG 'skipping perl checks'
else
	if test -z "$PERL" || test ! -x "$PERL"
	then
		AC_PATH_PROGS(PERL, perl5 perl)
	fi
	if test ! -x "$PERL"
	then
		NOPERL="true"
		AC_MSG_WARN([could not locate perl (try --with-perl=PATH)])
	fi
	AC_SUBST(PERL)
fi])dnl
