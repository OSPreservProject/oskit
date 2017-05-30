/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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

/*
 * OSKit glue for the little minimad decoder example in the mad distribution.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/soundcard.h>

#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/page.h>

extern int oskit_linux_sound_open(const char *path, int flags);
extern int oskit_linux_sound_write(int fd, const char *buf, int count);
extern int oskit_linux_sound_ioctl(int fd, int cmd, void *arg);
extern int oskit_linux_sound_close(int fd);

static int sfd, mp3fd;
static char soundbuf[64], *sbp = soundbuf;

int minimad_main(int argc, char *argv[]);

/*
 * Main: setup OSKit stuff and
 * redirect stdin from a file before calling minimad's main.
 */
int
main(int argc, char *argv[])
{
	int a, i;
	char *mmargv[2] = { "minimad", 0 };

	printf("OSKit mp3 player\n");

	oskit_clientos_init();
	start_sound_devices();
	start_fs_bmod();

	if (argc < 2) {
		printf("Usage: %s mp3file ...\n", argv[0]);
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		mp3fd = open(argv[i], O_RDONLY);
		if (mp3fd < 0) {
			printf("%s: could not open, ignoring\n", argv[i]);
			continue;
		}

		if ((sfd = oskit_linux_sound_open("/dev/dsp", O_WRONLY)) == -1)
			panic("dsp: could not open");

		if (oskit_linux_sound_ioctl(sfd, SNDCTL_DSP_SYNC, 0) == -1)
			panic("dsp: sync failed");

		a = AFMT_S16_LE;
		if (oskit_linux_sound_ioctl(sfd, SNDCTL_DSP_SETFMT, &a) == -1)
			panic("dsp: SNDCTL_DSP_SETFMT failed");

		a = 2;
		if (oskit_linux_sound_ioctl(sfd, SNDCTL_DSP_CHANNELS, &a) == -1)
			panic("dsp: SNDCTL_DSP_CHANNELS failed");

		a = 44100;
		if (oskit_linux_sound_ioctl(sfd, SNDCTL_DSP_SPEED, &a) == -1)
			panic("dsp: SNDCTL_DSP_SPEED failed");

		if (minimad_main(1, mmargv) != 0)
			panic("minimad failed");

		oskit_linux_sound_close(sfd);
		close(mp3fd);
	}

	return 0;
}

/*
 * Mmap reads the file into a buffer
 */
void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	void *mem;

	if (addr != 0 || prot != PROT_READ || fd != STDIN_FILENO || offset != 0)
		panic("mmap: bad args");

	mem = memalign(PAGE_SIZE, len);
	if (mem == 0)
		return MAP_FAILED;

	if (read(mp3fd, mem, len) != len) {
		printf("mmap: incomplete read\n");
		free(mem);
		return MAP_FAILED;
	}
	return mem;
}

/*
 * Munmap frees the mmap buffer
 */
int
munmap(void *addr, size_t len)
{
	if (sbp != soundbuf) {
		if (oskit_linux_sound_write(sfd, soundbuf, sbp-soundbuf) < 0)
			panic("dsp: write failed");
		sbp = soundbuf;
	}
	free(addr);
	return 0;
}

/*
 * Make stat of STDIN_FILENO return stat for our real file instead
 */
int
minimad_fstat(int fd, struct stat *sb)
{
	if (fd != STDIN_FILENO)
		panic("fstat: bad arg");

	return fstat(mp3fd, sb);
}

/*
 * Make putchar go to the audio device
 */
static inline int
minimad_putchar(int c)
{
	*sbp++ = c;
	if (sbp == &soundbuf[64]) {
		if (oskit_linux_sound_write(sfd, soundbuf, sizeof soundbuf) < 0)
			panic("dsp: write failed");
		sbp = soundbuf;
	}
	return c;
}

#define main minimad_main
#define fstat minimad_fstat
#define putchar minimad_putchar

/*
 * Include the real minimad program now that we have redefined everything
 * we need to.
 */
#include <minimad.c>
