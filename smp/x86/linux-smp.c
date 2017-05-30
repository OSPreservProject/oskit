/*
 * Copyright (c) 1996-1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit SMP Support Library, which is free software,
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
 *	Intel MP v1.1/v1.4 specification support routines for multi-pentium
 *	hosts.
 *
 *	(c) 1995 Alan Cox, CymruNET Ltd  <alan@cymru.net>
 *	Supported by Caldera http://www.caldera.com.
 *	Much of the core SMP work is based on previous work by Thomas Radke, to
 *	whom a great many thanks are extended.
 *
 *	Thanks to Intel for making available several different Pentium and
 *	Pentium Pro MP machines.
 *
 *	This code is released under the GNU public license version 2 or
 *	later.
 *
 *	Fixes
 *		Felix Koop	:	NR_CPUS used properly
 *		Jose Renau	:	Handle single CPU case.
 *		Alan Cox	:	By repeated request 8) - Total BogoMIP report.
 *		Greg Wright	:	Fix for kernel stacks panic.
 *		Erich Boleyn	:	MP v1.4 and additional changes.
 */

#include <oskit/config.h>
#include <oskit/smp.h>
#include <oskit/x86/smp.h>

#include <oskit/gdb.h>

/*static*/ volatile unsigned int num_processors = 1;			/* Internal processor count				*/

#ifdef HAVE_CODE16 /* SMP startup requires 16-bit assembly code support */

#include "linux-smp.h"
#include "bitops.h"


#include <stdio.h>
#define printk printf

#include <string.h>


unsigned long apic_addr=0xFEE00000;                     /* Address of APIC (defaults to 0xFEE00000)             */


#include <oskit/lmm.h>
#include <malloc.h>
#include <assert.h>

#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_cpu.h>
#include <oskit/x86/base_paging.h>
#include <oskit/x86/pc/phys_lmm.h>


#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_idt.h>

/* These will be in the code.  Modify then copy to trampoline page */
extern unsigned short smp_boot_gdt_limit;
extern unsigned long smp_boot_gdt_linear_base;

extern unsigned short smp_boot_idt_limit;
extern unsigned long smp_boot_idt_linear_base;

/* These two mark the beginning and end of the trampoline code */
extern int _SMP_TRAMP_START_;
extern int _SMP_TRAMP_END_;





/* XXX: use iodelay */
static int QQ;

#define udelay(x) for (QQ=0;QQ<100*x;QQ++)

/*
 *	Why isn't this somewhere standard ??
 */

extern __inline int max(int a,int b)
{
	if(a>b)
		return a;
	return b;
}


int smp_found_config=0;					/* Have we found an SMP box 				*/

unsigned long cpu_present_map = 0;			/* Bitmask of existing CPU's 				*/
int smp_num_cpus = 1;					/* Total count of live CPU's 				*/
int smp_threads_ready=0;				/* Set when the idlers are all forked 			*/
volatile int cpu_number_map[NR_CPUS];			/* which CPU maps to which logical number		*/
volatile int cpu_logical_map[NR_CPUS];			/* which logical number maps to which CPU		*/
volatile unsigned long cpu_callin_map[NR_CPUS] = {0,};	/* We always use 0 the rest is ready for parallel delivery */
struct cpuinfo_x86 cpu_data[NR_CPUS];			/* Per cpu bogomips and other parameters 		*/

/* static */ unsigned long io_apic_addr = 0xFEC00000;		/* Address of the I/O apic (not yet used) 		*/

unsigned char boot_cpu_id = 0;				/* Processor that is doing the boot up 			*/
static int smp_activated = 0;				/* Tripped once we need to start cross invalidating 	*/
int apic_version[NR_CPUS];				/* APIC version number					*/
unsigned long apic_retval;				/* Just debugging the assembler.. 			*/
unsigned char *kernel_stacks[NR_CPUS];			/* Kernel stack pointers for CPU's (debugging)		*/

#if 0
/* used in Linux message passing code */
static volatile unsigned char smp_cpu_in_msg[NR_CPUS];	/* True if this processor is sending an IPI		*/
static volatile unsigned long smp_msg_data;		/* IPI data pointer					*/
static volatile int smp_src_cpu;			/* IPI sender processor					*/
static volatile int smp_msg_id;				/* Message being sent					*/
#endif


volatile unsigned long kernel_flag=0;			/* Kernel spinlock 					*/


volatile unsigned char active_kernel_processor = NO_PROC_ID;	/* Processor holding kernel spinlock		*/
volatile unsigned long kernel_counter=0;		/* Number of times the processor holds the lock		*/
volatile unsigned long syscall_count=0;			/* Number of times the processor holds the syscall lock	*/

volatile unsigned long  smp_proc_in_lock[NR_CPUS] = {0,};/* for computing process time */
volatile unsigned long smp_process_available=0;


#ifdef SMP_DEBUG
#define SMP_PRINTK(x)	printf x
#else
#define SMP_PRINTK(x)
#endif


/*
 *      Checksum an MP configuration block.
 */

static int mpf_checksum(unsigned char *mp, int len)
{
        int sum=0;
        while(len--)
                sum+=*mp++;
        return sum&0xFF;
}

/*
 *      Processor encoding in an MP configuration block
 */

static char *mpc_family(int family,int model)
{
        static char n[32];
        static char *model_defs[]=
        {
                "80486DX","80486DX",
                "80486SX","80486DX/2 or 80487",
                "80486SL","Intel5X2(tm)",
                "Unknown","Unknown",
                "80486DX/4"
        };
        if(family==0x6)
                return("Pentium(tm) Pro");
        if(family==0x5)
                return("Pentium(tm)");
        if(family==0x0F && model==0x0F)
                return("Special controller");
        if(family==0x04 && model<9)
                return model_defs[model];
        sprintf(n,"Unknown CPU [%d:%d]",family, model);

	return n;
}

/*
 *	Read the MPC
 */

static int smp_read_mpc(struct mp_config_table *mpc)
{
	char str[16];
	int count=sizeof(*mpc);
	int apics=0;
	unsigned char *mpt=((unsigned char *)mpc)+count;

	SMP_PRINTK(("MPC table being read at 0x%x.\n", mpc));

	if(memcmp(mpc->mpc_signature,MPC_SIGNATURE,4)) {
		printk("Bad signature [%c%c%c%c].\n",
			mpc->mpc_signature[0],
			mpc->mpc_signature[1],
			mpc->mpc_signature[2],
			mpc->mpc_signature[3]);
		return 1;
	}
	if(mpf_checksum((unsigned char *)mpc,mpc->mpc_length)) {
		printk("Checksum error.\n");
		return 1;
	}
	if(mpc->mpc_spec!=0x01 && mpc->mpc_spec!=0x04) {
		printk("Bad Config Table version (%d)!!\n",mpc->mpc_spec);
		return 1;
	}
	memcpy(str,mpc->mpc_oem,8);
	str[8]=0;
	printk("OEM ID: %s ",str);
	memcpy(str,mpc->mpc_productid,12);
	str[12]=0;
	printk("Product ID: %s ",str);
	printk("APIC at: 0x%lX\n",mpc->mpc_lapic);

	/* set the local APIC address */
	apic_addr = mpc->mpc_lapic;

	/*
	 *	Now process the configuration blocks.
	 */

	while(count<mpc->mpc_length) {
		switch(*mpt) {
			case MP_PROCESSOR: {
				struct mpc_config_processor *m=
					(struct mpc_config_processor *)mpt;
				if(m->mpc_cpuflag&CPU_ENABLED) {
					printk("Processor #%d %s APIC version %d\n",
						m->mpc_apicid,
						mpc_family((m->mpc_cpufeature&
							CPU_FAMILY_MASK)>>8,
							(m->mpc_cpufeature&
								CPU_MODEL_MASK)>>4),
						m->mpc_apicver);
#ifdef SMP_DEBUG
					if(m->mpc_featureflag&(1<<0))
						printk("    Floating point unit present.\n");
					if(m->mpc_featureflag&(1<<7))
						printk("    Machine Exception supported.\n");
					if(m->mpc_featureflag&(1<<8))
						printk("    64 bit compare & exchange supported.\n");
					if(m->mpc_featureflag&(1<<9))
						printk("    Internal APIC present.\n");
#endif

					if(m->mpc_cpuflag&CPU_BOOTPROCESSOR) {
						printk("    Bootup CPU\n");
						boot_cpu_id=m->mpc_apicid;
					} else {	/* Boot CPU already counted */
						num_processors++;

					}
					if(m->mpc_apicid>NR_CPUS)
						printk("Processor #%d unused. (Max %d processors).\n",m->mpc_apicid, NR_CPUS);
					else {
						cpu_present_map|=(1<<m->mpc_apicid);
						apic_version[m->mpc_apicid]=m->mpc_apicver;
					}

				}
				mpt+=sizeof(*m);
				count+=sizeof(*m);
				break;
			}
			case MP_BUS: {
				struct mpc_config_bus *m=
					(struct mpc_config_bus *)mpt;
				memcpy(str,m->mpc_bustype,6);
				str[6]=0;
				printk("Bus #%d is %s\n", m->mpc_busid, str);
				mpt+=sizeof(*m);
				count+=sizeof(*m);
				break;
			}
			case MP_IOAPIC: {
				struct mpc_config_ioapic *m=
					(struct mpc_config_ioapic *)mpt;
				if(m->mpc_flags&MPC_APIC_USABLE) {
					apics++;
	                                printk("I/O APIC #%d Version %d at 0x%lX.\n",
	                                	m->mpc_apicid,m->mpc_apicver,
	                                	m->mpc_apicaddr);

	                                io_apic_addr = m->mpc_apicaddr;
	                        }
                                mpt+=sizeof(*m);
                                count+=sizeof(*m);
                                break;
			}
			case MP_INTSRC:
			{
				struct mpc_config_intsrc *m=
					(struct mpc_config_intsrc *)mpt;

				mpt+=sizeof(*m);
				count+=sizeof(*m);
				break;
			}
			case MP_LINTSRC:
			{
				struct mpc_config_intlocal *m=
					(struct mpc_config_intlocal *)mpt;
				mpt+=sizeof(*m);
				count+=sizeof(*m);
				break;
			}
		}
	}
	if(apics>1)
		printk("Warning: Multiple APIC's not supported.\n");

	SMP_PRINTK(("MPC table read completed\n"));
	return num_processors;
}

/*
 *	Scan the memory blocks for an SMP configuration block.
 */

int smp_scan_config(unsigned long base, unsigned long length)
{
	unsigned long *bp=(unsigned long *)phystokv((unsigned long *)base);
	struct intel_mp_floating *mpf;

	SMP_PRINTK(("Scan SMP from %d k for %ld bytes.\n",
		(int)base/1024, length));

	if(sizeof(*mpf)!=16)
		printk("Error: MPF size\n");

	while(length>0) {
		if(*bp==SMP_MAGIC_IDENT) {
			mpf=(struct intel_mp_floating *)bp;
			if(mpf->mpf_length==1 &&
				!mpf_checksum((unsigned char *)bp,16) &&
				(mpf->mpf_specification == 1
				 || mpf->mpf_specification == 4) )
			{
				printk("Intel MultiProcessor Specification v1.%d\n", mpf->mpf_specification);
				if(mpf->mpf_feature2&(1<<7))
					printk("    IMCR and PIC compatibility mode.\n");
				else
					printk("    Virtual Wire compatibility mode.\n");
				smp_found_config=1;
				/*
				 *	Now see if we need to read further.
				 */
				if(mpf->mpf_feature1!=0) {
					/*
					 *	We need to know what the local
					 *	APIC id of the boot CPU is!
					 */

					boot_cpu_id = GET_APIC_ID(*((volatile unsigned long *) APIC_ID));

					/*
					 *	2 CPUs, numbered 0 & 1.
					 */
					cpu_present_map=3;
					num_processors=2;
					printk("I/O APIC at 0xFEC00000.\n");
					printk("Bus#0 is ");
				}
				switch(mpf->mpf_feature1) {
					case 1:
					case 5:
						printk("ISA\n");
						break;
					case 2:
						printk("EISA with no IRQ8 chaining\n");
						break;
					case 6:
					case 3:
						printk("EISA\n");
						break;
					case 4:
					case 7:
						printk("MCA\n");
						break;
					case 0:
						break;
					default:
						printk("???\nUnknown standard configuration %d\n",
							mpf->mpf_feature1);
						return 1;
				}
				if(mpf->mpf_feature1>4) {
					printk("Bus #1 is PCI\n");

					/*
					 *	Set local APIC version to
					 *	the integrated form.
					 *	It's initialized to zero
					 *	otherwise, representing
					 *	a discrete 82489DX.
					 */
					apic_version[0] = 0x10;
					apic_version[1] = 0x10;
				}
				/*
				 *	Read the physical hardware table.
				 *	Anything here will override the
				 *	defaults.
				 */
				if(mpf->mpf_physptr)
					smp_read_mpc((void *)mpf->mpf_physptr);
				else
					printf("No MPC table found.\n");

				/*
				 *	Now that the boot CPU id is known,
				 *	set some other information about it.
				 */
				cpu_logical_map[0] = boot_cpu_id;

				/*
				 *	Only use the first configuration found.
				 */
				SMP_PRINTK(("Returning from SCAN_CONFIG w/found config\n"));
				return 1;
			}
		}
		bp+=4;
		length-=16;
	}

	return 0;
}


/*
 *	Trampoline 80x86 program executed on AP startup in real mode.
 *	It gets the processor into protected mode.
 */

/*
 *	Currently trivial. Write the real->protected mode
 *	bootstrap into the page concerned. The caller
 *	has made sure it's suitably aligned.
 */

static void install_trampoline(unsigned char *mp)
{
	memcpy(mp,&_SMP_TRAMP_START_,4096);
}


/*
 *	Architecture specific routine called by the kernel just before init is
 *	fired off. This allows the BP to have everything in order [we hope].
 *	At the end of this all the AP's will hit the system scheduling and off
 *	we go. Each AP will load the system gdt's and jump through the kernel
 *	init into idle(). At this point the scheduler will one day take over
 * 	and give them jobs to do. smp_callin is a standard routine
 *	we use to track CPU's as they power up.
 */

void smp_callin(void)
{
	/* these CAN'T be static vars... */
	int cpuid=GET_APIC_ID(apic_read(APIC_ID));
	unsigned long l;

	/*
	 *	Activate our APIC
	 */

	SMP_PRINTK(("CALLIN %d\n",smp_processor_id()));
 	l=apic_read(APIC_SPIV);
 	l|=(1<<8);		/* Enable */
 	apic_write(APIC_SPIV,l);
 	sti();

	/*
	 *	Allow the master to continue.
	 */
	/* ATOMIC bit set */
	set_bit(cpuid, (unsigned long *)&cpu_callin_map[0]);
	/*
	 *	until we are ready for SMP scheduling
	 */
#if 0
	local_flush_tlb();
#endif
	SMP_PRINTK(("Commenced...\n"));

	/* this will return to assembly, which will wait to be started. */
}



/*
 *	Cycle through the processors sending pentium IPI's to boot each.
 */

int smp_boot_cpus(void)
{
	int i;
	int cpucount=0;
	unsigned long cfg;
	void *stack;

	struct pseudo_descriptor pdesc;
        pdesc.limit = sizeof(base_gdt) - 1;
        pdesc.linear_base = kvtolin(&base_gdt);
#if 0
	assert(kvtolin(&base_gdt) == (long)&base_gdt);
#endif
	assert(kvtophys(&base_gdt) == (long)&base_gdt);

	SMP_PRINTK(("in SMP_BOOT_CPUs\n"));

	smp_boot_gdt_limit = sizeof(base_gdt) - 1;
	smp_boot_gdt_linear_base = (long)&base_gdt;

	smp_boot_idt_limit = sizeof(base_idt) - 1;
	smp_boot_idt_linear_base = (long)&base_idt;

	SMP_PRINTK(("%p %d: _SMP_TRAMP_START_\n", &_SMP_TRAMP_START_,
		(int)&_SMP_TRAMP_START_));

	SMP_PRINTK(("%p %d: _SMP_TRAMP_END_\n", &_SMP_TRAMP_END_,
		(int)&_SMP_TRAMP_END_));

	/*
	 *	Initialize the logical to physical cpu number mapping
	 */

	for (i = 0; i < NR_CPUS; i++)
		cpu_number_map[i] = -1;

	/*
	 *	Setup boot CPU information
	 */

	cpu_present_map |= (1 << smp_processor_id());
	cpu_number_map[boot_cpu_id] = 0;
	active_kernel_processor=boot_cpu_id;

	/*
	 *	If we don't conform to the Intel MPS standard, get out
	 *	of here now!
	 */

	if (!smp_found_config)
		return E_SMP_NO_CONFIG;

#ifdef SMP_DEBUG
	{
		int reg;

		/*
		 *	This is to verify that we're looking at
		 *	a real local APIC.  Check these against
		 *	your board if the CPUs aren't getting
		 *	started for no apparent reason.
		 */

		reg = apic_read(APIC_VERSION);
		SMP_PRINTK(("Getting VERSION: %x\n", reg));

		apic_write(APIC_VERSION, 0);
		reg = apic_read(APIC_VERSION);
		SMP_PRINTK(("Getting VERSION: %x\n", reg));

		/*
		 *	The two version reads above should print the same
		 *	NON-ZERO!!! numbers.  If the second one is zero,
		 *	there is a problem with the APIC write/read
		 *	definitions.
		 *
		 *	The next two are just to see if we have sane values.
		 *	They're only really relevant if we're in Virtual Wire
		 *	compatibility mode, but most boxes are anymore.
		 */


		reg = apic_read(APIC_LVT0);
		SMP_PRINTK(("Getting LVT0: %x\n", reg));

		reg = apic_read(APIC_LVT1);
		SMP_PRINTK(("Getting LVT1: %x\n", reg));
	}
#endif

	/*
	 *	Enable the local APIC
	 */

	cfg=apic_read(APIC_SPIV);
	cfg|=(1<<8);		/* Enable APIC */
	apic_write(APIC_SPIV,cfg);

	udelay(10);

	/*
	 *	Now scan the cpu present map and fire up the other CPUs.
	 */

	SMP_PRINTK(("CPU map: %lx\n", cpu_present_map));

	for(i=0;i<NR_CPUS;i++) {
		/*
		 *	Don't even attempt to start the boot CPU!
		 */
		if (i == boot_cpu_id) {
			SMP_PRINTK(("BSP -- skipping\n"));
			continue;
		}

		if (cpu_present_map & (1 << i)) {
			unsigned long send_status, accept_status;
			int timeout, num_starts, j;

			/*
			 *	We need a kernel stack for each processor.
			 *	The LMM routines can get us low memory :)
			 */

			stack=(char *)lmm_alloc_page(&malloc_lmm, LMMF_1MB);    /* get 1 page < 1 MB */

			if(stack==NULL)
				panic("No memory for processor stacks.\n");

			kernel_stacks[i]=stack;
			install_trampoline(stack);

			SMP_PRINTK(("Booting processor %d stack %p: ",i,stack));		/* So we set what's up   */

			/*
			 *	This grunge runs the startup process for
			 *	the targeted processor.
			 */

#if 0
			SMP_PRINTK(("Setting warm reset code and vector.\n"));
			/*
			 *	Install a writable page 0 entry.
			 */

			cfg=pg0[0];

			CMOS_WRITE(0xa, 0xf);
			pg0[0]=7;
			local_flush_tlb();
			*((volatile unsigned short *) 0x469) = ((unsigned long)stack)>>4;
			*((volatile unsigned short *) 0x467) = 0;

			/*
			 *	Protect it again
			 */

			pg0[0]= cfg;
			local_flush_tlb();
#endif

			/*
			 *	Be paranoid about clearing APIC errors.
			 */

			if ( apic_version[i] & 0xF0 ) {
				apic_write(APIC_ESR, 0);
				accept_status = (apic_read(APIC_ESR) & 0xEF);
			}

			/*
			 *	Status is now clean
			 */

			send_status = 	0;
			accept_status = 0;

			/*
			 *	Starting actual IPI sequence...
			 */

			SMP_PRINTK(("Asserting INIT.\n"));

			/*
			 *	Turn INIT on
			 */

			cfg=apic_read(APIC_ICR2);
			cfg&=0x00FFFFFF;
			apic_write(APIC_ICR2, cfg|SET_APIC_DEST_FIELD(i)); 			/* Target chip     	*/
			cfg=apic_read(APIC_ICR);
			cfg&=~0xCDFFF;								/* Clear bits 		*/
			cfg |= (APIC_DEST_FIELD | APIC_DEST_LEVELTRIG
				| APIC_DEST_ASSERT | APIC_DEST_DM_INIT);
			apic_write(APIC_ICR, cfg);						/* Send IPI */

			udelay(200);
			SMP_PRINTK(("Deasserting INIT.\n"));

			cfg=apic_read(APIC_ICR2);
			cfg&=0x00FFFFFF;
			apic_write(APIC_ICR2, cfg|SET_APIC_DEST_FIELD(i));			/* Target chip     	*/
			cfg=apic_read(APIC_ICR);
			cfg&=~0xCDFFF;								/* Clear bits 		*/
			cfg |= (APIC_DEST_FIELD | APIC_DEST_LEVELTRIG
				| APIC_DEST_DM_INIT);
			apic_write(APIC_ICR, cfg);						/* Send IPI */

			/*
			 *	Should we send STARTUP IPIs ?
			 *
			 *	Determine this based on the APIC version.
			 *	If we don't have an integrated APIC, don't
			 *	send the STARTUP IPIs.
			 */

			if ( apic_version[i] & 0xF0 )
				num_starts = 2;
			else
				num_starts = 0;

			/*
			 *	Run STARTUP IPI loop.
			 */

			for (j = 1; !(send_status || accept_status)
				    && (j <= num_starts) ; j++) {
				SMP_PRINTK(("Sending STARTUP #%d.\n",j));

				apic_write(APIC_ESR, 0);

				/*
				 *	STARTUP IPI
				 */

				cfg=apic_read(APIC_ICR2);
				cfg&=0x00FFFFFF;
				apic_write(APIC_ICR2, cfg|SET_APIC_DEST_FIELD(i));			/* Target chip     	*/
				cfg=apic_read(APIC_ICR);
				cfg&=~0xCDFFF;								/* Clear bits 		*/
				cfg |= (APIC_DEST_FIELD
					| APIC_DEST_DM_STARTUP
					| (((int) stack) >> 12) );					/* Boot on the stack 	*/
				apic_write(APIC_ICR, cfg);						/* Kick the second 	*/

				timeout = 0;
				do {
					udelay(10);
				} while ( (send_status = (apic_read(APIC_ICR) & 0x1000))
					  && (timeout++ < 1000));
				udelay(200);

				accept_status = (apic_read(APIC_ESR) & 0xEF);
			}

			if (send_status) {		/* APIC never delivered?? */
				SMP_PRINTK(("APIC never delivered???\n"));
			}
			if (accept_status) {		/* Send accept error */
				SMP_PRINTK(("APIC delivery error (%lx).\n", accept_status));
			}
			if( !(send_status || accept_status) ) {
				for(timeout=0;timeout<50000;timeout++) {
					if(cpu_callin_map[0]&(1<<i))
						break;				/* It has booted */
					udelay(100);				/* Wait 5s total for a response */
				}
				if(cpu_callin_map[0]&(1<<i)) {
					cpucount++;
					/* number CPUs logically, starting from 1 (BSP is 0) */
					cpu_number_map[i] = cpucount;
					cpu_logical_map[cpucount] = i;
				} else {
					if(*((volatile unsigned char *)8192)==0xA5) {
						SMP_PRINTK(("Stuck ??\n"));
					} else {
						SMP_PRINTK(("Not responding.\n"));
					}
				}
			}
#if 0
/* what does this do? */
			/* mark "stuck" area as not stuck */
			*((volatile unsigned long *)8192) = 0;
#endif
		}

		/*
		 *	Make sure we unmap all failed CPUs
		 */

		if (cpu_number_map[i] == -1)
			cpu_present_map &= ~(1 << i);
	}

	/*
	 *	Cleanup possible dangling ends...
	 */
#if 0
	/*
	 *	Install writable page 0 entry.
	 */

	cfg = pg0[0];
	pg0[0] = 3;	/* writeable, present, addr 0 */
	local_flush_tlb();

	/*
	 *	Paranoid:  Set warm reset code and vector here back
	 *	to default values.
	 */

	CMOS_WRITE(0, 0xf);

	*((volatile long *) 0x467) = 0;

	/*
	 *	Restore old page 0 entry.
	 */

	pg0[0] = cfg;
	local_flush_tlb();

#endif

	/*
	 *	Allow the user to impress friends.
	 */

	if(cpucount==0) {
		SMP_PRINTK(("Error: only one processor found.\n"));
		cpu_present_map=(1<<smp_processor_id());
	} else {
		printk("Total of %d processors activated.\n", cpucount+1);
	}
	smp_num_cpus=cpucount+1;
	smp_activated=1;	/* XXX -- activate w/one processor? */
	if (num_processors!=smp_num_cpus) {
		printf("Error: configuration has %d processors, %d found\n",
			num_processors, smp_num_cpus);
		printf("adjusting count to %d\n", smp_num_cpus);
		num_processors=smp_num_cpus;
	}

	return 0;
}


#endif /* HAVE_CODE16 */
