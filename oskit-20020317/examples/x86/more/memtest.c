/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Simple (in theory if not implementation) standalone memory testing program.
 * To reduce the size of this program it is linked with:
 *
 *	tiny_stack.S:		to allocate a 4K stack instead of 64K.
 *	direct_console.c:	to override the standard base_console stuff
 *				which includes 10k of GDB and COM console code.
 *
 * NOTE: I am told that, on the PC, bad accesses to memory don't generate
 * faults, writes are ignored and reads just return all ones.  Hence, I don't
 * have any exception handling code in here.  Hmm...should get rid of that
 * -1 test case then...
 *
 * NOTE2: This code has never actually detected a memory error.  At best
 * this says that I just haven't run it on any bad memory, at worst it says
 * that this program is as good as the BIOS memory test :-)
 *
 * NOTE3:  At the moment, this is hardwired to the real console, NOT the
 * serial console.  It's not really hanging on you...
 */
#include <stdio.h>
#include <stdlib.h>

#define DOBYTE	0
#define DOWORD	1
#define DOLONG	2
#define DOINCR	3

char *testtype[] = { "bytes", "words", "longwords", "increment" };

#define NEXTLOC	0	/* do all of memory with a single pattern */
#define NEXTPAT	1	/* do a single memory location with each pattern */

char *testmode[] = { "next-location", "next-pattern" };

/*
 * Various amusing bit patterns
 */
#define MAXPAT	16

unsigned char bpatterns[] = {
	0x55,	0xAA,	0xC3,	0x3C,	0xFF,	0x00,	0x99,	0x66
};
#define NBPAT	(sizeof(bpatterns)/sizeof(bpatterns[0]))

unsigned short wpatterns[] = {
	0x5555,	0xAAAA,	0xFFFF,	0x0000,	0xA443,	0x5BBC,	0x0FF0,	0xF00F
};
#define NWPAT	(sizeof(wpatterns)/sizeof(wpatterns[0]))

unsigned long lpatterns[] = {
	0x55555555,	0xAAAAAAAA,	0xFFFFFFFF,	0x00000000,
	0xA4420810,	0x5BBDF7EF,	0xF0F0F0F0,	0x78787878,
	0x3C3C3C3C,	0x1E1E1E1E,	0x0F0F0F0F
};
#define NLPAT	(sizeof(lpatterns)/sizeof(lpatterns[0]))

int npatterns = MAXPAT;

void *saddr, *eaddr;
unsigned long long stime, etime;

int domode = NEXTLOC;
int dotype = DOLONG;
int forever = 0;
int haltonerror = 0;
size_t totalmem = 0;
int faults = 0;

/*
 * x86 timing stuff
 */
#ifdef OSKIT
#include <oskit/x86/proc_reg.h>

#define TIMEVAL	unsigned long long

#define GETTIME(t) \
	t = get_tsc()

#define PRINTTIME(s, e, sz) { \
	unsigned long long d = (e) - (s); \
	d /= mhz; \
	printf(", %d.%06d sec (%d.%06d MB/sec)\n", \
		(int)(d / 1000000), (int)(d % 1000000), \
		(int)((unsigned long long)(sz) / d), \
		(int)((unsigned long long)(sz) % d)); \
}
#else
#define GETTIME(t)
#define PRINTTIME(s, e, sz)	printf("no time\n")
#endif

/*
 * XXX it would be useful if there was an OSKIT routine to determine the CPU
 * speed so we didn't have to specify it.
 */
#ifndef CYCLE_COUNT
#define CYCLE_COUNT 200
#endif
unsigned int mhz = CYCLE_COUNT;

size_t getmem(void);
void testmem(size_t size);
void parse_args(int argc, char **argv);
void testbyte(size_t size, int ix);
void testword(size_t size, int ix);
void testlong(size_t size, int ix);
void testincr(size_t size, int ix);
int testpatterns(size_t size);

#ifdef OSKIT
/*
 * XXX prompt user for "the command line"
 * This is really only needed for the BSD boot program which cannot pass
 * arbitrary flags through.  LILO and GRUB can I hear.
 */
#include <strings.h>
#include <oskit/clientos.h>
#define ctrl(c)	(c - 0x40)

int real_main(int argc, char *argv[]);

int
main()
{
	int argc, i;
	char *argv[16];
	char prompt[] = "args> ";
	char cmdline[128], c, *cl, **ap;

	oskit_clientos_init();

	printf("%s", prompt);
	for (i = 0; i < sizeof cmdline - 1; i++) {
		c = getchar();
		switch (c) {
		case '\b':
		case '\?':
			if (i > 0) {
				printf("\b \b");
				i--;
			}
			i--;
			continue;
		case ctrl('U'):
			while (i > 0) {
				printf("\b \b");
				i--;
			}
			i--;
			continue;
		case ctrl('R'):
			cmdline[i] = 0;
			printf("^R\n%s%s", prompt, cmdline);
			i--;
			continue;
		case '\n':
		case ctrl('M'):
			putchar('\n');
			break;
		default:
			cmdline[i] = c;
			putchar(c);
			continue;
		}
		break;
	}
	cmdline[i] = 0;
	cl = cmdline;
	argv[0] = "memtest";
	argc = 1;
	for (ap = &argv[1]; (*ap = strsep(&cl, " \t")) != 0; )
		if (**ap) {
			ap++;
			if (++argc == sizeof(argv)/sizeof(argv[0]))
				break;
		}
	return real_main(argc, argv);
}
#else
#define real_main main
#endif

int
real_main(int argc, char **argv)
{
	size_t size;

#ifdef OSKIT
	static void drain_lmm(void);
	drain_lmm();
#endif
	parse_args(argc, argv);
	size = getmem();
	printf("MEMTEST: CPU=%d MHz, testtype=%s, testmode=%s\n",
	       mhz, testtype[dotype], testmode[domode]);

	printf("Testing memory [%p - %p] (%d MB) %s\n",
	       saddr, saddr + size, (int)(size / (1024 * 1024)),
	       forever ? "forever" : "once");
	testmem(size);
	printf("Done: %d errors\n", faults);

	exit(0);
}

#ifdef OSKIT
#include <oskit/lmm.h>
#include <oskit/x86/pc/phys_lmm.h>
#include <oskit/debug.h>

/*
 * XXX there is no single point at which I can intercept memory allocations
 * by OSKit routines.  LMM routines are called directly on malloc_lmm in a
 * number of places in the OSKit libraries.
 *
 * So, we just drain the LMM at boot time and then override morecore.
 * This will force either a panic, or a call to morecore when an attempt
 * to allocate memory is made.
 */
static void
drain_lmm(void)
{
	extern oskit_addr_t phys_mem_max;
	extern lmm_t malloc_lmm;

	lmm_remove_free(&malloc_lmm, 0, phys_mem_max);
}

int
morecore(size_t size)
{
	dump_stack_trace();
	panic("morecore called");

	return 0;
}
#endif

size_t
getmem(void)
{
#ifdef OSKIT
	/*
	 * Assumes no dynamically allocated memory;
	 * i.e., if the program has grown past end[], you are screwed.
	 */
	extern char end[];
	extern oskit_addr_t phys_mem_max;

	saddr = (void *)end;
	eaddr = (void *)phys_mem_max;
	if (totalmem && saddr + totalmem < eaddr)
		eaddr = saddr + totalmem;
#else
	extern void *sbrk();
	size_t size, incr = 2;

	saddr = sbrk(0);

	if (totalmem) {
		eaddr = sbrk(totalmem);
		if (eaddr != (void *)(-1)) {
			eaddr = sbrk(0);
			goto done;
		}
	}
	while (incr > 1) {
		eaddr = sbrk(incr);
		if (eaddr == (void *)(-1))
			incr = incr / 2;
		else
			incr = incr * 2;
		eaddr = sbrk(0);
	}
done:
#endif

	return (size_t)(eaddr - saddr);
}

void
testmem(size_t size)
{
	int i;
	void (*rout)();
	size_t sz;

again:
	if (domode == NEXTPAT) {
		TIMEVAL st, et;

		sz = size >> 2;
		printf("%d longwords, 4 pattern tests ", sz);
		GETTIME(st);
		i = testpatterns(sz);
		GETTIME(et);
		if (i == 0)
			printf("[ OK ]");
		else
			printf("[ FAILED ]");
		PRINTTIME(st, et, size);
	} else {
		switch (dotype) {
		case DOBYTE:
			rout = testbyte;
			sz = size;
			break;
		case DOWORD:
			rout = testword;
			sz = size >> 1;
			break;
		case DOLONG:
			rout = testlong;
			sz = size >> 2;
			break;
		case DOINCR:
			rout = testincr;
			sz = size >> 2;
			break;
		default:
			rout = 0;
			sz = size;
			break;
		}
		if (rout)
			for (i = 0; i < npatterns; i++)
				(*rout)(sz, i);
	}
	if (forever && (!haltonerror || faults == 0)) {
#ifdef OSKIT
		/*
		 * Trygetchar is fraught with peril.
		 * See the man page.
		 */
		if (trygetchar() != 0x1b)
#endif
		goto again;
	}
}

void
testbyte(size_t size, int ix)
{
	volatile unsigned char *sp;
	unsigned char *ep = &((unsigned char *)saddr)[size];
	unsigned char pat = bpatterns[ix];
	TIMEVAL st, et;

	printf("%d bytes of 0x%08x ", size, pat);
	GETTIME(st);
	for (sp = saddr; sp < ep; sp++)
		*sp = pat;
	for (sp = saddr; sp < ep; sp++) {
		if (*sp != pat) {
			printf("\tMismatch at %p: 0x%08x != 0x%08x\n",
			       sp, *sp, pat);
			faults++;
		}
	}
	GETTIME(et);
	if (faults == 0)
		printf("[ OK ]");
	else
		printf("[ FAILED ]");
	PRINTTIME(st, et, size);
}

void
testword(size_t size, int ix)
{
	volatile unsigned short *sp;
	unsigned short *ep = &((unsigned short *)saddr)[size];
	unsigned short pat = wpatterns[ix];
	TIMEVAL st, et;

	printf("%d words of 0x%08x ", size, pat);
	GETTIME(st);
	for (sp = saddr; sp < ep; sp++)
		*sp = pat;
	for (sp = saddr; sp < ep; sp++) {
		if (*sp != pat) {
			printf("\tMismatch at %p: 0x%08x != 0x%08x\n",
			       sp, *sp, pat);
			faults++;
		}
	}
	GETTIME(et);
	if (faults == 0)
		printf("[ OK ]");
	else
		printf("[ FAILED ]");
	PRINTTIME(st, et, size << 1);
}

void
testlong(size_t size, int ix)
{
	volatile unsigned long *sp;
	unsigned long *ep = &((unsigned long *)saddr)[size];
	unsigned long pat = lpatterns[ix];
	TIMEVAL st, et;

	printf("%d longwords of 0x%08lx ", size, pat);
	GETTIME(st);
	for (sp = saddr; sp < ep; sp++)
		*sp = pat;
	for (sp = saddr; sp < ep; sp++) {
		if (*sp != pat) {
			printf("\tMismatch at %p: 0x%08lx != 0x%08lx\n",
			       sp, *sp, pat);
			faults++;
		}
	}
	GETTIME(et);
	if (faults == 0)
		printf("[ OK ]");
	else
		printf("[ FAILED ]");
	PRINTTIME(st, et, size << 2);
}

void
testincr(size_t size, int ix)
{
	volatile unsigned long *sp;
	unsigned long *ep = &((unsigned long *)saddr)[size];
	unsigned long pat;
	static long patval = 0;
	TIMEVAL st, et;

	pat = patval;
	printf("%d increasing longwords from 0x%08lx ", size, pat);
	GETTIME(st);
	for (sp = saddr; sp < ep; sp++) {
		*sp = pat;
		pat++;
	}
	pat = patval;
	for (sp = saddr; sp < ep; sp++) {
		if (*sp != pat) {
			printf("\tMismatch at %p: 0x%08lx != 0x%08lx\n",
			       sp, *sp, pat);
			faults++;
		}
		pat++;
	}
	GETTIME(et);
	if (faults == 0)
		printf("[ OK ]");
	else
		printf("[ FAILED ]");
	PRINTTIME(st, et, size << 2);
	patval = pat;
}

/*
 * For each memory location we write/read four patterns consecutively
 * as fast as possible.  Any caches need to be disabled for this to be
 * meaningful!
 *
 * XXX this code is written to be as efficient as possible when compiled
 * with GCC on the x86.
 */
int
testpatterns(size_t size)
{
	unsigned long pat1 = 0x55555555;
	unsigned long pat2 = 0xA4420810;
	volatile unsigned long *sp;
	unsigned long *ep = &((unsigned long *)saddr)[size];

	for (sp = saddr; sp < ep; sp++) {
		*sp = pat1;
		if (*sp != pat1)
			goto bad;
		pat1 = ~pat1;
		*sp = pat1;
		if (*sp != pat1)
			goto bad;
		*sp = pat2;
		if (*sp != pat2)
			goto bad;
		pat2 = ~pat2;
		*sp = pat2;
		if (*sp != pat2)
			goto bad;
	}
	return 0;
bad:
	return 1;
}


#define isdigit(c)	((c) >= '0' && (c) <= '9')

/*
 * Convert a char string into an integer.  Accepts
 * 'M', 'm', 'b', 'B', 'K' and k modifiers to mean
 * megabytes, bytes, and kilobytes, respectively.
 */
static size_t
parse_bytes(char *arg)
{
	size_t size = 0, numerator = 0, denom = 1;

	while (isdigit(*arg))
		size = (size * 10) + (*arg++ - '0');

	if (*arg == '.' && isdigit(*(arg+1))) {
		arg++;
		while (isdigit(*arg)) {
			numerator = (numerator * 10) + (*arg++ - '0');
			denom *= 10;
		}
	}
		
	switch (*arg) {
	case 'M':
	case 'm':
		size = (size * 1024 * 1024) + (numerator * 1024 * 1024 / denom);
		break;
	case 'K':
	case 'k':
		size = (size * 1024) + (numerator * 1024 / denom);
		break;
	}

	return size;
}

void
parse_args(int argc, char **argv)
{
	int i;

	i = 1;
	while (i < argc) {
		if (!strcmp(argv[i], "-forever")) {
			forever = 1;
		} else if (!strcmp(argv[i], "-haltonerror")) {
			haltonerror = 1;
		} else if (!strcmp(argv[i], "-mem") && i + 1 < argc) {
			i++;
			totalmem = parse_bytes(argv[i]);
		} else if (!strcmp(argv[i], "-npat") && i + 1 < argc) {
			i++;
			npatterns = atoi(argv[i]);
			if (npatterns < 1)
				npatterns = 1;
			else if (npatterns > MAXPAT)
				npatterns = MAXPAT;
		} else if (!strcmp(argv[i], "-mhz") && i + 1 < argc) {
			i++;
			mhz = atoi(argv[i]);
			if (mhz < 0 || mhz > 1000)
				panic("mhz(%u) out of range [0-1000]\n", mhz);
		} else if (!strcmp(argv[i], "-type") && i + 1 < argc) {
			i++;
			if(!strcmp(argv[i], "byte"))
				dotype = DOBYTE;
			else if(!strcmp(argv[i], "word"))
				dotype = DOWORD;
			else if(!strcmp(argv[i], "long"))
				dotype = DOLONG;
			else if(!strcmp(argv[i], "incr"))
				dotype = DOINCR;
			else
				panic("type(%s) unknown, must be: "
				      "byte, word, long, or incr\n", argv[i]);
		} else if (!strcmp(argv[i], "-mode") && i + 1 < argc) {
			i++;
			if(!strcmp(argv[i], "nextloc"))
				domode = NEXTLOC;
			else if(!strcmp(argv[i], "nextpat"))
				domode = NEXTPAT;
			else
				panic("mode(%s) unknown, must be: "
				      "nextloc or nextpat\n", argv[i]);
		}
		
		i++;
	}

	switch (dotype) {
	case DOBYTE: i = NBPAT; break;
	case DOWORD: i = NWPAT; break;
	case DOLONG: i = NLPAT; break;
	case DOINCR: i = NLPAT; break;
	}
	if (npatterns > i)
		npatterns = i;
}
