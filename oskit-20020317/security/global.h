/* FLASK */

/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
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
 * Global definitions that are included at the beginning
 * of every file using the -include directive.
 *
 * These definitions are used to permit the same
 * source code to be used to build both the security
 * server component and the checkpolicy program.
 */

#define CONFIG_FLASK 1
#define CONFIG_FLASK_AUDIT 1
#define CONFIG_FLASK_NOTIFY 1
#define CONFIG_FLASK_MLS 1

#include <flask/flask.h>

typedef oskit_security_context_t security_context_t;
typedef oskit_security_id_t security_id_t;
typedef oskit_access_vector_t access_vector_t;
typedef oskit_security_class_t security_class_t;

#define SECSID_NULL OSKIT_SECSID_NULL

#define SECINITSID_NUM OSKIT_SECINITSID_NUM
#define SECINITSID_FS OSKIT_SECINITSID_FS
#define SECINITSID_FILE OSKIT_SECINITSID_FILE
#define SECINITSID_PORT OSKIT_SECINITSID_PORT
#define SECINITSID_NETIF OSKIT_SECINITSID_NETIF
#define SECINITSID_NETMSG OSKIT_SECINITSID_NETMSG
#define SECINITSID_NODE OSKIT_SECINITSID_NODE

#define SECCLASS_PROCESS OSKIT_SECCLASS_PROCESS
#define SECCLASS_FILE OSKIT_SECCLASS_FILE
#define SECCLASS_DIR OSKIT_SECCLASS_DIR
#define SECCLASS_SOCK_FILE OSKIT_SECCLASS_SOCK_FILE
#define SECCLASS_LNK_FILE OSKIT_SECCLASS_LNK_FILE
#define SECCLASS_CHR_FILE OSKIT_SECCLASS_CHR_FILE
#define SECCLASS_BLK_FILE OSKIT_SECCLASS_BLK_FILE
#define SECCLASS_FIFO_FILE OSKIT_SECCLASS_FIFO_FILE

#include <oskit/net/socket.h>
#define AF_INET OSKIT_AF_INET

/* XXX: Where are the equivalent macros defined in the OSKit? */
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define cpu_to_le64(x) (x)
#define le64_to_cpu(x) (x)

typedef oskit_u8_t __u8;
typedef oskit_u16_t __u16;
typedef oskit_u32_t __u32;
typedef oskit_u64_t __u64;

#ifndef __KERNEL__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]

#else

#include <oskit/c/string.h>
#include <oskit/fs/openfile.h>
#include <oskit/c/ctype.h>

int oskit_security_avc_ss_reset(oskit_u32_t seqno);
#define avc_ss_reset(seqno) oskit_security_avc_ss_reset(seqno)

int sprintf(char *__dest, const char *__format, ...);

void *oskit_security_malloc(unsigned size);
void oskit_security_free(void *addr);
int oskit_security_printf(const char * fmt, ...);
void oskit_security_panic(const char * fmt, ...);

#define malloc(size) oskit_security_malloc(size)
#define free(addr) oskit_security_free(addr)
#define printf(fmt...) oskit_security_printf(fmt)
#define panic(fmt...) oskit_security_panic(fmt)

typedef struct oskit_openfile FILE;

typedef oskit_u32_t size_t;

extern inline oskit_u32_t fread(void *buf, oskit_u32_t size, oskit_u32_t nitems,
 FILE *fp)
{
        oskit_u32_t nbytes;
        oskit_error_t error;

        error = oskit_openfile_read(fp, buf, nitems*size, &nbytes);
        if (error)
                return -1;

        return nbytes;
}


#define exit(error_code) panic("SS: exiting (%d)",error_code)

#define ENOMEM (-OSKIT_ENOMEM)
#define EEXIST (-OSKIT_EEXIST)
#define ENOENT (-OSKIT_ENOENT)
#define EINVAL (-OSKIT_EINVAL)

#ifndef NULL
#define NULL 0
#endif

/* anything to silence warnings about ss_lock and flags not being used */
typedef oskit_u32_t spinlock_t;
#define SPIN_LOCK_UNLOCKED 0
#define spin_lock_irqsave(l,flags) *(l) = (flags) = 0
#define spin_unlock_irqrestore(l,flags)

/*
 nm liboskit_security.a | grep '[TDRC]' | grep -v oskit_ | awk '{ printf("#defin
e %s oskit_security_%s\n", $3, $3);}' | sort
 */

#define avtab_destroy oskit_security_avtab_destroy
#define avtab_hash_eval oskit_security_avtab_hash_eval
#define avtab_init oskit_security_avtab_init
#define avtab_insert oskit_security_avtab_insert
#define avtab_map oskit_security_avtab_map
#define avtab_read oskit_security_avtab_read
#define avtab_search oskit_security_avtab_search
#define constraint_expr_destroy oskit_security_constraint_expr_destroy
#define context_cpy oskit_security_context_cpy
#define ebitmap_cmp oskit_security_ebitmap_cmp
#define ebitmap_contains oskit_security_ebitmap_contains
#define ebitmap_cpy oskit_security_ebitmap_cpy
#define ebitmap_destroy oskit_security_ebitmap_destroy
#define ebitmap_get_bit oskit_security_ebitmap_get_bit
#define ebitmap_or oskit_security_ebitmap_or
#define ebitmap_read oskit_security_ebitmap_read
#define ebitmap_set_bit oskit_security_ebitmap_set_bit
#define hashtab_create oskit_security_hashtab_create
#define hashtab_destroy oskit_security_hashtab_destroy
#define hashtab_insert oskit_security_hashtab_insert
#define hashtab_map oskit_security_hashtab_map
#define hashtab_map_remove_on_error oskit_security_hashtab_map_remove_on_error
#define hashtab_remove oskit_security_hashtab_remove
#define hashtab_replace oskit_security_hashtab_replace
#define hashtab_search oskit_security_hashtab_search
#define mls_compute_av oskit_security_mls_compute_av
#define mls_compute_context_len oskit_security_mls_compute_context_len
#define mls_context_cpy oskit_security_mls_context_cpy
#define mls_context_isvalid oskit_security_mls_context_isvalid
#define mls_context_to_sid oskit_security_mls_context_to_sid
#define mls_convert_context oskit_security_mls_convert_context
#define mls_member_sid oskit_security_mls_member_sid
#define mls_read_level oskit_security_mls_read_level
#define mls_read_perms oskit_security_mls_read_perms
#define mls_read_range oskit_security_mls_read_range
#define mls_sid_to_context oskit_security_mls_sid_to_context
#define policydb oskit_security_policydb
#define policydb_context_isvalid oskit_security_policydb_context_isvalid
#define policydb_destroy oskit_security_policydb_destroy
#define policydb_index_classes oskit_security_policydb_index_classes
#define policydb_index_others oskit_security_policydb_index_others
#define policydb_init oskit_security_policydb_init
#define policydb_load_isids oskit_security_policydb_load_isids
#define policydb_read oskit_security_policydb_read
#define security_compute_av oskit_security_security_compute_av
#define security_context_to_sid oskit_security_security_context_to_sid
#define security_fs_sid oskit_security_security_fs_sid
#define security_load_policy oskit_security_security_load_policy
#define security_member_sid oskit_security_security_member_sid
#define security_netif_sid oskit_security_security_netif_sid
#define security_node_sid oskit_security_security_node_sid
#define security_notify_perm oskit_security_security_notify_perm
#define security_port_sid oskit_security_security_port_sid
#define security_sid_to_context oskit_security_security_sid_to_context
#define security_transition_sid oskit_security_security_transition_sid
#define sidtab oskit_security_sidtab
#define sidtab_context_to_sid oskit_security_sidtab_context_to_sid
#define sidtab_insert oskit_security_sidtab_insert
#define sidtab_map oskit_security_sidtab_map
#define sidtab_map_remove_on_error oskit_security_sidtab_map_remove_on_error
#define sidtab_remove oskit_security_sidtab_remove
#define sidtab_search oskit_security_sidtab_search
#define ss_initialized oskit_security_ss_initialized
#define symtab_init oskit_security_symtab_init

#endif
