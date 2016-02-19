/*
 * Copyright (c) 1997,1998,1999,2000 The University of Utah and the Flux Group.
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
 * Random glue code.
 */

#include <oskit/c/stdlib.h>

#include <linux/mm.h>
#include <linux/swapctl.h>
#include <linux/sched.h>
#include <linux/config.h>

/* Linux uses this for inode versions. */
unsigned long event = 0;

/* min_free_pages is arbitrary. */
int min_free_pages = 20;

/* nr_free_pages is only compared to min_free_pages, so we kludge it to 1000 */
int nr_free_pages = 1000;

/* mem_map is only indexed by MAP_NR, which we make always return 0. */
static mem_map_t mem_map_init = {
	NULL,				/* next */
	NULL,				/* prev */
	NULL,				/* inode */
	0,				/* offset */
	0,				/* next_hash */
	{1},				/* count */
	0,				/* flags */
	NULL,				/* wait */
	NULL,				/* prev_hash */
	NULL,				/* buffers */
};
mem_map_t *mem_map = &mem_map_init;

DECLARE_TASK_QUEUE(tq_disk);

/* These are referenced by some of the fs files but shouldn't ever be used. */
struct inode_operations chrdev_inode_operations = { NULL, NULL, };
struct inode_operations blkdev_inode_operations = { NULL, NULL, };

/* Linux changes this with sysctl calls, we leave it zero. */
int securelevel = 0;

/* This is supposed to contain timezone info. */
struct timezone sys_tz = {0,0};

/*
 * Taken verbatim from Linux's fs/open.c.
 */
int
do_truncate(struct dentry *dentry, unsigned long length)
{
	struct inode *inode = dentry->d_inode;
	int error;
	struct iattr newattrs;

	/* Not pretty: "inode->i_size" shouldn't really be "off_t". But it is. */
	if ((off_t) length < 0)
		return -EINVAL;

	down(&inode->i_sem);
	newattrs.ia_size = length;
	newattrs.ia_valid = ATTR_SIZE | ATTR_CTIME;
	error = notify_change(dentry, &newattrs);
	if (!error) {
		/* truncate virtual mappings of this file */
#ifndef OSKIT_FS
		vmtruncate(inode, length);
#endif
		if (inode->i_op && inode->i_op->truncate)
			inode->i_op->truncate(inode);
	}
	up(&inode->i_sem);
	return error;
}

/*
 * Verbatim from Linux's kernel/sys.c.
 */
int in_group_p(gid_t grp)
{
	int	i;

	if (grp == current->fsgid)
		return 1;

	for (i = 0; i < NGROUPS; i++) {
		if (current->groups[i] == NOGROUP)
			break;
		if (current->groups[i] == grp)
			return 1;
	}
	return 0;
}

/*
 * Verbatim from Linux's fs/open.c.
 */
void __fput(struct file *filp)
{
	struct dentry * dentry = filp->f_dentry;
	struct inode * inode = dentry->d_inode;

	if (filp->f_op && filp->f_op->release)
		filp->f_op->release(inode, filp);
	filp->f_dentry = NULL;
	if (filp->f_mode & FMODE_WRITE)
		put_write_access(inode);
	dput(dentry);
}

/*
 * NOT Verbatim from Linux's fs/file_table.c
 */
void fput(struct file *file)
{
	int count = file->f_count-1;

	if (!count) {
		__fput(file);
		file->f_count = 0;
	} else
		file->f_count = count;
}

/*
 * From fs/devices.c
 */
char * bdevname(kdev_t dev)
{
	static char buffer[32];
	const char * name = NULL;

	if (!name)
		name = "unknown-block";

	sprintf(buffer, "%s(%d,%d)", name, MAJOR(dev), MINOR(dev));
	return buffer;
}

/*
 * No way to deal with this right now.
 */
int send_sig(int sig, struct task_struct * p, int priv)
{
	printk("Linux send_sig called with sig %d\n", sig);
	return 0;
}

#include <linux/string.h>
#include <linux/ctype.h>

#ifndef __HAVE_ARCH_STRNICMP
/*
 * This is used only by the linux FS code. If its not inlined, define it.
 */
int
strnicmp(const char *s1, const char *s2, size_t n)
{
	while (1)
	{
		unsigned char c1, c2;

		if (n <= 0)
			return 0;
		c1 = tolower(*s1);
		c2 = tolower(*s2);
		if (c1 != c2)
			return c1 - c2;
		if (c1 == 0)
			return 0;

		s1++;
		s2++;
		n--;
	}
}
#endif

#ifndef __HAVE_ARCH_STRNLEN
/*
 * This is used only by the linux DEV code. If its not inlined, define it.
 */
size_t
strnlen(const char * s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}
#endif

#ifndef __HAVE_ARCH_MEMSCAN
void * memscan(void * addr, int c, size_t size)
{
	unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (void *) p;
		p++;
		size--;
	}
  	return (void *) p;
}
#endif

#ifdef CONFIG_BLK_DEV_FD
/*
 * XXX stub code needed by the Linux FS code to support filesystems on
 * floppies.  Bleh!
 */
#include <oskit/console.h>

void
wait_for_keypress(void)
{
	(void)console_getchar();
}
#endif
