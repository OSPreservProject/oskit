#
# Copyright (c) 1997-1998 The University of Utah and the Flux Group.
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

#
# This is used in the makefile to generate a file called `fileystems'.
# See the makefile for more info.
#

BEGIN {
	printf "FILESYSTEMS =\n\n";
}

#
# Example line:
#	filesystem ext2 "Second extended filesystem" init_ext2_fs
#

/^filesystem/ {
	printf "FILESYSTEMS += %s\n", $2;

	## Rule to link src files to scoped names.
	## This is needed so the -MD flag to gcc will generate .d files
	## listing a dependency for ext2_foo.o.
	printf "%s_%%.c: $(OSKIT_SRCDIR)/linux/src/fs/%s/%%.c\n", $2, $2;
	printf "\t$(OSKIT_QUIET_MAKE_INFORM) \"Creating Link $@\"\n";
	printf "\tln -s $^ $@\n";

	## Rule to compile the .c files with an extra -I path so they
	## can #include relative paths.
	printf "%s_%%.o: %s_%%.c\n", $2, $2;
	printf "\t$(OSKIT_QUIET_MAKE_INFORM) \"Generating $@\"\n";
	printf "\t$(CC) -c -o $@ -I$(OSKIT_SRCDIR)/linux/src/fs/%s $(OSKIT_CFLAGS) $(CFLAGS) $<\n", $2;

	printf "%s_%%.po: %s_%%.c\n", $2, $2;
	printf "\t$(OSKIT_QUIET_MAKE_INFORM) \"Generating $@\"\n";
	printf "\t$(CC) -c -o $@ -I$(OSKIT_SRCDIR)/linux/src/fs/%s $(OSKIT_CFLAGS) -pg $(CFLAGS) $<\n", $2;

	printf "CLEAN_FILES += %s_*.c\n\n", $2;
}

