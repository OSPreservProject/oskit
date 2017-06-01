/*
 * Copyright (c) 1997-2000 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Main initialization for the Linux fs code
 */

#include <oskit/types.h>

#include <linux/fs.h>

#include "dev.h"
#include "shared.h"
#include "hashtab.h"

/*
 * Hash table
 */
hashtab_t		dentrytab;	/* dentry -> oskit_file_t */
static oskit_error_t	dentrytab_init(void);
#define DENTRYTAB_SIZE	103

#ifndef KNIT
/*
 * Intialize the real filesystems.
 */
static void
real_fs_init()
{
#define filesystem(name, desc, init) {			\
	extern int init(void);				\
	printk("Initializing %s - %s\n", #name, desc);	\
	init();						\
}
#include <oskit/fs/linux_filesystems.h>
#undef filesystem
}
#endif /* !KNIT */

/*
 * Main initialization for the Linux fs code
 */
oskit_error_t
fs_linux_init(void)
{
#ifndef KNIT
	static int inited = 0;

	if (inited)
		return 0;
#endif		

	/* Initialize the glue code. */
	oskit_linux_init();

	/* Initialize the device stuff we will be using. */
	fs_linux_dev_init();

	/* Initialize the Linux code. */
	inode_init();
	dcache_init();
	buffer_init(0x1000000);

	/* Create the dentry -> oskit_file_t hashtable */
	dentrytab_init();
#ifndef KNIT
	real_fs_init();

	inited = 1;
#endif
	return 0;
}


/*
 * Hashtab support.
 */
static unsigned int
dentryhash(hashtab_key_t key)
{
	unsigned int h;
	
	h = (unsigned int) key;
	return h % DENTRYTAB_SIZE;
}

static int
dentrycmp(hashtab_key_t key1, hashtab_key_t key2)
{
	return !(key1 == key2);
}

static int
dentrytab_init(void)
{
	dentrytab = hashtab_create(dentryhash, dentrycmp, DENTRYTAB_SIZE);
	if (!dentrytab)
		return OSKIT_ENOMEM;
	return 0;
}
