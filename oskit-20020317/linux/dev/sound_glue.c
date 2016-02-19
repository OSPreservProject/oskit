/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include <oskit/dev/linux.h>
#include <linux/autoconf.h>		/* CONFIG_foo */

#include "glue.h"

#ifdef CONFIG_SOUND
/*
 * XXX hack fer now interface
 */
#include <linux/sched.h>
#include <linux/fcntl.h>

#include <oskit/error.h>
#include <oskit/fs/file.h>
#include "sound_config.h"

static int sound_opened;
static struct file sound_file;
static struct task_struct sound_task;
static int sound_dev, sound_fd;

int
oskit_linux_sound_open(const char *path, int flags)
{
	struct task_struct *ts, *cur;
	int mode = 0;

	if (sound_opened++)
		return EBUSY;

	/*
	 * XXX minimal task context, just need the sleep rec
	 */
	ts = &sound_task;
	memset(ts, 0, sizeof *ts);

	/*
	 * Map pathname into device
	 */
	if (strncmp(path, "/dev/", 5) == 0)
		path += 5;
	if (strcmp(path, "audio") == 0)
		sound_dev = 4;
	else if (strcmp(path, "dsp") == 0)
		sound_dev = 3;
	else if (strcmp(path, "dsp16") == 0)
		sound_dev = 5;
	else {
		sound_opened = 0;
		return EINVAL;
	}

	/*
	 * File access mode
	 */
	switch (flags & OSKIT_O_ACCMODE) {
	case OSKIT_O_RDONLY:
		mode = FMODE_READ;
		break;
	case OSKIT_O_WRONLY:
		mode = FMODE_WRITE;
		break;
	case OSKIT_O_RDWR:
		mode = FMODE_READ|FMODE_WRITE;
		break;
	}
	sound_file.f_mode = mode;

	/*
	 * Non blocking is the only other flag we care about
	 */
	if (flags & OSKIT_O_NONBLOCK)
		flags = O_NONBLOCK;
	else
		flags = 0;
	sound_file.f_flags = flags;

	cur = current;
	current = &sound_task;
	sound_fd = audio_open(sound_dev, &sound_file);
	current = cur;
	return sound_fd;
}

int
oskit_linux_sound_close(int fd)
{
	struct task_struct *cur;

	if (sound_opened == 0 || fd != sound_fd)
		return EINVAL;

	cur = current;
	current = &sound_task;
	audio_release(sound_dev, &sound_file);
	current = cur;

	sound_opened = 0;
	return 0;
}

int
oskit_linux_sound_read(int fd, char *buf, int count)
{
	struct task_struct *cur;
	int rc;

	if (sound_opened == 0 || fd != sound_fd)
		return EINVAL;

	cur = current;
	current = &sound_task;
	rc = audio_read(sound_dev, &sound_file, buf, count);
	current = cur;
	return rc;
}

int
oskit_linux_sound_write(int fd, const char *buf, int count)
{
	struct task_struct *cur;
	int rc;

	if (sound_opened == 0 || fd != sound_fd)
		return EINVAL;

	cur = current;
	current = &sound_task;
	rc = audio_write(sound_dev, &sound_file, buf, count);
	current = cur;
	return rc;
}

int
oskit_linux_sound_ioctl(int fd, int cmd, void *arg)
{
	struct task_struct *cur;
	int rc;

	if (sound_opened == 0 || fd != sound_fd)
		return EINVAL;

	cur = current;
	current = &sound_task;
	rc = audio_ioctl(sound_dev, &sound_file, cmd, arg);
	current = cur;
	return rc;
}
#endif
