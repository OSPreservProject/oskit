/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Support code for Roland's wacky "bus tree walk syntax" to
 * find devices in the oskit bus tree.
 */

#include <oskit/unsupp/bus_walk.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>
#include <oskit/dev/bus.h>
#include <oskit/dev/blk.h>
#include <oskit/dev/net.h>
#include <oskit/dev/isa.h>
#include <oskit/dev/ide.h>
#include <oskit/dev/scsi.h>
#include <oskit/dev/ethernet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Wacky, wacky, wacky "bus tree walk" syntax for devices.
   This is a nutty little syntax by which you can drive the iteration (and
   some recursion) of a walk over the bus tree to find the device you want.

   The fancy operators in this syntax are intended to make it possible to
   encode any of the canonical compatibility syntaxes for device names,
   e.g. the first ethernet device ("@>:etherdev") or the Nth device on the
   Mth IDE controller ("@+N>:idebus@.M") or the Nth disk on any SCSI
   controller ("@+N(:>scsibus,=disk)").

   It's about like this:

   	@COMPONENT1[@COMPONENT2...]

   We start at the root bus.  Each component yields a device object that is
   the root for the next iteration (or is the final device).  The yield of
   the last component is the resultant device.  As a special kludge, the
   final component can take the form @,SUBPART to indiciate a partition of
   the block device specified by the penultimate component.  Otherwise, the
   device yielded must be a bus to iterate on.  Each component can take one
   of these forms:

	.N		bus child at getchild index N
	POSITION	bus child whose getchild position string is POSITION
	=NAME		bus child whose getinfo name string is NAME
	:TYPE		bus child that recognizes interface TYPE
			TYPEs are strings naming IIDs (blkdev, idebus, etc)
	+N<SPEC>	skip over N matches for <SPEC> before yielding device
	>SPEC		consider SPEC over a depth-first bus walk from here,
			not just direct children of this bus
	(COMPONENT1[,COMPONTENT2...])
			process the list of bus components from here;
			this defines a new scope and grouping for +N skipping

   Thus: wd3 -> @+1>:idebus@.1
   			walk the system to find IDE busses and take the second;
			from that IDE bus, choose the child at index 1
         sd2s1 -> @+2(:>scsibus,=disk)@,s1
			walk the system to find SCSI busses and each SCSI bus
			to find children named "disk", and take the third
			in that global ordering;
			from that disk, take partition "s1"
	 eth0 -> @>:etherdev (or @+0>:etherdev)
			walk the system to find the first Ethernet device
   You can also encode more complex things, like:
	@>:usbbus@+1>:blkdev
			walk the system to find the first USB bus;
			walk that USB bus (down individual controllers
			and bus bridges) to find block devices, and take
			the second found anywhere on that USB bus */

/* Forward declaration for mutually recursive subfunction.  */
static oskit_error_t parse_bus_spec_component (oskit_bus_t *bus, /* consumed */
					       const char *name,
					       const char *name_end,
					       unsigned int *skip, int recurse,
					       oskit_device_t **com_devicep,
					       const char **subpartp);

/* Parse a full "bus device spec" found in NAME, starting at BUS.  */
static oskit_error_t
parse_bus_spec (oskit_bus_t *bus,
		const char *name, int separator, int terminator,
		unsigned int *skip, int recurse,
		oskit_device_t **com_devicep,
		const char **subpartp)
{
  oskit_error_t rc;
  oskit_device_t *com_device = 0;

  ++name;
  com_device = (oskit_device_t *) bus;
  while (*name != '\0')
    {
      const char *name_end = (strchr (name, separator)
			      ?: strchr (name, terminator));

      /* Get a device for this component of the spec.
	 This can recurse back to us to handle a subspec.  */
      rc = parse_bus_spec_component (bus, /* consumed */
				     name, name_end,
				     skip, recurse, &com_device, subpartp);
      if (OSKIT_FAILED (rc))
	return rc;

      /* Found our device.  */
      if (name_end[0] == '\0')	/* End of the whole spec, we are done! */
	break;
      name = name_end + 1;

      /* There is another component to the spec.  Unless it's the
	 partitioning kludge, this device better be a bus.  */
      rc = oskit_device_query (com_device, &oskit_bus_iid, (void **) &bus);
      if (OSKIT_FAILED (rc) && name[0] == ',')
	{
	  /* Kludge for partitioning.  */
	  *subpartp = name + 1;
	  break;
	}
      oskit_device_release (com_device);
      if (OSKIT_FAILED (rc))
	/* This device is not a bus, so the @... is bogus.  */
	return OSKIT_E_DEV_NOSUCH_DEV;
    }

  *com_devicep = com_device;
  return 0;
}


/* Parse one component of a bus spec, from [NAME,NAME_END) to be
   looked up within BUS.  Always consumes BUS.  */
static oskit_error_t
parse_bus_spec_component (oskit_bus_t *bus, /* consumed */
			  const char *name, const char *name_end,
			  unsigned int *skip, int recurse,
			  oskit_device_t **com_devicep,
			  const char **subpartp)
{
  oskit_error_t rc;
  char pos[OSKIT_BUS_POS_MAX];
  unsigned int i;

  /* First, we probe the bus so it knows what devices are on it.  */
  rc = oskit_bus_probe (bus);
  if (OSKIT_FAILED (rc) && rc != OSKIT_E_NOTIMPL)
    {
      oskit_bus_release (bus);
      return rc;
    }

  /* Now we parse this component of NAME (until the next '@').  */
  rc = OSKIT_E_DEV_NOSUCH_DEV;

 parse_component:
  switch (name[0])
    {
    case '.':		/* Let @.N mean child N (0-origin) of this bus.  */
      {
	char *end;
	i = strtoul (name + 1, &end, 0);
	if (end == name_end)
	  rc = oskit_bus_getchild (bus, i * (1 + *skip), com_devicep, pos);
	break;
      }

    case '=':		/* Let @=NAME mean child whose name is NAME. */
      {
	/* Iterate over this bus's children looking for a match.
	   For this check we need to make a getinfo call on the device
	   itself.  */
	for (i = 0;; ++i)
	  {
	    oskit_devinfo_t info;
	    oskit_bus_t *childbus;
	    rc = oskit_bus_getchild (bus, i, com_devicep, pos);
	    if (OSKIT_FAILED (rc))
	      break;
	    rc = oskit_device_getinfo (*com_devicep, &info);
	    if (OSKIT_SUCCEEDED (rc)
		&& !strncmp (info.name, &name[1], name_end - &name[1]))
	      {			/* We got a name match! */
		if (*skip == 0)
		  break;	/* We got our device! */
		--*skip;	/* Skip this match and keep going.  */
	      }
	    else if (recurse && !oskit_device_query (*com_devicep,
						     &oskit_bus_iid,
						     (void **) &childbus))
	      {
		/* This wasn't a match, but it's a bus we can recurse
		   looking for one.  */
		oskit_device_release (*com_devicep);
		rc = parse_bus_spec_component (childbus,
					       name, name_end, skip, 1,
					       com_devicep, subpartp);
		if (OSKIT_SUCCEEDED (rc))
		  break;	/* That did it.  */
		continue;	/* Nope, keep looking.  */
	      }
	    oskit_device_release (*com_devicep);
	  }
	break;
      }

    case ':':		/* Let @:TYPE mean child supporting interface TYPE.  */
      {
	/* First turn the TYPE name into an IID to look for.  */
	static const struct
	{
	  const char *name;
	  oskit_size_t namelen;
	  const oskit_iid_t *iid;
	} iid_names[] =
	  {
#define IID(name)	{ #name, sizeof #name - 1, &oskit_##name##_iid }
	    IID (blkdev),
	    IID (etherdev),
	    IID (isabus),
	    IID (scsibus),
	    IID (idebus),
	  };
	const oskit_iid_t *iid = 0;
	for (i = 0; i < sizeof iid_names / sizeof iid_names[0]; ++i)
	  if (name_end - &name[1] == iid_names[i].namelen &&
	      !memcmp (&name[1], iid_names[i].name, name_end - &name[1]))
	    {
	      iid = iid_names[i].iid;
	      break;
	    }
	if (iid == 0)		/* No match, no joy. */
	  break;

	/* Iterate over this bus's children looking for a match.
	   For this check we need to query the device itself.  */
	for (i = 0;; ++i)
	  {
	    oskit_bus_t *childbus;
	    void *iidi;
	    rc = oskit_bus_getchild (bus, i, com_devicep, pos);
	    if (OSKIT_FAILED (rc))
	      break;
	    rc = oskit_device_query (*com_devicep, iid, &iidi);
	    if (OSKIT_SUCCEEDED (rc))
	      {			/* We got an interface match!  */
		oskit_device_release ((oskit_device_t *) iidi);
		if (*skip == 0)
		  break;	/* We got our device! */
		--*skip;	/* Skip this match and keep going.  */
	      }
	    else if (recurse && !oskit_device_query (*com_devicep,
						     &oskit_bus_iid,
						     (void **) &childbus))
	      {
		/* This wasn't a match, but it's a bus we can recurse
		   on looking for one.  */
		oskit_device_release (*com_devicep);
		rc = parse_bus_spec_component (childbus, /* consumed */
					       name, name_end, skip, 1,
					       com_devicep, subpartp);
		if (OSKIT_SUCCEEDED (rc))
		  break;	/* That did it.  */
		continue;	/* Nope, keep looking.  */
	      }
	    oskit_device_release (*com_devicep);
	  }
	break;
      }

    case '+':			/* Let @+N<SPEC> mean skip N matches.  */
      {
	char *end;
	i = strtoul (name + 1, &end, 0);
	if (end && end < name_end)
	  {
	    *skip += i;
	    name = end;
	    goto parse_component;
	  }
	break;
      }

    case '>':			/* Make this spec recurse on busses.  */
      {
	recurse = 1;
	++name;
	goto parse_component;
      }

    case '(':			/* Recurse with a new scope for subspecs.  */
      if (name_end[-1] == ')')
	{
	  unsigned int subskip = 0;
	  return parse_bus_spec (bus, name, ',', ')', &subskip, recurse,
				 com_devicep, subpartp);
	}
      break;

    default:		/* @POS -> match position string to POS.  */
      /* Iterate over this bus's children looking for a match.  */
      for (i = 0;; ++i)
	{
	  oskit_size_t len;
	  rc = oskit_bus_getchild (bus, i, com_devicep, pos);
	  if (OSKIT_FAILED (rc))
	    break;
	  len = strlen (pos);
	  if (len == name_end - name && !memcmp (pos, name, len))
	    {
	      if (*skip == 0)
		break;
	      --*skip;
	    }
	  oskit_device_release (*com_devicep);
	}
    }

  oskit_bus_release (bus);

  return rc;
}

oskit_error_t
oskit_bus_walk_lookup (const char *name,
		       oskit_device_t **com_devicep,
		       const char **subpartp)
{
  unsigned int skip = 0;
  *subpartp = 0;
  return parse_bus_spec (osenv_rootbus_getbus (), name, '@', '\0',
			 &skip, 0, com_devicep, subpartp);
}


/*
 * Turn a device string compatible with BSD, Linux, or Mach into
 * a specification in bus walk syntax.
 */
int
bus_walk_from_compat_string (const char *name,
			     char constructed[])
{
	unsigned int n;
	char *end;
	const char *subpart = 0;

	if (!strncmp (name, "/dev/", 5))
		name += 5;

#define RETURN(fmt, args...) \
	return (sprintf(constructed, fmt "%s%s", args, \
			subpart ? "@," : "", subpart ? subpart : ""), 1)

	if (name[1] == 'd' && name[2] >= '0' && name[2] <= '9') {
		n = strtoul (&name[2], &end, 10);
		if (end && (*end == '\0' || *end == 's')) {
			if (*end != '\0')
				subpart = end;
			switch (name[0]) {
			case 'h': /* Mach hd[0123] -> IDE disk */
			case 'w': /* BSD wd[0123] same */
				/* Each IDE controller supports two drives;
				   the low bit chooses the drive on the
				   controller, and the remaining high bits
				   choose the (N/2)th IDE controller found
				   depth-first in the bus tree.  */
				RETURN("@+%u>:idebus@.%u", n / 2, n % 2);
			case 'f': /* fdN -> floppy disk */
				/* Each floppy controller supports up to
				   four drives.  The low two bits choose
				   the drive on the controller, and the
				   remaining high bits choose the (N/4)/th
				   floppy controller found depth-first on
				   the bus tree.  */
				RETURN("@+%u>=floppy@.%u", n / 4, n % 4);
			case 's': /* sdN -> SCSI disk */
				/* There may be any number of SCSI
				   controllers in the system, and each may
				   support any number (well, up to 16)
				   drives.  We want to simply find the Nth
				   SCSI drive in the system.  We order the
				   drives on each controller by their SCSI
				   IDs and LUNs, and controllers as they
				   are found depth-first in the bus tree.  */
				RETURN("@+%u(>:scsibus,=disk)", n);
			}
		}
	}
	else if (name[1] == 'd' && name[2] >= 'a' && name[2] <= 'z') {
		n = strtoul(&name[3], &end, 10);
		if (end && *end == '\0') {
			n = name[2] - 'a';
			switch (name[0]) {
			case 'h': /* Linux hd[abcd] -> IDE disk */
				RETURN("@+%u>:idebus@.%u", n / 2, n % 2);
			case 's': /* Linux sd[abcd] -> SCSI disk */
				RETURN("@+%u(>:scsibus,=disk)", n);
			}
		}
	}
	else if (!strncmp (name, "eth", 3)) {
		/* Want Nth ethernet device in the system.  */
		n = strtoul (&name[4], &end, 10);
		if (end && *end == '\0')
			RETURN("@+%u>:etherdev", n);
	}

	/*
	 * Ya got me, fella.
	 */
	return 0;
}
