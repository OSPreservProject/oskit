/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* Changes by Michael Weller. */


/* The code is a bit of a mess; also note that the drawing functions */
/* are not speed optimized (the gl functions are much faster). */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

/* These used to be static in vga.c */
extern int modeinfo_mask;
extern int color_text;		/* true if color text emulation */


static char *driver_names[] =
{"", "VGA", "ET4000", "Cirrus", "TVGA", "Oak", "EGA", "S3",
 "ET3000", "Mach32", "GVGA6400", "ARK", "ATI", "ALI", "Mach64", 
 "C&T", "APM", "NV3", NULL};

/* Chipset drivers */

/* vgadrv       Standard VGA (also used by drivers below) */
/* et4000       Tseng ET4000 (from original vgalib) */
/* cirrus       Cirrus Logic GD542x */
/* tvga8900     Trident TVGA 8900/9000 (derived from tvgalib) */
/* oak          Oak Technologies 037/067/077 */
/* egadrv       IBM EGA (subset of VGA) */
/* s3           S3 911 */
/* mach32       ATI MACH32 */
/* ark          ARK Logic */
/* gvga6400     Genoa 6400 (old SVGA) */
/* ati          ATI */
/* ali          ALI2301 */
/* mach64	ATI MACH64 */
/* chips	chips & technologies*/


extern unsigned __svgalib_maxhsync[];


/* Parse a string for options.. str is \0-terminated source,
   commands is an array of char ptrs (last one is NULL) containing commands
   to parse for. (if first char is ! case sensitive),
   func is called with ind the index of the detected command.
   func has to return the ptr to the next unhandled token returned by strtok(NULL," ").
   Use strtok(NULL," ") to get the next token from the file..
   mode is 1 when reading from conffile and 0 when parsing the env-vars. This is to
   allow disabling of dangerous (hardware damaging) options when reading the ENV-Vars
   of Joe user.
   Note: We use strtok, that is str is destroyed! */
static void parse_string(char *str, char **commands, char *(*func) (int ind, int mode), int mode)
{
    int index;
    register char *ptr, **curr;

    /*Pass one, delete comments,ensure only whitespace is ' ' */
    for (ptr = str; *ptr; ptr++) {
	if (*ptr == '#') {
	    while (*ptr && (*ptr != '\n')) {
		*ptr++ = ' ';
	    }
	    if (*ptr)
		*ptr = ' ';
	} else if (isspace(*ptr)) {
	    *ptr = ' ';
	}
    }
    /*Pass two, parse commands */
    ptr = strtok(str, " ");
    while (ptr) {
#ifdef DEBUG_CONF
	printf("Parsing: %s\n", ptr);
#endif
	for (curr = commands, index = 0; *curr; curr++, index++) {
#ifdef DEBUG_CONF
	    printf("Checking: %s\n", *curr);
#endif
	    if (**curr == '!') {
		if (!strcmp(*curr + 1, ptr)) {
		    ptr = (*func) (index, mode);
		    break;
		}
	    } else {
		if (!strcasecmp(*curr, ptr)) {
		    ptr = (*func) (index, mode);
		    break;
		}
	    }
	}
	if (!*curr)		/*unknow command */
	    ptr = strtok(NULL, " ");	/* skip silently til' next command */
    }
}

static int allowoverride = 0;	/* Allow dangerous options in ENV-Var or in */
				/* the $HOME/.svgalibrc */

static void process_config_file(FILE *file, int mode, char **commands,
		 			char *(*func)(int ind, int mode)) {
 struct stat st;
 char *buf, *ptr;
 int i;

  fstat(fileno(file), &st);	/* Some error analysis may be fine here.. */
  if ( (buf = alloca(st.st_size + 1)) == 0) {	/* + a final \0 */
    puts("svgalib: out of mem while parsing config file !");
    return;
  }
  fread(buf, 1, st.st_size, file);
  for (i = 0, ptr = buf; i < st.st_size; i++, ptr++) {
    if (!*ptr)
      *ptr = ' ';			/* Erase any maybe embedded \0 */
    }
  *ptr = 0;					/* Trailing \0 */
  parse_string(buf, commands, func, mode);	/* parse config file */
}

/* This is a service function for drivers. Commands and func are as above.
   The following config files are parsed in this order:
    - /etc/vga/libvga.conf (#define SVGALIB_CONFIG_FILE)
    - ~/.svgalibrc
    - the file where env variavle SVGALIB_CONFIG_FILE points
    - the env variable SVGALIB_CONFIG (for compatibility, but I would remove
      it, we should be more flexible...  Opinions ?)
    - MW: I'd rather keep it, doesn't do too much harm and is sometimes nice
      to have.
*/
void __svgalib_read_options(char **commands, char *(*func) (int ind, int mode)) {
    FILE *file;
    char *buf = NULL, *ptr;
    int i;

    if ( (file = fopen(SVGALIB_CONFIG_FILE, "r")) != 0) {
#ifdef DEBUG_CONF
  printf("Processing config file \'%s\'\n", SVGALIB_CONFIG_FILE);
#endif
      process_config_file(file, 1, commands, func);
      fclose(file);
    } else {
	fprintf(stderr, "svgalib: Configuration file \'%s\' not found.\n", SVGALIB_CONFIG_FILE);
    }

    if ( (ptr = getenv("HOME")) != 0) {
      char *filename; 

      filename = alloca(strlen(ptr) + 20);
      if (!filename) {
	puts("svgalib: out of mem while parsing SVGALIB_CONFIG_FILE !");
      } else {
	strcpy(filename, ptr);
	strcat(filename, "/.svgalibrc");
	if ( (file = fopen(filename, "r")) != 0) {
#ifdef DEBUG_CONF
	  printf("Processing config file \'%s\'\n", filename);
#endif
	  process_config_file(file, allowoverride, commands, func);
	  fclose(file);
	}
      }
    }

    if ( (ptr = getenv("SVGALIB_CONFIG_FILE")) != 0) {
      if ( (file = fopen(ptr, "r")) != 0) {
#ifdef DEBUG_CONF
  printf("Processing config file \'%s\'\n", ptr);
#endif
	process_config_file(file, allowoverride, commands, func);
	fclose(file);
      } else {
        fprintf(stderr, "svgalib: warning: config file \'%s\', pointed to by SVGALIB_CONFIG_FILE, not found !\n", ptr);
      }
    }

    if ( (ptr = getenv("SVGALIB_CONFIG")) != 0  &&  (i = strlen(ptr)) != 0) {
      buf = alloca(i + 1);
      if (!buf) {
	puts("svgalib: out of mem while parsing SVGALIB_CONFIG !");
      } else {
	strcpy(buf, ptr);		/* Copy for safety and strtok!! */
#ifdef DEBUG_CONF
	puts("Parsing env variable \'SVGALIB_CONFIG\'");
#endif
	parse_string(buf, commands, func, allowoverride);
      }
    }
}

/* Configuration file, mouse interface, initialization. */

static int configfileread = 0;	/* Boolean. */

/* What are these m0 m1 m... things ? Shouldn't they be removed ? */
static char *vga_conf_commands[] = {
    "mouse", "monitor", "!m", "!M", "chipset", "overrideenable", "!m0", "!m1", "!m2", "!m3",
    "!m4", "!m9", "!M0", "!M1", "!M2", "!M3", "!M4", "!M5", "!M6", "nolinear",
    "linear", "!C0", "!C1", "!C2", "!C3", "!C4", "!C5", "!C6", "!C7", "!C8", "!C9",
    "!c0", "!c1", "monotext", "colortext", "!m5",
    "leavedtr", "cleardtr", "setdtr", "leaverts", "clearrts",
    "setrts", "grayscale", "horizsync", "vertrefresh", "modeline",
    "security","mdev", "default_mode", "nosigint", "sigint",
    "joystick0", "joystick1", "joystick2", "joystick3",
    NULL};

static char *conf_mousenames[] =
{
  "Microsoft", "MouseSystems", "MMSeries", "Logitech", "Busmouse", "PS2",
    "MouseMan", "gpm", "Spaceball", NULL};

static int check_digit(char *ptr, char *digits)
{
    if (ptr == NULL)
	return 0;
    return strlen(ptr) == strspn(ptr, digits);
}

static char *process_option(int command, int mode)
{
    static char digits[] = ".0123456789";
    char *ptr, **tabptr, *ptb;
    int i, j;
    float f;

#ifdef DEBUG_CONF
    printf("command %d detected.\n", command);
#endif
    switch (command) {
    case 5:
#ifdef DEBUG_CONF
	puts("Allow override");
#endif
	if (mode)
	    allowoverride = 1;
	else
	    puts("Overrideenable denied. (Gee.. Do you think I'm that silly?)");
	break;
#ifndef OSKIT
    case 0:			/* mouse */
    case 2:			/* m */
	ptr = strtok(NULL, " ");
	if (ptr == NULL)
	    goto inv_mouse;
	if (check_digit(ptr, digits + 1)) {	/* It is a number.. */
	    i = atoi(ptr);
	    if ((i != 9) && (i >= sizeof(conf_mousenames)/sizeof(char *)))
		goto inv_mouse;
	    mouse_type = i;
	} else {		/* parse for symbolic name.. */
	    if (!strcasecmp(ptr, "none")) {
		mouse_type = 9;
		break;
	    }
	    for (i = 0, tabptr = conf_mousenames; *tabptr; tabptr++, i++) {
		if (!strcasecmp(ptr, *tabptr)) {
		    mouse_type = i;
		    goto leave;
		}
	    }
	  inv_mouse:
	    printf("svgalib: Illegal mouse setting: {mouse|m} %s\n"
		   "Correct usage: {mouse|m} mousetype\n"
		   "where mousetype is one of 0, 1, 2, 3, 4, 5, 6, 7, 9,\n",
		   (ptr != NULL) ? ptr : "");
	    for (tabptr = conf_mousenames; *tabptr; tabptr++) {
		printf("%s, ", *tabptr);
	    }
	    puts("or none.");
	    return ptr;		/* Allow a second parse of str */
	}
	break;
#endif /* !OSKIT */
    case 1:			/* monitor */
    case 3:			/* M */
	ptr = strtok(NULL, " ");
	if (check_digit(ptr, digits + 1)) {	/* It is an int.. */
	    i = atoi(ptr);
	    if (i < 7) {
		command = i + 12;
		goto monnum;
	    } else {
		f = i;
		goto monkhz;
	    }
	} else if (check_digit(ptr, digits)) {	/* It is a float.. */
	    f = atof(ptr);
	  monkhz:
	    if (!mode)
		goto mon_deny;
	    __svgalib_horizsync.max = f * 1000.0f;
	} else {
	    printf("svgalib: Illegal monitor setting: {monitor|M} %s\n"
		   "Correct usage: {monitor|M} monitortype\n"
		   "where monitortype is one of 0, 1, 2, 3, 4, 5, 6, or\n"
		   "maximal horz. scan frequency in khz.\n"
		   "Example: monitor 36.5\n",
		   (ptr != NULL) ? ptr : "");
	    return ptr;		/* Allow a second parse of str */
	}
	break;
    case 4:			/* chipset */
	ptr = strtok(NULL, " ");
	if (ptr == NULL) {
	    puts("svgalib: Illegal chipset setting: no chipset given");
	    goto chip_us;
	}
	/*First param is chipset */
	for (i = 0, tabptr = driver_names; *tabptr; tabptr++, i++) {
	    if (!strcasecmp(ptr, *tabptr)) {
		if (!__svgalib_driverspecslist[i]) {
		    printf("svgalib: Illegal chipset setting: Driver for %s is NOT compiled in.\n",
			ptr);
		    continue; /* The for above will loop a few more times and fail */
		}
		ptr = strtok(NULL, " ");
		if (check_digit(ptr, digits + 1)) {
		    j = atoi(ptr);
		    ptr = strtok(NULL, " ");
		    if (check_digit(ptr, digits + 1)) {
			if (mode)
			    vga_setchipsetandfeatures(i, j, atoi(ptr));
			else {
			  chipdeny:
			    puts("chipset override from environment denied.");
			}
			return strtok(NULL, " ");
		    } else {
			puts("svgalib: Illegal chipset setting: memory is not a number");
			goto chip_us;
		    }
		}
		if (mode)
		    vga_setchipset(i);
		else
		    puts("chipset override from environment denied.");
		return ptr;
	    }
	}
	printf("svgalib: Illegal chipset setting: chipset %s\n", ptr);
      chip_us:
	puts("Correct usage: chipset driver [par1 par2]\n"
	     "where driver is one of:");
	ptb = "%s";
	for (i = 0, tabptr = driver_names; *tabptr; tabptr++, i++) {
	    if (__svgalib_driverspecslist[i] != NULL) {
		printf(ptb, *tabptr);
		ptb = ", %s";
	    }
	}
	puts("\npar1 and par2 are river dependant integers.\n"
	     "Example: Chipset VGA    or\n"
	     "Chipset VGA 0 512");
	return ptr;
#ifndef OSKIT
    case 6:			/* oldstyle config: m0-m4 */
    case 7:
    case 8:
    case 9:
    case 10:
	mouse_type = command - 6;
	break;
    case 11:			/* m9 */
	mouse_type = 9;
	break;
#endif /* !OSKIT */
    case 12:			/* oldstyle config: M0-M6 */
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
      monnum:
	if (!mode) {
	  mon_deny:
	    puts("Monitor setting from environment denied.");
	    break;
	} else {
	    __svgalib_horizsync.max = __svgalib_maxhsync[command - 12];
	}
	break;
    case 19:			/*nolinear */
	modeinfo_mask &= ~CAPABLE_LINEAR;
	break;
    case 20:			/*linear */
	modeinfo_mask |= CAPABLE_LINEAR;
	break;
    case 21:			/* oldstyle chipset C0 - C9 */
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
	if (!mode)
	    goto chipdeny;
	vga_setchipset(command - 21);
	break;
    case 31:			/* c0-c1 color-text selection */
	if (!mode) {
	  coltexdeny:
	    puts("Color/mono text selection from environment denied.");
	    break;
	}
	color_text = 0;
	break;
    case 32:
	if (!mode) {
	    puts("Color/mono text selection from environment denied.");
	    break;
	}
	color_text = 1;
	break;
    case 33:
    case 34:
	if (!mode)
	    goto coltexdeny;
	color_text = command - 32;
	break;
#ifndef OSKIT
    case 35:			/* Mouse type 5 - "PS2". */
	mouse_type = 5;
	break;
    case 36:
	mouse_modem_ctl &= ~(MOUSE_CHG_DTR | MOUSE_DTR_HIGH);
	break;
    case 37:
	mouse_modem_ctl &= ~MOUSE_DTR_HIGH;
	mouse_modem_ctl |= MOUSE_CHG_DTR;
	break;
    case 38:
	mouse_modem_ctl |= (MOUSE_CHG_RTS | MOUSE_RTS_HIGH);
	break;
    case 39:
	mouse_modem_ctl &= ~(MOUSE_CHG_RTS | MOUSE_RTS_HIGH);
	break;
    case 40:
	mouse_modem_ctl &= ~MOUSE_RTS_HIGH;
	mouse_modem_ctl |= MOUSE_CHG_RTS;
	break;
    case 41:
	mouse_modem_ctl |= (MOUSE_CHG_RTS | MOUSE_RTS_HIGH);
	break;
#endif /* !OSKIT */
    case 42:			/* grayscale */
	__svgalib_grayscale = 1;
	break;
    case 43:			/* horizsync */
	ptr = strtok(NULL, " ");
	if (check_digit(ptr, digits)) {		/* It is a float.. */
	    f = atof(ptr);
	    if (!mode)
		goto mon_deny;
	    __svgalib_horizsync.min = f * 1000;
	} else
	    goto hs_bad;

	ptr = strtok(NULL, " ");
	if (check_digit(ptr, digits)) {		/* It is a float.. */
	    f = atof(ptr);
	    if (!mode)
		goto mon_deny;
	    __svgalib_horizsync.max = f * 1000;
	} else {
	  hs_bad:
	    printf("svgalib: Illegal HorizSync setting.\n"
		   "Correct usage: HorizSync min_kHz max_kHz\n"
		   "Example: HorizSync 31.5 36.5\n");
	}
	break;
    case 44:			/* vertrefresh */
	ptr = strtok(NULL, " ");
	if (check_digit(ptr, digits)) {		/* It is a float.. */
	    f = atof(ptr);
	    if (!mode)
		goto mon_deny;
	    __svgalib_vertrefresh.min = f;
	} else
	    goto vr_bad;

	ptr = strtok(NULL, " ");
	if (check_digit(ptr, digits)) {		/* It is a float.. */
	    f = atof(ptr);
	    if (!mode)
		goto mon_deny;
	    __svgalib_vertrefresh.max = f;
	} else {
	  vr_bad:
	    printf("svgalib: Illegal VertRefresh setting.\n"
		   "Correct usage: VertRefresh min_Hz max_Hz\n"
		   "Example: VertRefresh 50 70\n");
	}
	break;
    case 45:{			/* modeline */
	    MonitorModeTiming mmt;
	    const struct {
		char *name;
		int val;
	    } options[] = {
		{
		    "-hsync", NHSYNC
		},
		{
		    "+hsync", PHSYNC
		},
		{
		    "-vsync", NVSYNC
		},
		{
		    "+vsync", PVSYNC
		},
		{
		    "interlace", INTERLACED
		},
		{
		    "interlaced", INTERLACED
		},
		{
		    "doublescan", DOUBLESCAN
		}
	    };
#define ML_NR_OPTS (sizeof(options)/sizeof(*options))

	    /* Skip the name of the mode */
	    ptr = strtok(NULL, " ");
	    if (!ptr)
		break;

	    ptr = strtok(NULL, " ");
	    if (!ptr)
		break;
	    mmt.pixelClock = atof(ptr) * 1000;

#define ML_GETINT(x) \
	ptr = strtok(NULL, " "); if(!ptr) break; \
	mmt.##x = atoi(ptr);

	    ML_GETINT(HDisplay);
	    ML_GETINT(HSyncStart);
	    ML_GETINT(HSyncEnd);
	    ML_GETINT(HTotal);
	    ML_GETINT(VDisplay);
	    ML_GETINT(VSyncStart);
	    ML_GETINT(VSyncEnd);
	    ML_GETINT(VTotal);
	    mmt.flags = 0;
	    while ((ptr = strtok(NULL, " "))) {
		for (i = 0; i < ML_NR_OPTS; i++)
		    if (!strcasecmp(ptr, options[i].name))
			mmt.flags |= options[i].val;
		if (i == ML_NR_OPTS)
		    break;
	    }
#undef ML_GETINT
#undef ML_NR_OPTS

	    __svgalib_addusertiming(&mmt);
	    return ptr;
	    
	}
#ifndef OSKIT
    case 46:
	if (!mode) {
	    puts("Security setting from environment denied.");
	    break;
	}
        if ( (ptr = strtok( NULL, " ")) ) {
            if (!strcasecmp("revoke-all-privs", ptr)) {
                 __svgalib_security_revokeallprivs = 1;
		 break;
            } else if (!strcasecmp("compat", ptr)) {
                 __svgalib_security_revokeallprivs = 0;
		 break;
	    }
	} 
        puts("svgalib: Unknown security options\n");
        break;
    case 47:
	ptr = strtok(NULL," ");
	if (ptr) {
	    mouse_device = strdup(ptr);
	    if (mouse_device == NULL) {
#ifndef SVGA_AOUT
              nomem:
#endif
		puts("svgalib: Fatal error: out of memory.");
		exit(1);
	    }
	} else
	    goto param_needed;
	break;
    case 48:		/* default_mode */
	if ( (ptr = strtok(NULL, " ")) != 0) {
	 int mode = vga_getmodenumber(ptr);
	  if (mode != -1) {
	    __svgalib_default_mode = mode;
	  } else {
	    printf("svgalib: config: illegal mode \'%s\' for \'%s\'\n",
	   			  ptr, vga_conf_commands[command]);
	  }
	} else {
  param_needed:
  	  printf("svgalib: config: \'%s\' requires parameter(s)",
  	  				vga_conf_commands[command]);
	  break;
	}
	break;
    case 49: /* nosigint */
	__svgalib_nosigint = 1;
	break;
    case 50: /* sigint */
	__svgalib_nosigint = 0;
	break;
    case 51: /* joystick0 */
    case 52: /* joystick1 */
    case 53: /* joystick2 */
    case 54: /* joystick3 */
	if (! (ptr = strtok(NULL, " ")) )
		goto param_needed;
#ifndef SVGA_AOUT
	if (__joystick_devicenames[command - 51])
	    free(__joystick_devicenames[command - 51]);
	__joystick_devicenames[command - 51] = strdup(ptr);
	if (!__joystick_devicenames[command - 51])
	    goto nomem;
#else
	printf("svgalib: No joystick support in a.out version.\n");
#endif
	break;
#endif /* !OSKIT */
    }
  leave:
    return strtok(NULL, " ");
}

void readconfigfile(void)
{
    if (configfileread)
	return;
    configfileread = 1;
#ifndef OSKIT
    mouse_type = -1;
#endif /* !OSKIT */
    __svgalib_read_options(vga_conf_commands, process_option);
#ifndef OSKIT
    if (mouse_type == -1) {
	mouse_type = MOUSE_MICROSOFT;	/* Default. */
	puts("svgalib: Assuming Microsoft mouse.");
    }
#endif /* !OSKIT */
    if (__svgalib_horizsync.max == 0U) {
	/* Default monitor is low end SVGA/8514. */
	__svgalib_horizsync.min = 31500U;
	__svgalib_horizsync.max = 35500U;
	puts("svgalib: Assuming low end SVGA/8514 monitor (35.5 KHz).");
    }
#ifdef DEBUG_CONF
    printf("Mouse is: %d Monitor is: H(%5.1f, %5.1f) V(%u,%u)\n", mouse_type,
      __svgalib_horizsync.min / 1000.0, __svgalib_horizsync.max / 1000.0,
	   __svgalib_vertrefresh.min, __svgalib_vertrefresh.max);
    printf("Mouse device is: %s",mouse_device);
#endif
}
