#!/bin/sh
#
# Copyright (c) 1996-1999 The University of Utah and the Flux Group.
# 
# This file is part of the OSKit Linux Glue Libraries, which are free
# software, also known as "open source;" you can redistribute them and/or
# modify them under the terms of the GNU General Public License (GPL),
# version 2, as published by the Free Software Foundation (FSF).
# 
# The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
# received a copy of the GPL along with the OSKit; see the file COPYING.  If
# not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
#

# This is used to generate the shared/global.h file.
# It is meant to be run by hand,
# not from a makefile or anything.
# You would run it where the .o files for the shared stuff have
# already been built.
# What it does is go look in the shared src tree for the filenames that
# comprise the shared portion of the linux fs and dev libraries,
# and then runs `nm' on them to figure out which symbols need to be
# prefixed with OSKIT_LINUX_.

OSKIT_SRCDIR=$HOME/oskit
files=

#
# Edit these ...
# 
nm=nm					# $prefix depends on this too
prefix=_				# for a.out nm
linux_arch=i386
oskit_arch=x86

for d in 	${OSKIT_SRCDIR}/linux/shared				\
		${OSKIT_SRCDIR}/linux/shared/${oskit_arch}		\
		${OSKIT_SRCDIR}/linux/src/lib				\
		${OSKIT_SRCDIR}/linux/src/arch/${linux_arch}/lib	\
		${OSKIT_SRCDIR}/linux/src/arch/${linux_arch}/kernel	
do
	files="$files `find $d -name '*.[csS]' -print | sed 's-.*/--;s-\.[csS]-.o-'`"
done

echo $files

$nm $files \
| grep ' [TDRC] ' \
| awk '{ print $3 }' \
| egrep -vi '^(oskit_|fs_linux|fdev_)' \
| sed "s/^$prefix//" \
| awk '{ printf "#define "$1" OSKIT_LINUX_"$1"\n"; }' \
| sort
