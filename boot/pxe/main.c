/*
 * Copyright (c) 2000, 2001 University of Utah and the Flux Group.
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <oskit/x86/pc/reset.h>
#include <oskit/x86/i16.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/pc/base_i16.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/error.h>
#include <oskit/exec/exec.h>
#include <oskit/lmm.h>
#include <oskit/machine/phys_lmm.h>
#include <oskit/diskpart/pcbios.h>
#include <oskit/boot/bootwhat.h>
#include <oskit/console.h>

#include "boot.h"
#include "pxe.h"
#include "decls.h"
#include "udp.h"

/*
 * Define to have the default behaviour for not having server bootinfo
 * be to run the "whoami" kernel instead of booting from the disk.
 *
 * We use this for initially setting up testbed nodes.
 */
#undef RUNWHOMAI

/*
 * Define to use BIOS extended disk read operation
 * (when available)
 */
#undef USEEDD

/*
 * BIOS drive number to read from
 *	0x80 == HDD 0
 */
#define BIOSDISK	0x80

/*
 * Our segment number
 * XXX always zero, just a constant for clarity
 */
#define OURSEG		0

/*
 * For now, hardwired.
 */
#define NETBOOT		"/tftpboot/netboot"

/*
 * Many retries in case network is congested.
 */
#define BOOTINFO_RETRY	20

/*
 * Defined in crt0.S
 */
extern unsigned int	pxenv_segment;		/* Segment of PXENV+ */
extern unsigned int	pxenv_offset;		/* Offset of PXENV+ */
extern SEGOFF16_t	pxe_entrypoint;		/* Entrypoint (16bit) of PXE */

/*
 * PXENV+ and !PXE pointers.
 */
static pxe_t		*pxep;
static pxenv_t		*pxenvp;

/*
 * Our DHCP information.
 */
struct in_addr		myip, serverip, gatewayip; 

/* Forward decls */
static int		boot_mbr_default(void);
static int		boot_mbr_sysid(int systemid);
static int		boot_mbr_partition(int partition);
static void		boot_system(void);
static void		boot_interactive(void);
void			delay(int);
int			biosdprobe(int);
int			biosdread(int, int, int, int, int, void *);
void			biosdreset(int);
static int		parse_multiboot_spec(char *,
				struct in_addr *, char **, char **);
extern char		version[], build_info[];

#ifdef USEEDD
int			bioseddread(int, int, int, void *);
#endif

int
main(int argc, char **argv)
{
	BOOTPLAYER		bootplayer;
	t_PXENV_GET_CACHED_INFO cacheinfo;

	printf("\n%s\n", version);
	printf("%s\n", build_info);

	/*
	 * Construct 32 bit addresses for these two.
	 */
	pxenvp = (pxenv_t *) ((pxenv_segment << 4) + pxenv_offset);
	pxep   = (pxe_t *) ((pxenvp->PXEPtr.segment << 4) +
			    pxenvp->PXEPtr.offset);	

	DPRINTF("PXENV+ at %p, !PXE at %p\n", pxenvp, pxep);

	/*
	 * Check that what we think is !PXE, really is.
	 */
	if (strncmp("!PXE", pxep->Signature, sizeof(pxep->Signature)))
		PANIC("Failed the \"!PXE\" test!");

	/*
	 * Suck out the 16bit PXE entrypoint and stash for the trampoline
	 * code to find for the intersegment call to the PXE.
	 */
	pxe_entrypoint = pxep->EntryPointSP;
	DPRINTF("PXE Entrypoint is %x:%x\n",
		pxep->EntryPointSP.segment, pxep->EntryPointSP.offset);

	/*
	 * Get the information we need from the PXE to find the boot
	 * server. This is the response to the last DHCP request, which
	 * provided the name of this program.
	 */
	cacheinfo.Status         = 0;
	cacheinfo.PacketType     = PXENV_PACKET_TYPE_BINL_REPLY;
	cacheinfo.BufferSize     = sizeof(bootplayer);
	cacheinfo.Buffer.segment = PXESEG(&bootplayer);
	cacheinfo.Buffer.offset  = PXEOFF(&bootplayer);

	pxe_invoke(PXENV_GET_CACHED_INFO, &cacheinfo);
	assert(!cacheinfo.Status);

	/* Stash */
	myip.s_addr       = ((struct in_addr *) &bootplayer.cip)->s_addr;
	serverip.s_addr   = ((struct in_addr *) &bootplayer.sip)->s_addr;
	gatewayip.s_addr  = ((struct in_addr *) &bootplayer.gip)->s_addr;

	/*
	 * Print some useful info.
	 */
	{
		char	cip[16], sip[16], gip[16];

		strcpy(cip, inet_ntoa(myip));
		strcpy(sip, inet_ntoa(serverip));
		strcpy(gip, inet_ntoa(gatewayip));

		printf("DHCP Info: IP: %s, Server: %s, Gateway: %s\n",
		       cip, sip, gip);
	}

	boot_system();

	/*
	 * We don't want to go through the i16 exit since that
	 * waits for a character on the console before reboot!
	 */
	pc_reset();
	return 0;
}

void
boot_system(void)
{
	boot_info_t		boot_info;
	boot_what_t	       *boot_whatp;
	int			success, i, j, err, actual, port;
	int			badpkts;
	struct in_addr		ip;

	/*
	 * Give the user a few seconds to interrupt and go into
	 * interactive mode. If we return, go through the auto
	 * boot and hope it works.
	 */
	printf("Type a key for interactive mode (quick, quick!)\n");
	for (i = 0; i < 1000; i++) {
		if (console_trygetchar() < 0)
			delay(10);
		else {
			boot_interactive();
			break;
		}
	}

	err = udp_open(myip.s_addr);
	if (err)
		goto mbr;

	/*
	 * Try a bunch of times.
	 */
	success = badpkts = 0;
	for (i = 0; i < BOOTINFO_RETRY; i++) {
		memset(&boot_info, 0, sizeof(boot_info));
		boot_info.opcode = BIOPCODE_BOOTWHAT_REQUEST;

		printf("Sending boot info request %d\n", i);

		err = udp_write(serverip.s_addr, gatewayip.s_addr,
				BOOTWHAT_SRCPORT, BOOTWHAT_DSTPORT,
				&boot_info, sizeof(boot_info));
		if (err)
			goto mbr;

		/*
		 * UDP read is nonblocking, so wait for a few seconds.
		 */
		for (j = 0; j < 100; j++) {
			err = udp_read(myip.s_addr, BOOTWHAT_SRCPORT,
				       &boot_info, sizeof(boot_info),
				       &actual, &port, &ip.s_addr);
			if (err) {
				delay(100);
				continue;
			}

			if (port != BOOTWHAT_DSTPORT) {
				badpkts++;
				continue;
			}

			if (boot_info.opcode == BIOPCODE_BOOTWHAT_REPLY) {
				if (boot_info.status == BISTAT_FAIL)
					printf("No bootinfo for us on server\n");
				else
					success = 1;
				goto gotit;
			}

			printf("Bad reply from server\n");
			delay(100);
		}
	}
	printf("No reply from server\n");
	if (badpkts > 100)
		printf("Got %d non-bootinfo packets\n", badpkts);
 gotit:
	err = udp_close();
	if (err)
		goto mbr;

	if (!success)
		goto mbr;

	/*
	 * Got a seemingly valid boot info packet that says what to do.
	 */
	boot_whatp = (boot_what_t *) &boot_info.data;
	err = 0;
	
	switch (boot_whatp->type) {
	case BIBOOTWHAT_TYPE_MB:
		err = load_multiboot_image(&boot_whatp->what.mb.tftp_ip,
					   boot_whatp->what.mb.filename,
					   boot_whatp->cmdline);
		break;
		
	case BIBOOTWHAT_TYPE_PART:
		err = boot_mbr_partition(boot_whatp->what.partition);
		break;

	case BIBOOTWHAT_TYPE_SYSID:
		err = boot_mbr_sysid(boot_whatp->what.sysid);

	default:
		PANIC("Invalid type: %d\n", boot_whatp->type);
		break;
	}

	/*
	 * Well, the only way to get here is if the specified boot failed.
	 * Fall through to booting the default, and hope it works.
	 */
 mbr:
#ifdef RUNWHOAMI
	{
		struct in_addr ip;
			
		inet_aton("155.101.128.70", &ip);
		load_multiboot_image(&ip, "/tftpboot/whoami", "");
	}
#else
	boot_mbr_default();
#endif
}

/*
 * Assert support that uses way less space
 */
void
local_panic(void)
{
	extern void dump_stack_trace();

	printf("ASSERT FAILED!\n");
	dump_stack_trace();
	boot_mbr_default();
}

static int pxe_isshutdown;

/*
 * Get ready to invoke the kernel. Must clean up all the PXE goo.
 */
void
pxe_cleanup(void)
{
	t_PXENV_UNDI_SHUTDOWN	undi_shutdown;
	t_PXENV_UNLOAD_STACK	unload_stack;

	memset(&undi_shutdown, 0, sizeof(undi_shutdown));
	memset(&unload_stack, 0, sizeof(unload_stack));

	pxe_invoke(PXENV_UNDI_SHUTDOWN, &undi_shutdown);
	assert(! undi_shutdown.Status);
	
	pxe_invoke(PXENV_UNLOAD_STACK, &unload_stack);
	assert(! unload_stack.Status);

	pxe_isshutdown = 1;
}
	
/*
 * This is the first part of a trampoline into the PXE code.
 * Put the processor into 16bit/real mode, and then call the
 * little assembly stub that will invoke the PXE like this:
 *
 *   PXE(our data segment, operation code, argument/return block pointer);
 */
void
pxe_invoke(int routine, void *data)
{
	extern  long pxe_routine;
	extern  long pxe_databucket;
	extern  int  i16_pxe_invoke(void);

	pxe_routine    = routine;
	pxe_databucket = (long) data;
	
	do_16bit(KERNEL_CS, KERNEL_16_CS,
		base_i16_switch_to_real_mode();
		i16_pxe_invoke();
		base_i16_switch_to_pmode();
	);
}

/*
 * We need a trapstate to invoke bios calls.
 */
static struct trap_state ts;

/*
 * Load the Master Boot Record into the magic location (which happens
 * to be _start) using the BIOS readdisk routine.
 */
static void
load_mbr(void)
{
	extern char	_start[];
	int		*foo = (int *) _start;
	int		rv = 0, retry;
	int		doesedd;

	DPRINTF("Calling pxe_cleanup ...\n");
	pxe_cleanup();

	DPRINTF("Calling biosdread to overwrite _start ...\n");

	doesedd = biosdprobe(BIOSDISK);

	memset(foo, 0, 512);
	for (retry = 0; retry < 3; retry++) {
#ifdef USEEDD
		if (doesedd)
			rv = bioseddread(BIOSDISK, 0, 1, foo);
		else
#endif
		rv = biosdread(BIOSDISK, 0, 0, 1, 1, foo);
		if (rv == 0)
			break;
		biosdreset(BIOSDISK);
	}
	if (rv != 0)
		printf("read of MBR failed, err=%x\n", rv);
}

/*
 * Check sanity of the MBR.
 */
static int
check_mbr(void)
{
	extern char		   _start[];
	bios_label_t		   *label;
	struct bios_partition_info *entry;
	int			   i;

	label = (bios_label_t *) (_start + BIOS_LABEL_BYTE_OFFSET);
	if (label->magic != BIOS_LABEL_MAGIC)
		goto verybad;

	entry = (struct bios_partition_info *)
		(_start + BIOS_LABEL_BYTE_OFFSET);

	for (i = 0; i < 4; i++, entry++) {
		if (!entry->n_sectors)
			continue;
		if (entry->bootid == BIOS_BOOTABLE)
			return 1;
	}
 verybad:
	printf("MBR IS BAD! Rebooting ...\n");
	pc_reset();
	return 0;
}

/*
 * After cleaning up the PXE, switch to 16bit/real mode, and then jump
 * into the MBR.  As long as there is a MBR, and its intact, something
 * good should happen (say, Lilo will appear).
 */
static void
boot_mbr(void)
{
	extern void	i16_boot_mbr(void);

	check_mbr();

	DPRINTF("Jumping to MBR ...\n");
	do_16bit(KERNEL_CS, KERNEL_16_CS,
		base_i16_switch_to_real_mode();
		i16_boot_mbr();
		base_i16_switch_to_pmode();
	);

	/*
	 * Never returns of course!
	 */
}

/*
 * Simply load and boot the MBR right off the disk.
 */
static int
boot_mbr_default(void)
{
	printf("Loading MBR and booting active partition\n");

	load_mbr();
	boot_mbr();
	return 0;
}

/*
 * Boot a particular sysid. Search the table for that partition and
 * make it the active one, then boot. If the type does not exist,
 * fallback to booting the default partition.
 *
 * N.B. we activate the first (lowest numbered) partition of a particular type.
 */
int
boot_mbr_sysid(int sysid)
{
	extern char			_start[];
	int				i, found;
	struct bios_partition_info	*entry;

	printf("Loading MBR and booting system ID %d\n", sysid);

	load_mbr();

	/*
	 * Look for a non-empty partition that matches the one we want.
	 */
	entry = (struct bios_partition_info *)
		(_start + BIOS_LABEL_BYTE_OFFSET);
		
	for (i = 0; i < 4; i++, entry++) {
		if (!entry->n_sectors)
			continue;
		if (entry->systid == sysid)
			break;
	}

	/*
	 * Either we found it or it does not exist! If we found it,
	 * then make it the bootable partition.
	 */
	if (i < 4) {
		entry = (struct bios_partition_info *)
			(_start + BIOS_LABEL_BYTE_OFFSET);
			
		found = 0;
		for (i = 0; i < 4; i++, entry++) {
			if (!entry->n_sectors)
				continue;
			if (!found && entry->systid == sysid) {
				entry->bootid = BIOS_BOOTABLE;
				found = 1;
			} else
				entry->bootid = 0;
		}
	}
	else {
		printf("%d: sysid not found in any partition\n", sysid);
		return 1;
	}

	boot_mbr();
	return 0;
}

/*
 * Boot a particular partition by making it bootable. Better be non-zero.
 */ 
int
boot_mbr_partition(int partition)
{
	extern char			_start[];
	int				i;
	struct bios_partition_info	*table;

	printf("Loading MBR and booting system partition %d\n", partition);
	if (partition < 1 || partition > 4) {
		printf("%d: invalid partition number\n", partition);
		return 1;
	}

	load_mbr();

	table = (struct bios_partition_info *)
		(_start + BIOS_LABEL_BYTE_OFFSET);

	/* partition is 1-based, table is 0-based */
	partition--;

	if (! table[partition].n_sectors) {
		printf("%d: non-existent partition\n", partition);
		return 1;
	}

	for (i = 0; i < 4; i++) {
		if (i == partition)
			table[i].bootid = BIOS_BOOTABLE;
		else
			table[i].bootid = 0;
	}

	boot_mbr();
	return 0;
}

/*
 * Allow for an interactive boot specification. Returning implies that
 * we go through the automated boot.
 */
static void
boot_interactive(void)
{
#define PROMPT	"Enter boot spec: "
	char	buf[512];
	
	while (1) {
		/*
		 * Flush!
		 */
		while (console_trygetchar() >= 0)
			;

		console_putbytes(PROMPT, strlen(PROMPT));
		gets(buf);

#define PART	"part:"
		if (strncmp(buf, PART, strlen(PART)) == 0) {
			int part = atoi(&buf[strlen(PART)]);

			boot_mbr_partition(part);			
		}
#define SYSID	"sysid:"
		else if (strncmp(buf, SYSID, strlen(SYSID)) == 0) {
			int sysid = atoi(&buf[strlen(SYSID)]);

			boot_mbr_sysid(sysid);
		}
#define AUTO	"auto"
		else if (strncmp(buf, AUTO, strlen(AUTO)) == 0) {
			if (!pxe_isshutdown)
				return;
		}
#define REBOOT	"reboot"
		else if (strncmp(buf, REBOOT, strlen(REBOOT)) == 0) {
			printf("Rebooting ...\n");
			pc_reset();
			return;
		}
		else if (isalpha(buf[0]) || isdigit(buf[0]) || buf[0] == '/') {
			struct		in_addr	ip;
			char		*path, *args;
			
			if (!pxe_isshutdown) {
				parse_multiboot_spec(buf, &ip, &path, &args);
				load_multiboot_image(&ip, path, args);
			}
		}

		if (pxe_isshutdown)
			printf("PXE shutdown, must reboot to use network\n");
		printf("Choices are: \n"
		       "  part:<partition number>, or\n"
		       "  sysid:<system ID>, or\n"
		       "  <pathway to multiboot image>, or\n"
		       "  auto (query the bootinfo server again), or\n"
		       "  reboot\n");
	}
}

int
biosdprobe(int device)
{
	ts.trapno	= 0x13;
	ts.eax		= 0x800;
	ts.edx		= device;

	base_real_int(&ts);

#ifdef USEEDD
	/* check for EDD */
	ts.eax		= 0x4100;
	ts.edx		= device;
	ts.ebx		= 0x55aa;

	base_real_int(&ts);
	if ((ts.eflags & 1) == 0 && (ts.ebx & 0xFFFF) == 0xaa55 &&
	    (ts.ecx & 1) != 0)
		return 1;
#endif

	return 0;
}

void
biosdreset(int device)
{
	ts.trapno	= 0x13;
	ts.eax		= device >= 0x80 ? 0x0D00 : 0x0000;
	ts.edx		= device;

	DPRINTF("dreset\n");
	base_real_int(&ts);
	DPRINTF("returns stat=%x\n", (ts.eax >> 8) & 0xFF);
}

/*
 * biosdread(dev, cyl, head, sec, nsec, offset)
 *	Read "nsec" sectors from disk to offset "offset" in boot segment
 * BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
 *	Call with	%ah = 0x2
 *			%al = number of sectors
 *			%ch = cylinder
 *			%cl = sector
 *			%dh = head
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *			%es:%bx = segment:offset of buffer
 *	Return:		
 *			%ah = 0x0 on success; err code on failure
 */
int
biosdread(int device, int cyl, int head, int sector, int count, void *addr)
{
	ts.trapno	= 0x13;
	ts.eax		= (0x2 << 8)  | count;
	ts.ecx		= (cyl << 8)  | sector;
	ts.edx		= (head << 8) | device;
	ts.ebx		= ((oskit_u32_t)addr) & 0xFFFF;
	ts.v86_es	= OURSEG;

	DPRINTF("dread to %x:%x\n", ts.v86_es, ts.ebx);
	base_real_int(&ts);
	DPRINTF("returns cf=%x err=%x\n", ts.eflags&1, (ts.eax>>8) & 0xFF);
	return (ts.eflags & 0x1) ? ((ts.eax >> 8) & 0xFF) : 0;
}

#ifdef USEEDD
/*
 * Use extended device read.
 */
int
bioseddread(int device, int dblk, int count, void *addr)
{
	static unsigned short packet[8];

	packet[0] = 0x10;
	packet[1] = count;
	packet[2] = ((oskit_u32_t)addr) & 0xFFFF;
	packet[3] = OURSEG;
	packet[4] = dblk & 0xFFFF;
	packet[5] = (dblk >> 16) & 0xFFFF;
	packet[6] = 0;
	packet[7] = 0;

	ts.trapno = 0x13;
	ts.eax = 0x4200;
	ts.edx = device;
	ts.ds = OURSEG;
	ts.esi = (oskit_u32_t)packet;

	DPRINTF("EDD dread to %x:%x\n", packet[3], packet[2]);
	base_real_int(&ts);
	DPRINTF("returns cf=%x err=%x\n", ts.eflags&1, (ts.eax>>8) & 0xFF);
	return (ts.eflags & 0x1) ? ((ts.eax >> 8) & 0xFF) : 0;
}
#endif

/*
 * Kind of a hack.
 */
void
delay(int millis)
{
	
	long long count = millis * 200000;

	while (count)
		count--;
}

/*
 * Take a string and separate the IP, path, and arguments into strings.
 */
static int
parse_multiboot_spec(char *cmdline,
		     struct in_addr *ip, char **path, char **args)
{
	char	*p;

	if (isdigit(*cmdline) && (p = strchr(cmdline, ':'))) {
		*p++ = '\0';
		inet_aton(cmdline, ip);
	}
	else {
		ip->s_addr = 0;
		p = cmdline;
	}

	*path = p;
	
	if ((p = strchr(p, ' '))) {
		*p++ = '\0';
		*args = p;
	}
	else
		*args = "";

	return 0;
}
