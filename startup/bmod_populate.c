/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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

#include <oskit/fs/dir.h>
#include <oskit/io/absio.h>
#include <oskit/io/bufio.h>
#include <oskit/machine/multiboot.h>
#include <oskit/machine/base_multiboot.h>
#include <oskit/machine/base_vm.h>
#include <oskit/fs/memfs.h>
#include <stdio.h>
#include <string.h>
#include <oskit/boolean.h>
#include <oskit/c/assert.h>
#include <oskit/io/blkio.h>

/*
 * Populate a directory with the contents of the multiboot bmods
 *
 * If the free_routine is passed in, this routine will free each
 * bmod as it is entered into the filesystem.  The free routine
 * will be called with the start address & length of the bmod.
 * (that's the virtual address).
 */
  
int
bmod_populate(oskit_dir_t *dest,
	      void (*free_routine)(void *data, oskit_addr_t addr,
				   oskit_size_t len),
	      void *free_data)
{
	int flags, i, mods_count;
	oskit_addr_t mods_addr, file_offs;
	oskit_dir_t *sub, *addto;
	oskit_absio_t *thefilebuf;
	oskit_file_t *thefile;
        struct multiboot_module *m;
	oskit_u32_t actual;

	flags = boot_info.flags;
	mods_count = boot_info.mods_count;
	mods_addr = phystokv(boot_info.mods_addr);

	m = (struct multiboot_module *)mods_addr;

	if (!dest) {
		return -1;
	}

	/* Don't bother doing any work if they didn't include any mods */
	if (!(flags & MULTIBOOT_MODS)) {
		return 0;
	}

	for (i = 0; i < mods_count; i++, m++) {
		char *p;
		char *name = (char *) phystokv(m->string);
		oskit_addr_t start, end, blocklen;
		oskit_error_t rc;
		
		/*
		 * Skip any leading slashes in the name.
		 */
		while (*name == '/')
			++name;

		/*
		 * Create any intervening directories in the pathname
		 */

		addto = dest;
		while (1) {
			p = strchr(name, '/');
			if (!p) {
				break;
			}

			*p = '\0';

			rc = oskit_dir_lookup(addto, name,
					      (oskit_file_t **)&sub);

			if (rc == 0) {
				addto = sub;
				oskit_dir_release(sub);
			} else {
				assert(rc == OSKIT_ENOENT);
				rc = oskit_dir_mkdir(addto, name, 0777);
				assert(rc == 0);
				rc = oskit_dir_lookup(addto, name,
						      (oskit_file_t **)&sub);
				assert(rc == 0);
				addto = sub;
				oskit_dir_release(sub);
			}
			name = p + 1;
		}
		/* Create a file in the directory */
		rc = oskit_dir_create(addto, name, 1, 0666, &thefile);
		if (rc == OSKIT_EEXIST) {
			printf("bmod_populate: `%s' already exists, ignored\n",
			       (char *)m->string);
			if (free_routine) {
				start = phystokv(m->mod_start);
				end = phystokv(m->mod_end);
				free_routine(free_data, start, end-start);
			}
			continue;
		}
		assert (rc == 0);
		/* Grab its absio interface */
		rc = oskit_file_query(thefile, &oskit_absio_iid,
				      (void **)&thefilebuf);
		assert (rc == 0);

		start = phystokv(m->mod_start);
		end = phystokv(m->mod_end);

		file_offs = 0;
		while (start < end) {
			blocklen = end - start;
			rc = oskit_bufio_write((oskit_bufio_t *)thefilebuf,
					       (char *)start,
					       file_offs, blocklen,
					       &actual);
			assert(rc == 0);
			start += actual;
			file_offs += actual;
		}
		if (free_routine) {
			start = phystokv(m->mod_start);
			free_routine(free_data, start, (end-start));
		}
		oskit_absio_release(thefilebuf);
		oskit_file_sync(thefile, TRUE);
		oskit_file_release(thefile);

	}	

	return 0;
}
