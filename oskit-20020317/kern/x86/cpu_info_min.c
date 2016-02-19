/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Routine to determine the minimum or least-common-denominator
 * of the contents of two cpu_info structures.
 * Typically used on SMP systems to determine the basic feature set
 * common across all processors in the system regardless of type.
 */

#include <string.h>
#include <oskit/x86/cpuid.h>

void cpu_info_min(struct cpu_info *id, const struct cpu_info *id2)
{
	unsigned i, j;

	/*
	 * First calculate the minimum family, model, and stepping values.
	 * These values are treated as if concatenated into one big number,
	 * family being "most significant" and stepping being "least".
	 */
	if (id2->family < id->family) {
		id->family = id2->family;
		id->model = id2->model;
		id->stepping = id2->stepping;
	} else if (id2->family == id->family) {
		if (id2->model < id->model) {
			id->model = id2->model;
			id->stepping = id2->stepping;
		} else if (id2->model == id->model) {
			if (id2->stepping < id->stepping)
				id->stepping = id2->stepping;
		}
	}

	/*
	 * Don't bother doing anything with cpu_info.type;
	 * it's not really useful to software anyway.
	 */

	/* Take the minimum of the feature flags... */
	id->feature_flags &= id2->feature_flags;

	/*
	 * If we ever see the day when an SMP system
	 * contains multiple processors from different vendors,
	 * just clear the vendor ID string to "unknown".
	 */
	if (memcmp(id->vendor_id, id2->vendor_id, 12) != 0)
		memset(id->vendor_id, 0, 12);

	/*
	 * Take the minimum of the cache config descriptors,
	 * eliminating any that aren't in both processor descriptions.
	 */
	for (i = 0; i < sizeof(id->cache_config); i++) {
		unsigned char id_desc = id->cache_config[i];
		if (id_desc != 0) {
			for (j = 0; ; j++) {
				if (j >= sizeof(id2->cache_config)) {
					/* Not found in id2 - remove from id */
					id->cache_config[i] = 0;
					break;
				}
				if (id2->cache_config[j] == id_desc) {
					/* Found in id2 - leave in id */
					break;
				}
			}
		}
	}
}

