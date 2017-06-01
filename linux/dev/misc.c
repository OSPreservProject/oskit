/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
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
 * Miscellaneous Linux support functions and variables.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/blk.h>
#include <linux/proc_fs.h>
#include <linux/kernel_stat.h>
#include <linux/time.h>
#include <linux/module.h>

#include "linux_emul.h"
#include "osenv.h"

int (*dispatch_scsi_info_ptr)(int ino, char *buffer, char **start,
			      off_t offset, int length, int inout) = 0;

struct kernel_stat kstat;

int
linux_to_oskit_error(int err)
{
	switch (err) {

	case 0:
		return (0);

	case -EPERM:
		return (OSKIT_E_DEV_BADOP);

	case -EIO:
		return (OSKIT_E_DEV_IOERR);

	case -ENXIO:
		return (OSKIT_E_DEV_NOSUCH_DEV);

	case -EACCES:
		return (OSKIT_E_DEV_BADOP);

	case -EFAULT:
		return (OSKIT_E_DEV_BADPARAM);

	case -EBUSY:
		return (OSKIT_E_DEV_IOERR);	/* XXX */

	case -EINVAL:
		return (OSKIT_E_DEV_BADPARAM);

	case -EROFS:
		return (OSKIT_E_DEV_BADOP);

	case -EWOULDBLOCK:
		return (OSKIT_E_DEV_IOERR);	/* XXX */

	case -ENOMEM:
		return (OSKIT_E_OUTOFMEMORY);

	default:
		osenv_log(OSENV_LOG_NOTICE, "%s:%d: unknown code %d\n", 
			__FILE__, __LINE__, err);
		return (OSKIT_E_DEV_IOERR);
	}
}

int
block_fsync(struct file *filp, struct dentry *d)
{
	return (0);
}

struct proc_dir_entry proc_scsi;
struct inode_operations proc_scsi_inode_operations;
struct proc_dir_entry proc_net;
struct inode_operations proc_net_inode_operations;

int
proc_register (struct proc_dir_entry *xxx1, struct proc_dir_entry *xxx2)
{
	return 0;
}

int
proc_unregister (struct proc_dir_entry *xxx1, int xxx2)
{
	return 0;
}

void
add_blkdev_randomness (int major)
{
}

int
dev_get_info (char *buffer, char **start, off_t offset, int length, int dummy)
{
	return 0;
}

void
do_gettimeofday(struct timeval *tv)
{
	tv->tv_sec = jiffies;	/* XXX! */
	tv->tv_usec = 0;
}

/*
 * XXX: These *really* need to be fixed
 */

unsigned long
virt_to_phys(volatile void *address)
{
	return ((unsigned long)osenv_mem_get_phys((oskit_addr_t)address));
}

void *
phys_to_virt(unsigned long address)
{
	return ((void *)osenv_mem_get_virt((oskit_addr_t)address));
}

void
iounmap(void *addr)
{
}

#include <linux/string.h>
#include <linux/ctype.h>

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
