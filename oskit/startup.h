/*
 * Copyright (c) 1998, 1999, 2000, 2001 The University of Utah and the Flux Group.
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
 * Declarations for -loskit_startup functions, a random collection
 * of functions to start things up in vaguely canonical ways.
 */

#ifndef _OSKIT_STARTUP_H_
#define _OSKIT_STARTUP_H_

#include <oskit/error.h>
#include <oskit/io/blkio.h>
#include <oskit/fs/dir.h>
#include <oskit/dev/osenv.h>

/*
 * Start the system clock and plug it into libc.
 * This function should be called only once.
 */
void start_clock(void);

/*
 * Convenience function to initialize/probe device drivers,
 * using link-time dependencies to select sets to initialize.
 * This can safely be called more than once, but is not thread-safe.
 * You do not necessarily have to call this, since other start_*
 * functions do.
 */
void start_devices(void);

/*
 * More convenience functions to start either network, block or sound devices.
 */
void start_net_devices(void);
void start_blk_devices(void);
void start_sound_devices(void);

/*
 * Open the named disk device and partition on it (or the whole disk
 * if `partition' is null).
 * This calls start_devices if need be, and causes it to probe block devices.
 */
oskit_error_t start_disk(const char *disk, const char *partition,
			 int read_only, oskit_blkio_t **out_bio);


/*
 * Initialize the disk-based filesystem code, mount a filesystem from
 * the given block device, and plug it into libc.
 */
void start_fs_on_blkio(oskit_blkio_t *part);
void start_linux_fs_on_blkio(oskit_blkio_t *part);

/* version used with pthreads */
void start_fs_on_blkio_pthreads(oskit_blkio_t *part);
void start_linux_fs_on_blkio_pthreads(oskit_blkio_t *part);

/*
 * Start up the memfs code, and load the bmods from the kernel into
 * it.
 */
oskit_dir_t *start_bmod(void);

/* version used with pthreads. Returns a wrapped memfs adaptor. */
oskit_dir_t *start_bmod_pthreads(void);

/*
 * Same as above, but initialize the posix level filesystem as well.
 */
void start_fs_bmod(void);

/* version used with pthreads */
void start_fs_bmod_pthreads(void);

/*
 * Convenience code used by start_fs_bmod to fill up a memory
 * filesystem from a bmod.
 *
 * If free_routine is non-null, then it is called on each bmod
 * with free_routine(free_data, addr, len)
 *
 * Incidentally, you can pass lmm_add_free() in directly, since it
 * takes these arguments.
 */

int
bmod_populate(oskit_dir_t *dest,
	      void (*free_routine)(void *data, oskit_addr_t addr,
				   oskit_size_t len),
	      void *free_data);

/*
 * This just calls start_disk and start_fs_on_blkio for you.
 */
void start_fs(const char *diskname, const char *partname);
void start_linux_fs(const char *diskname, const char *partname);

/* version used with pthreads */
void start_fs_pthreads(const char *diskname, const char *partname);
void start_linux_fs_pthreads(const char *diskname, const char *partname);


/*
 * Start up a network interface using BOOTP, initialize the network stack,
 * and register its socketfactory.
 *
 * This calls start_devices if need be, and causes it to probe net devices.
 * This should be called only once.  This only configures the first network
 * device.
 *
 * Network device paramters, address and mask, can be overridden via:
 *	start_<name>_addr=<ipaddr>
 *	start_<name>_mask=<ipaddr>
 * environment variables, where <name> is the interface name, obtained via
 * the start_conf_network_eifname (defaults to de<num>, e.g. "de0").
 * <ipaddr> is a standard dotted quad address.
 *
 * Network host parameters (hostname, domainname, gateway and nameservers)
 * can be overridden via:
 *	start_hostname=<string>
 *	start_domainname=<string>
 *	start_gateway=<ipaddr>
 *	start_nameservers=<ipaddr>[,<ipaddr>]
 * environment variables, where <string> is a text string suitable for the
 * context (e.g., "fast" or "cs.utah.edu").  Note that start_nameservers is
 * a comma-seperated list of ipaddrs.
 *
 * If BOOTP information is available, that information is used as the default.
 * If no BOOTP and no environment information is supplied, the kernel will
 * prompt on the console for the information.
 */
void start_network(void);
void start_network_pthreads(void);

/*
 * Like start_network, but explicitly specify the interface to use.
 * Should only be called once.
 */
void start_network_single(int ifn);
void start_network_single_pthreads(int ifn);

/*
 * Like start_network, but starts up all of the network interfaces
 * and tweaks a magic FreeBSD sysctl variable to allow routing.
 * Should only be called once.
 */
void start_network_router(void);
void start_network_router_pthreads(void);

/*
 * Get configuration information for a specific ethernet interface using
 * a variety of sources.  Returns zero if all is well, an error otherwise.
 *
 * Interface-specific parameters are IP address and netmask which can be
 * obtained in a number of ways:
 *
 * If start_<name>_addr, start_<name>_mask environment variables are set,
 * use those.
 *
 * Otherwise, if a BOOTP request to the interface returns info, use that.
 *
 * Otherwise if ask is set, prompt the user on the console for values.
 *
 * Note that a BOOTP request is always sent (to obtain default machine-wide
 * information) even if the environment variables are set.  This implies
 * that the device must be opened and thus a probe for all interfaces may
 * happen on the first call to this function.
 */
int start_getinfo_network_eif(int ifn, char **addrp, char **maskp, int ask);

/*
 * Get machine-wide (host) network information using a variety of sources.
 * Returns zero if all is well, an error otherwise.
 *
 * Machine-wide parameters are hostname, domainname, default gateway,
 * and nameservers which can be obtained in a number of ways:
 *
 * If start_hostname, start_domainname, start_gateway, and start_nameservers
 * environment variables are set, use those.
 *
 * Otherwise, if previous interface initialization returned BOOTP info,
 * use that info.
 *
 * Otherwise if ask is set, prompt the user on the console for values.
 *
 * NOTE: this function should be called AFTER any start_getinfo_network_eif
 * calls to take advantage of any BOOTP info discovered via the interfaces.
 */
int start_getinfo_network_host(char **hname, char **dname, char **gwname,
			       char ***nsnames, int ask);

/*
 * XXX Put the following (start_conf_*) in a separate header file?
 */
#include <oskit/dev/ethernet.h>
#include <oskit/net/socket.h>
#include <oskit/net/freebsd.h>

/*
 * Initialize the network stack.  
 * Invoked by start_conf_network()
 */
int start_conf_network_init(oskit_osenv_t *osenv, oskit_socket_factory_t **fsc);

/*
 * Set various host-level network parameters.  Hostname and domainname are
 * text strings, gateway is an IP address string and nameservers is an array
 * of IP address strings (with the last element NULL).
 */
int start_conf_network_host(const char *hostname, const char *gateway,
			    const char *domainname, char **nameservers);

/*
 * Tweaks the magic FreeBSD sysctl variable to allow routing.
 */
void start_conf_network_router(void);

/*
 * Start and stop a specific interface.
 * Start opens and ifconfig's the interface using the given information.
 * Stop takes the interface down and closes the underlying device.
 */
int start_conf_network_eifstart(int ifn, const char *eif_addr,
				const char *eif_mask);
int start_conf_network_eifstop(int ifn);

/*
 * Open a particular ethernet interface.
 * Invoked by start_conf_network().
 */
int start_conf_network_open_eif(oskit_etherdev_t *etherdev,
				oskit_freebsd_net_ether_if_t **eif);
/*
 * Close a particular ethernet interface.
 * Invoked by the libstartup network shutdown code.
 */
void start_conf_network_close_eif(oskit_freebsd_net_ether_if_t *eif);

/*
 * Provide a name for a given ethernet interface.
 * Invoked by start_conf_network().
 */
const char *start_conf_network_eifname(int i);

/*
 * Configure a particular ethernet interface.
 * Invoked by start_conf_network().
 */
int start_conf_network_eifconfig(oskit_freebsd_net_ether_if_t *eif,
				 const char *eif_name,
				 const char *eif_addr,
				 const char *eif_mask);

/*
 * Marks a specific inteface as ``down''.
 */
int start_conf_network_eifdown(oskit_freebsd_net_ether_if_t *eif);

/*
 * Configure ethernet devices and networking stack.
 */
void start_conf_network(int maxdev);
void start_conf_network_pthreads(int maxdev);

/*
 * Start the SVM page-protection system, paging to the given disk partition.
 * If disk is `null', does no disk paging.  Otherwise, args are passed
 * to `start_disk' (see above).
 */
oskit_error_t start_svm(const char *disk, const char *partition);

/* version used with pthreads */
oskit_error_t start_svm_pthreads(const char *disk, const char *partition);

/*
 * Start the SVM system on a specific bio instance. In the pthreads case,
 * the bio should NOT be wrapped; it will be done internally.
 */
oskit_error_t start_svm_on_blkio(oskit_blkio_t *bio);

/* version used with pthreads */
oskit_error_t start_svm_on_blkio_pthreads(oskit_blkio_t *bio);

/*
 * Convenience function to start up clock, disk filesystem, swap, and network
 * in canonical ways and plug them into libc.
 *
 * Looks for environment/command-line variables called "root" and "swap"
 * for device/partition selection with user-friendly syntax, and variable
 * "read-only" to open the root fs read-only.  If there is no "root"
 * variable, makes the bmod fs the root fs.  If there is no "swap" variable,
 * turns on page-protection with SVM, but no disk paging.
 * Starts the network with bootp.  (See start_network() for more network
 * configuration.)
 *
 * This can safely be called more than once, but is not thread-safe.
 */
void start_world(void);

/* version used with pthreads */
void start_world_pthreads(void);

/*
 * Start the console. Typically called to replace the default console
 * stream with a real TTY device.
 */
void start_console(void);
void start_console_pthreads(void);

/*
 * Another convenience function to start the pthread system.
 */
void start_pthreads(void);

/*
 * And another convenience function to create the default osenv object.
 */
oskit_osenv_t *start_osenv(void);

/*
 * A couple of startup functions that use the unix emulation stuff
 * instead of the oskit drivers. 
 */
void	start_network_native(void);
void	start_network_native_pthreads(void);
void	start_fs_native(char *rootname);
void	start_fs_native_pthreads(char *rootname);

/*
 * Start gprof support code.
 */
void	start_gprof(void);
void	pause_gprof(int onoff);

/*
 * atexit support for the startup library to ensure that the startup atexits
 * get run all at once, instead of interspersed with non-startup atexits.
 */
int	startup_atexit(void (*function)(void *arg), void *arg);
void	startup_atexit_init(void);

#endif /* _OSKIT_STARTUP_H_ */
