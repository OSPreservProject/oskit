/*
 * Copyright (c) 1994-1999 University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Boot Loader, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * misc.c
 * 
 * This is a collection of several routines from gzip-1.0.3 
 * adapted for Linux.
 *
 * malloc by Hannu Savolainen 1993 and Matthias Urlichs 1994
 * puts by Nick Holloway 1993
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <oskit/debug.h>
#include <oskit/lmm.h>
#include <oskit/exec/exec.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/pc/base_i16.h>
#include <oskit/x86/pc/phys_lmm.h>

#include "gzip.h"
#include "lzw.h"

#include <linux/config.h>
#include <linux/segment.h>

#define fcalloc calloc

/*
 * These are set up by the setup-routine at boot-time:
 */

struct screen_info {
	unsigned char  orig_x;
	unsigned char  orig_y;
	unsigned char  unused1[2];
	unsigned short orig_video_page;
	unsigned char  orig_video_mode;
	unsigned char  orig_video_cols;
	unsigned short orig_video_ega_ax;
	unsigned short orig_video_ega_bx;
	unsigned short orig_video_ega_cx;
	unsigned char  orig_video_lines;
};

/*
 * This is set up by the setup-routine at boot-time
 */
#define EXT_MEM_K (*(unsigned short *)0x90002)
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define SCREEN_INFO (*(struct screen_info *)0x90000)
#define RAMDISK_SIZE (*(unsigned short *)0x901F8)
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)
#define AUX_DEVICE_INFO (*(unsigned char *)0x901FF)

#define EOF -1

DECLARE(uch, inbuf, INBUFSIZ);
DECLARE(uch, outbuf, OUTBUFSIZ+OUTBUF_EXTRA);
DECLARE(uch, window, WSIZE);

unsigned outcnt;
unsigned insize;
unsigned inptr;

static char *input_data;
static int input_len;
static int input_ptr;

int method, exit_code, part_nb, last_member;
int test = 0;
int force = 0;
int verbose = 1;
long bytes_in, bytes_out;

int to_stdout = 0;
int hard_math = 0;

void (*work)(int inf, int outf);
void makecrc(void);

struct outbuf {
	struct outbuf *next;
	unsigned size;
	char data[0];
};
static struct outbuf *first_out, *last_out;

local int get_method(int);

extern ulg crc_32_tab[];   /* crc table, defined below */

/* ===========================================================================
 * Run a set of bytes through the crc shift register.  If s is a NULL
 * pointer, then initialize the crc shift register contents instead.
 * Return the current crc in either case.
 */
ulg updcrc(s, n)
    uch *s;                 /* pointer to bytes to pump through */
    unsigned n;             /* number of bytes in s[] */
{
    register ulg c;         /* temporary variable */

    static ulg crc = (ulg)0xffffffffL; /* shift register contents */

    if (s == NULL) {
	c = 0xffffffffL;
    } else {
	c = crc;
	while (n--) {
	    c = crc_32_tab[((int)c ^ (*s++)) & 0xff] ^ (c >> 8);
	}
    }
    crc = c;
    return c ^ 0xffffffffL;       /* (instead of ~c for 64-bit machines) */
}

/* ===========================================================================
 * Clear input and output buffers
 */
void clear_bufs()
{
    outcnt = 0;
    insize = inptr = 0;
    bytes_in = bytes_out = 0L;
}

/* ===========================================================================
 * Fill the input buffer. This is called only when the buffer is empty
 * and at least one byte is really needed.
 */
int fill_inbuf()
{
    int len, i;

    /* Read as much as possible */
    insize = 0;
    do {
	len = INBUFSIZ-insize;
	if (len > (input_len-input_ptr+1)) len=input_len-input_ptr+1;
        if (len == 0 || len == EOF) break;

        for (i=0;i<len;i++) inbuf[insize+i] = input_data[input_ptr+i];
	insize += len;
	input_ptr += len;
    } while (insize < INBUFSIZ);

    if (insize == 0) {
	error("unable to fill buffer\n");
    }
    bytes_in += (ulg)insize;
    inptr = 1;
    return inbuf[0];
}

/* ===========================================================================
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
void flush_window()
{
	struct outbuf *buf;

	if (outcnt == 0) return;
	updcrc(window, outcnt);

	/*
	 * Stash the data in a dynamically allocated output buffer,
	 * and chain it onto the list of output buffers.
	 */
	buf = malloc(sizeof(*buf) + outcnt);
	if (last_out == NULL)
		first_out = last_out = buf;
	else
		last_out = last_out->next = buf;
	buf->next = NULL;
	buf->size = outcnt;
	memcpy(buf->data, window, outcnt);

	bytes_out += (ulg)outcnt;
	outcnt = 0;
}

/*
 * Code to compute the CRC-32 table. Borrowed from 
 * gzip-1.0.3/makecrc.c.
 */

ulg crc_32_tab[256];

void
makecrc(void)
{
/* Not copyrighted 1990 Mark Adler	*/

  unsigned long c;      /* crc shift register */
  unsigned long e;      /* polynomial exclusive-or pattern */
  int i;                /* counter for all possible eight bit values */
  int k;                /* byte being shifted into crc apparatus */

  /* terms of polynomial defining this crc (except x^32): */
  static int p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  /* Make exclusive-or pattern from polynomial */
  e = 0;
  for (i = 0; i < sizeof(p)/sizeof(int); i++)
    e |= 1L << (31 - p[i]);

  crc_32_tab[0] = 0;

  for (i = 1; i < 256; i++)
  {
    c = 0;
    for (k = i | 256; k != 1; k >>= 1)
    {
      c = c & 1 ? (c >> 1) ^ e : c >> 1;
      if (k & 1)
        c ^= e;
    }
    crc_32_tab[i] = c;
  }
}

void error(char *x)
{
	panic(x);
}

#include "boot.h"

static struct multiboot_info *boot_info;
static struct multiboot_module *boot_mods;

struct multiboot_header boot_kern_hdr;
void *boot_kern_image;
static struct exec_info boot_kern_info;

struct zmod
{
	oskit_addr_t start, end;
	oskit_addr_t cmdline;
};
struct zhdr
{
	int magic;
	struct zmod zmods[0];
};

static struct zhdr *z;

static oskit_size_t zread(oskit_addr_t file_ofs, void *buf, oskit_size_t size)
{
	struct outbuf *obuf;
	oskit_size_t actual = 0;
	oskit_size_t bsize;

	for (obuf = first_out; obuf != NULL && size > 0; obuf = obuf->next) {
		if (file_ofs >= obuf->size) {
			file_ofs -= obuf->size;
			continue;
		}
		bsize = obuf->size - file_ofs;
		if (bsize > size)
			bsize = size;
		memcpy(buf, obuf->data + file_ofs, bsize);
		file_ofs = 0;
		buf += bsize;
		size -= bsize;
		actual += bsize;
	}

	return actual;
}

static
int kimg_read(void *handle, oskit_addr_t file_ofs, void *buf,
		oskit_size_t size, oskit_size_t *out_actual)
{
	*out_actual = zread(z->zmods[0].start + file_ofs, buf, size);
	return 0;
}

static
int kimg_read_exec_1(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		     oskit_addr_t mem_addr, oskit_size_t mem_size,
		     exec_sectype_t section_type)
{
	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	if (mem_addr < boot_kern_hdr.load_addr)
		boot_kern_hdr.load_addr = mem_addr;
	if (mem_addr+file_size > boot_kern_hdr.load_end_addr)
		boot_kern_hdr.load_end_addr = mem_addr+file_size;
	if (mem_addr+mem_size > boot_kern_hdr.bss_end_addr)
		boot_kern_hdr.bss_end_addr = mem_addr+mem_size;

	return 0;
}

static
int kimg_read_exec_2(void *handle, oskit_addr_t file_ofs, oskit_size_t file_size,
		     oskit_addr_t mem_addr, oskit_size_t mem_size,
		     exec_sectype_t section_type)
{
	oskit_size_t actual;

	if (!(section_type & EXEC_SECTYPE_ALLOC))
		return 0;

	assert(mem_size > 0);
	assert(mem_addr >= boot_kern_hdr.load_addr);
	assert(mem_addr+file_size <= boot_kern_hdr.load_end_addr);
	assert(mem_addr+mem_size <= boot_kern_hdr.bss_end_addr);

	kimg_read(handle, file_ofs,
		  (void*)boot_kern_image + mem_addr - boot_kern_hdr.load_addr,
		  file_size, &actual);

	assert(actual == file_size);

	return 0;
}

static void load_kernel(void)
{
	static char search[MULTIBOOT_SEARCH+sizeof(struct multiboot_header)];
	struct multiboot_header *h;
	oskit_size_t actual;
	int i, err;

	/* Scan for the multiboot_header.  */
	kimg_read(0, 0, search, sizeof(search), &actual);
	for (i = 0; ; i += 4)
	{
		if (i >= MULTIBOOT_SEARCH)
			panic("kernel image has no multiboot_header");
		h = (struct multiboot_header*)(search + i);
		if (h->magic == MULTIBOOT_MAGIC
		    && !(h->magic + h->flags + h->checksum))
			break;
	}
	if (h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO)
		panic("unknown multiboot_header flag bits %08x",
			h->flags & MULTIBOOT_MUSTKNOW & ~MULTIBOOT_MEMORY_INFO);
	boot_kern_hdr = *h;

	if (h->flags & MULTIBOOT_AOUT_KLUDGE) {

		/*
		 * Allocate memory and load the kernel into it.
		 * We do this before reserving the memory
		 * for the final kernel location,
		 * because the code in boot_start.c
		 * to copy the kernel to its final location
		 * can handle overlapping sources and destinations,
		 * and this way we may not need as much memory during bootup.
		 */
		boot_kern_image = mustmalloc(h->load_end_addr - h->load_addr);
		kimg_read(0, i + h->load_addr - h->header_addr, boot_kern_image,
			  h->load_end_addr - h->load_addr, &actual);
		assert(actual == h->load_end_addr - h->load_addr);

	} else {
		/*
		 * No a.out-kludge information available;
		 * attempt to interpret the exec header instead,
		 * using the simple interpreter in libexec.a.
		 * Perform the "load" in two passes.
		 * In the first pass,
		 * find the number of sections the load image contains
		 * and reserve the physical memory containing each section.
		 * Also, initialize the boot_kern_hdr
		 * to reflect the extent of the image.
		 * In the second pass, load the sections into a temporary area
		 * that can be copied to the final location
		 * all at once by do_boot.S.
		 */

		boot_kern_hdr.load_addr = 0xffffffff;
		boot_kern_hdr.load_end_addr = 0;
		boot_kern_hdr.bss_end_addr = 0;

		if ((err = exec_load(kimg_read, kimg_read_exec_1, 0,
				     &boot_kern_info)) != 0)
			panic("cannot load kernel image 1: error code %d", err);
		boot_kern_hdr.entry = boot_kern_info.entry;

		/*
		 * Allocate memory to load the kernel into.
		 * It's OK to malloc this
		 * before reserving the memory the kernel will occupy,
		 * because do_boot.S can deal with
		 * overlapping source and destination.
		 */
		assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
		assert(boot_kern_hdr.load_end_addr < boot_kern_hdr.bss_end_addr);
		boot_kern_image = mustmalloc(boot_kern_hdr.load_end_addr -
					     boot_kern_hdr.load_addr);

		if ((err = exec_load(kimg_read, kimg_read_exec_2, 0,
				     &boot_kern_info)) != 0)
			panic("cannot load kernel image 2: error code %d", err);
		assert(boot_kern_hdr.entry == boot_kern_info.entry);
	}

	/*
	 * Reserve the memory that the kernel will eventually occupy.
	 * All malloc calls after this are guaranteed
	 * to stay out of this region.
	 */
	lmm_remove_free(&malloc_lmm,
			(void *)phystokv(boot_kern_hdr.load_addr),
			phystokv(boot_kern_hdr.bss_end_addr)
			- phystokv(boot_kern_hdr.load_addr));

	printf("kernel at %08x-%08x text+data %d bss %d\n",
		boot_kern_hdr.load_addr, boot_kern_hdr.bss_end_addr,
		boot_kern_hdr.load_end_addr - boot_kern_hdr.load_addr,
		boot_kern_hdr.bss_end_addr - boot_kern_hdr.load_end_addr);
	assert(boot_kern_hdr.load_addr < boot_kern_hdr.load_end_addr);
	assert(boot_kern_hdr.load_end_addr < boot_kern_hdr.bss_end_addr);
	if (boot_kern_hdr.load_addr < 0x1000)
		panic("kernel wants to be loaded too low!");
#if 0
	if (boot_kern_hdr.bss_end_addr > phys_mem_max)
		panic("kernel wants to be loaded beyond available physical memory!");
#endif
	if ((boot_kern_hdr.load_addr < 0x100000)
	    && (boot_kern_hdr.bss_end_addr > 0xa0000))
		panic("kernel wants to be loaded on top of I/O space!");
}

static void init_boot_info(void)
{
	int i;

	/* Allocate memory for the boot_info structure and modules array.  */
	boot_info = (struct multiboot_info*)mustcalloc(sizeof(*boot_info), 1);
	for (i = 1; z->zmods[i].start; i++);
	boot_info->mods_count = i-1;
	boot_mods = (struct multiboot_module*)mustcalloc(
		boot_info->mods_count * sizeof(*boot_mods), 1);
	boot_info->mods_addr = kvtophys(boot_mods);

	/* Fill in the upper and lower memory size fields in the boot_info.  */
	boot_info->flags |= MULTIBOOT_MEMORY;
	{
		static struct trap_state ts;

		/* Find the top of lower memory (up to 640K).  */
		ts.trapno = 0x12;
		base_real_int(&ts);
		boot_info->mem_lower = ts.eax & 0xFFFF;

		/* Find the top of extended memory (up to 64MB).  */
		ts.trapno = 0x15;
		ts.eax = 0x8800;
		base_real_int(&ts);
		boot_info->mem_upper = ts.eax & 0xFFFF;
	}

	/* Build the kernel command line.  */
	if (*((unsigned short*)phystokv(0x90020)) == 0xA33F) /* CL_MAGIC */
	{
		void *src = (void*)phystokv(0x90000 +
			*((unsigned short*)phystokv(0x90022)));
		void *dest = mustcalloc(2048, 1);
		boot_info->cmdline = kvtophys(dest);
		memcpy(dest, src, 2048);
		boot_info->flags |= MULTIBOOT_CMDLINE;
	}

	/* Initialize each boot module entry.  */
	for (i = 0; i < boot_info->mods_count; i++)
	{
		struct zmod *zm = &z->zmods[1+i];
		void *modbuf;
		oskit_size_t size = zm->end - zm->start;

		/* Allocate memory to load the module into.  */
		modbuf = mustmalloc(size);
		boot_mods[i].mod_start = kvtophys(modbuf);
		boot_mods[i].mod_end = boot_mods[i].mod_start + size;

		/* Load it */
		zread(zm->start, modbuf, size);

		/* Also provide the string associated with the module.  */
		{
			char *oldstring = first_out->data + zm->cmdline;
			char *newstring = mustmalloc(strlen(oldstring)+1);
			strcpy(newstring, oldstring);
			boot_mods[i].string = kvtophys(newstring);
		}
	}
	if (i)
		boot_info->flags |= MULTIBOOT_MODS;
}

void main(int argc, char **argv)
{
	extern char edata[];

	/* Find the zipped boot module package.  */
	input_data = (char*)phystokv(DEF_SYSSEG*16 + ((oskit_addr_t)edata - 5*512));
	input_len = (DEF_INITSEG - DEF_SYSSEG)*16;

	ALLOC(uch, inbuf, INBUFSIZ);
	ALLOC(uch, outbuf, OUTBUFSIZ+OUTBUF_EXTRA);
	ALLOC(uch, window, WSIZE);

	exit_code = 0;
	test = 0;
	input_ptr = 0;
	part_nb = 0;

	clear_bufs();
	makecrc();

	method = get_method(0);

	work(0, 0);

	z = (struct zhdr*)first_out->data;
	if (z->magic != 0xf00baabb)
		panic("bad magic value in compressed boot module set");
	if (z->zmods[0].start == 0)
		panic("compressed boot module set contains no modules");
	if (first_out->size < z->zmods[0].start)
		panic("not enough data decompressed in first block");

	load_kernel();
	init_boot_info();
	boot_start(boot_info);
}

void idt_irq_init()
{
}

/* ========================================================================
 * Check the magic number of the input file and update ofname if an
 * original name was given and to_stdout is not set.
 * Return the compression method, -1 for error, -2 for warning.
 * Set inptr to the offset of the next byte to be processed.
 * This function may be called repeatedly for an input file consisting
 * of several contiguous gzip'ed members.
 * IN assertions: there is at least one remaining compressed member.
 *   If the member is a zip file, it must be the only one.
 */
local int get_method(in)
    int in;        /* input file descriptor */
{
    uch flags;
    char magic[2]; /* magic header */

    magic[0] = (char)get_byte();
    magic[1] = (char)get_byte();

    method = -1;                 /* unknown yet */
    part_nb++;                   /* number of parts in gzip file */
    last_member = 0;
    /* assume multiple members in gzip file except for record oriented I/O */

    if (memcmp(magic, GZIP_MAGIC, 2) == 0
        || memcmp(magic, OLD_GZIP_MAGIC, 2) == 0) {

	work = unzip;
	method = (int)get_byte();
	flags  = (uch)get_byte();
	if ((flags & ENCRYPTED) != 0)
	    error("Input is encrypted\n");
	if ((flags & CONTINUATION) != 0)
	       error("Multi part input\n");
	if ((flags & RESERVED) != 0) {
	    error("Input has invalid flags\n");
	    exit_code = ERROR;
	    if (force <= 1) return -1;
	}
	(ulg)get_byte();	/* Get timestamp */
	((ulg)get_byte()) << 8;
	((ulg)get_byte()) << 16;
	((ulg)get_byte()) << 24;

	(void)get_byte();  /* Ignore extra flags for the moment */
	(void)get_byte();  /* Ignore OS type for the moment */

	if ((flags & EXTRA_FIELD) != 0) {
	    unsigned len = (unsigned)get_byte();
	    len |= ((unsigned)get_byte())<<8;
	    while (len--) (void)get_byte();
	}

	/* Get original file name if it was truncated */
	if ((flags & ORIG_NAME) != 0) {
	    if (to_stdout || part_nb > 1) {
		/* Discard the old name */
		while (get_byte() != 0) /* null */ ;
	    } else {
	    } /* to_stdout */
	} /* orig_name */

	/* Discard file comment if any */
	if ((flags & COMMENT) != 0) {
	    while (get_byte() != 0) /* null */ ;
	}
    } else
	error("unknown compression method");
    return method;
}
