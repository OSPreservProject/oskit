#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libvga.h"


#define PCI_CONF_ADDR  0xcf8
#define PCI_CONF_DATA  0xcfc


static int pci_read_header (unsigned char bus, unsigned char device,
        unsigned char fn, unsigned long *buf)
{
  int i;
  unsigned long bx = ((fn&7)<<8) | ((device&31)<<11) | (bus<<16) |
                        0x80000000;

  for (i=0; i<16; i++) {
        outl (PCI_CONF_ADDR, bx|(i<<2));
        buf[i] = inl (PCI_CONF_DATA);
  }

  return 0;
}


/* find a vga device of the specified vendor, and return
   its configuration (16 dwords) in conf 
   return zero if device found.
   */ 
int __svgalib_pci_find_vendor_vga(unsigned int vendor, unsigned long *conf)
{ unsigned long buf[16];
  int bus,device,cont;
  
  cont=1;

#ifndef OSKIT
  if (getenv("IOPERM") == NULL) {
        if (iopl(3) < 0) {
	    printf("svgalib: vgapci: cannot get I/O permissions\n");
	    exit(1);
	}
  }
#endif

  for(bus=0;(bus<16)&&cont;bus++)              
    for(device=0;(device<256)&&cont;device++){
      pci_read_header(bus,device,0,buf);
      if(((buf[0]&0xffff)==vendor)&&(((buf[2]>>16)&0xffff)==0x0300))cont=0;
    };
  if(!cont){memcpy(conf,buf,64); };
/*  for(i=0;i<16;i++)printf("%u\n",buf[i]);*/

#ifndef OSKIT
  if (getenv("IOPERM") == NULL)
  	iopl(0);
#endif

  return cont;
}

