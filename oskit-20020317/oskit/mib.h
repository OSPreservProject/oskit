/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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
 * Definitions used by OSKit component MIB interfaces.
 */
#ifndef _OSKIT_MIB_H_
#define _OSKIT_MIB_H_ 1

#include <oskit/machine/types.h>
#include <oskit/c/assert.h>
/* #include <oskit/com.h> --- needed by files that use the OPS macros. */


/*****************************************************************************/
/*
 * Basic data types to be exchanged across MIB interfaces.
 */

/*
 * Strings.  Note that these simple strings get ``wrapped up'' inside a
 * different type, `oskit_mib_mib_string_t', and it is this latter type that is
 * actually used in MIB interfaces.
 */
typedef struct oskit_mib_string oskit_mib_string_t;
struct oskit_mib_string {
	/*
	 * If `str' data includes a terminal NUL, then that NUL must be counted
	 * in `len'.
	 *
	 * If `len' > `max', that indicates that the provided buffer was too
	 * small to hold all of the data.  In this case, the buffer must
	 * contain `max' octets of data.
	 */
	char *		str;	/* Pointer to first octet. */
	oskit_u32_t	len;	/* # of octets that contain data. */
	oskit_u32_t	max;	/* # of octets in buffer (alloc'ed size). */
};

/*
 * IP addresses.  Define this ``by hand'' because I don't know if we want
 * `in_addr' from the OSKit minimal C library or from the FreeBSD C library.
 * Again, note that `oskit_mib_in_addr_t' gets wrapped up inside another type
 * below.
 *
 * XXX --- Should we just use `oskit_u32_t's for slots of this type?
 * XXX --- Should we define `oskit_in_addr_t' in <oskit/types.h> or somesuch?
 */
typedef oskit_u32_t oskit_mib_in_addr_t;

/*
 * The actual types that pass across MIB interfaces are structures defined via
 * the `OSKIT_MIB_DEFSTRUCT' macro below.  These structures contain a value and
 * a flag indicating whether or not the value is actually valid.  By using
 * these structures, MIB implementations can say ``no value'' in response to a
 * query --- either the implementation does not support the query, or it cannot
 * determine the value at the current time.  The validity flag is also used by
 * clients in making table queries, to indicate what kinds of table entries
 * should be returned.
 *
 * `OSKIT_MIB_DEFSTRUCT' works only for `oskit_*_t' types.
 */
#define OSKIT_MIB_DEFSTRUCT(slot_type)					\
	typedef struct oskit_mib_##slot_type oskit_mib_##slot_type##_t;	\
	struct oskit_mib_##slot_type {					\
		oskit_bool_t		valid;				\
		oskit_##slot_type##_t	value;				\
	};								\
	OSKIT_MIB_DEFSTRUCT_DEFTESTER(slot_type)			\
	OSKIT_MIB_DEFSTRUCT_DEFGETTER(slot_type)			\
	OSKIT_MIB_DEFSTRUCT_DEFSETTER(slot_type)			\
	OSKIT_MIB_DEFSTRUCT_DEFCLEARER(slot_type)

#define OSKIT_MIB_DEFSTRUCT_DEFTESTER(slot_type)			\
	static inline oskit_bool_t					\
	oskit_mib_##slot_type##_test(oskit_mib_##slot_type##_t *om)	\
	{								\
		return om->valid;					\
	}
#define OSKIT_MIB_DEFSTRUCT_DEFGETTER(slot_type)			\
	static inline oskit_##slot_type##_t				\
	oskit_mib_##slot_type##_get(oskit_mib_##slot_type##_t *om)	\
	{								\
		assert(om->valid);					\
		return om->value;					\
	}
#define OSKIT_MIB_DEFSTRUCT_DEFSETTER(slot_type)			\
	static inline oskit_##slot_type##_t				\
	oskit_mib_##slot_type##_set(					\
		oskit_mib_##slot_type##_t *om, oskit_##slot_type##_t v)	\
	{								\
		om->value = v;						\
		om->valid = 1;						\
		return v;						\
	}
#define OSKIT_MIB_DEFSTRUCT_DEFCLEARER(slot_type)			\
	static inline void						\
	oskit_mib_##slot_type##_clear(oskit_mib_##slot_type##_t *om)	\
	{								\
		om->valid = 0;						\
	}

OSKIT_MIB_DEFSTRUCT(bool)
OSKIT_MIB_DEFSTRUCT(s8)
OSKIT_MIB_DEFSTRUCT(u8)
OSKIT_MIB_DEFSTRUCT(s16)
OSKIT_MIB_DEFSTRUCT(u16)
OSKIT_MIB_DEFSTRUCT(s32)
OSKIT_MIB_DEFSTRUCT(u32)
OSKIT_MIB_DEFSTRUCT(s64)
OSKIT_MIB_DEFSTRUCT(u64)
OSKIT_MIB_DEFSTRUCT(f32)
OSKIT_MIB_DEFSTRUCT(f64)
OSKIT_MIB_DEFSTRUCT(mib_string)
OSKIT_MIB_DEFSTRUCT(mib_in_addr)
/*
 * XXX --- What other ``primitive'' types do we need to support?  Possibilities
 * include the following:
 *
 * char (signed, unsigned)
 * oskit_addr_t
 * oskit_size_t, oskit_ssize_t
 * IP port number
 * SNMP OID
 * SNMP counter, gauge
 */


/*****************************************************************************/
/*
 * Macros that help one define specific interface operations.
 */
     
/*
 * Atomic slots.  An atomic slot is an instance of one of the types defined by
 * `OSKIT_MIB_DEFSTRUCT'.  An atomic slot can be read-write or read-only.
 *
 * The `OPS' macros are used when defining the `ops' struct that corresponds to
 * the COM interface type.  The `IMPLS' macros are used when defining instances
 * of the `ops' interface type, i.e., the implementation.
 */
#define OSKIT_MIB_SLOT_OPS_RW(intf_type, slot_type, slot_name)		\
	OSKIT_MIB_SLOT_OPS_GETTER(intf_type, slot_type, slot_name)	\
	OSKIT_MIB_SLOT_OPS_SETTER(intf_type, slot_type, slot_name)
#define OSKIT_MIB_SLOT_OPS_RO(intf_type, slot_type, slot_name)		\
	OSKIT_MIB_SLOT_OPS_GETTER(intf_type, slot_type, slot_name)

#define OSKIT_MIB_SLOT_OPS_GETTER(intf_type, slot_type, slot_name)	  \
	OSKIT_COMDECL (*get_##slot_name)(intf_type *intf,		  \
					 oskit_mib_##slot_type##_t *val);
#define OSKIT_MIB_SLOT_OPS_SETTER(intf_type, slot_type, slot_name)	\
	OSKIT_COMDECL (*set_##slot_name)(intf_type *intf,		\
					 oskit_##slot_type##_t *val);

#define OSKIT_MIB_SLOT_IMPLS_RW(getter, setter) \
	(getter), (setter)
#define OSKIT_MIB_SLOT_IMPLS_RO(getter) \
	(getter)

/*
 * Slot sets.  A slot set is a structure whose members are atomic slots.
 *
 * The only difference with the above macros is in how we handle the slotset
 * type.  In the single-slot case, we impose the `oskit_mib_*_t' convention on
 * the getter and accept a normal value on the setter.  For a slotset, we
 * assume that the interface defines its own structure that contains
 * `oskit_mib_*_t' atoms, so we don't munge the `slotset_type' name on the
 * setter or getter.
 */
#define OSKIT_MIB_SLOTSET_OPS_RW(intf_type, slotset_type, slotset_name)	\
	OSKIT_MIB_SLOTSET_OPS_GETTER(intf_type,				\
				     slotset_type, slotset_name)	\
	OSKIT_MIB_SLOTSET_OPS_SETTER(intf_type,				\
				     slotset_type, slotset_name)
#define OSKIT_MIB_SLOTSET_OPS_RO(intf_type, slotset_type, slotset_name)	\
	OSKIT_MIB_SLOTSET_OPS_GETTER(intf_type,				\
				     slotset_type, slotset_name)

#define OSKIT_MIB_SLOTSET_OPS_GETTER(intf_type,				\
				     slotset_type, slotset_name)	\
	OSKIT_COMDECL (*get_##slotset_name)(intf_type *intf,		\
					    slotset_type *vals);
#define OSKIT_MIB_SLOTSET_OPS_SETTER(intf_type,				\
				     slotset_type, slotset_name)	\
	OSKIT_COMDECL (*set_##slotset_name)(intf_type *intf,		\
					    slotset_type *vals);

#define OSKIT_MIB_SLOTSET_IMPLS_RW(getter, setter) \
	(getter), (setter)
#define OSKIT_MIB_SLOTSET_IMPLS_RO(getter) \
	(getter)
     
/*
 * Slot set tables.  (There is no point in defining atomic slot tables.)  Now
 * things get complicated.  XXX --- This is work in progress.
 *
 * We can get and set many slots in many rows of a table at one time.
 *
 * get ---
 *
 *  `start_row' says how many matches to skip.  Would ususally be zero unless
 *  we are doing a second or subsequent get on the table, because we were told
 *  that there were more rows than we could get on a previous `get'.
 *
 *  `want_rows': how many rows are in the output table.
 *
 *  `matching': a pattern describing the rows that should be returned in the
 *  table.  Set slots in the slotset are compared (with ==) to rows in the
 *  table; if all set slots match, then the row is copied to the output table.
 *  Unset slots in the `matching' slotset are not considered when matching,
 *  i.e., they represent ``don't care'' values.  If `matching' is null, then
 *  that is the same as a slotset with no set slots, i.e., all rows will match.
 *
 *  `table': pointer to the array of slotsets.
 *
 *  `out_rows': how many rows are filled in the output table.  Zero or more.
 *
 *  `more_rows': set if there may be more matching rows in the table, that
 *  (presumably) didn't fit in the specified `table'.  If `more_rows' is true,
 *  that is not a guarantee that there are more rows: only that there may be.
 *  (Should we require that `more_rows' can be set only if the output table was
 *  filled?  Or can we set `more_rows' even if the function didn't actually
 *  fill the table for some reason?)
 *
 * set ---
 *
 *  `matching': specifies the ``key'' to be used to match rows in the table.
 *  Unset slots in the slotset are not considered when matching.  If `matching'
 *  is null, then all rows are treated.
 *
 *  `all_matches': if false, process only the first matching row.  (It is
 *  assumed that `matching' is a key, or the caller doesn't care which match is
 *  processesd.)  If true, process all matching rows.
 *
 *  `val': the values to be set in every matching row.  Unset slots in the
 *  `val' slotset represent values not to be set in the actual table.  Not all
 *  columns in a table are necessarily writeable; a `val' that attempts to set
 *  an unwriteable column will cause an error before any table rows are
 *  changed.  Error signalled through return value.
 */
#define OSKIT_MIB_SLOTSET_TABLE_OPS_RW(intf_type,		\
				       slotset_type,		\
				       slotset_name)		\
	OSKIT_MIB_SLOTSET_TABLE_OPS_GETTER(intf_type,		\
					   slotset_type,	\
					   slotset_name)	\
	OSKIT_MIB_SLOTSET_TABLE_OPS_SETTER(intf_type,		\
					   slotset_type,	\
					   slotset_name)
#define OSKIT_MIB_SLOTSET_TABLE_OPS_RO(intf_type,		\
				       slotset_type,		\
				       slotset_name)		\
	OSKIT_MIB_SLOTSET_TABLE_OPS_GETTER(intf_type,		\
					   slotset_type,	\
					   slotset_name)

#define OSKIT_MIB_SLOTSET_TABLE_OPS_GETTER(intf_type,		\
					   slotset_type,	\
					   slotset_name)	\
	OSKIT_COMDECL (*get_##slotset_name##_table)(		\
		intf_type *			intf,		\
		oskit_u32_t			start_row,	\
		oskit_u32_t			want_rows,	\
		slotset_type *			matching,	\
		/* OUT */ slotset_type *	table,		\
		/* OUT */ oskit_u32_t *		out_rows,	\
		/* OUT */ oskit_bool_t *	more_rows);
#define OSKIT_MIB_SLOTSET_TABLE_OPS_SETTER(intf_type,		\
					   slotset_type,	\
					   slotset_name)	\
	OSKIT_COMDECL (*set_##slotset_name##_table)(		\
		intf_type *			intf,		\
		slotset_type *			matching,	\
		oskit_bool_t			all_matches,	\
		slotset_type *			vals);
     
#define OSKIT_MIB_SLOTSET_TABLE_IMPLS_RW(getter, setter) \
	(getter), (setter)
#define OSKIT_MIB_SLOTSET_TABLE_IMPLS_RO(getter) \
	(getter)
     

/*****************************************************************************/

#endif /* _OSKIT_MIB_H_ */

/* End of file. */

