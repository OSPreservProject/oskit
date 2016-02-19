/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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
 * COM interface for locks. Create and register the lock manager at init
 * time. The lock interface then operates on locks created with the lock
 * manager.
 */

#include <threads/pthread_internal.h>
#include <oskit/com/lock_mgr.h>
#include <oskit/com/lock.h>
#include <oskit/com/condvar.h>
#include <oskit/c/string.h>

/*
 * An exportable lock. Actually a mutex.
 */
typedef struct lockimpl {
	oskit_lock_t	lock;
	pthread_mutex_t mutex;
	int		count;
	int		critical;
	int		interrupts_enabled;
} lockimpl_t;

static OSKIT_COMDECL 
lock_query(oskit_lock_t *intf, const oskit_iid_t *iid, void **out_ihandle)
{
	lockimpl_t	*l = (lockimpl_t *) intf;
	
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_lock_iid, sizeof(*iid)) == 0) {
                *out_ihandle = intf;
		++l->count;
                return 0; 
        }
  
        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
lock_addref(oskit_lock_t *intf)
{
	lockimpl_t	*l = (lockimpl_t *) intf;

        if (l == NULL)
                panic("lock_addref: NULL lock");
	
        if (l->count == 0)
                panic("lock_addref: Zero count for 0x%x", l);
	
        return ++l->count;
}

static OSKIT_COMDECL_U
lock_release(oskit_lock_t *intf)
{
	lockimpl_t	*l = (lockimpl_t *) intf;
	int		newcount;

        if (l == NULL)
                panic("lock_release: NULL lock");
	
        if (l->count == 0)
                panic("lock_release: Zero count for 0x%x", l);

        if ((newcount = --l->count) == 0) {
		osenv_mem_free(l, 0, sizeof(*l));
	}

	return newcount;
}

static OSKIT_COMDECL
lock_lock(oskit_lock_t *intf)
{
	lockimpl_t	*l = (lockimpl_t *) intf;

#ifdef	THREADS_DEBUG
        if (l == NULL)
                panic("lock_lock: NULL lock");
	
        if (l->count == 0)
                panic("lock_lock: Zero count for 0x%x", l);
#endif
	if (l->critical)
		l->interrupts_enabled = save_disable_interrupts();
	
	pthread_mutex_lock(&l->mutex);

	return 0;
}

static OSKIT_COMDECL
lock_unlock(oskit_lock_t *intf)
{
	lockimpl_t	*l = (lockimpl_t *) intf;

#ifdef	THREADS_DEBUG
        if (l == NULL)
                panic("lock_unlock: NULL lock");
	
        if (l->count == 0)
                panic("lock_unlock: Zero count for 0x%x", l);
#endif
	pthread_mutex_unlock(&l->mutex);

	if (l->critical)
		restore_interrupt_enable(l->interrupts_enabled);
	
	return 0;
}

static struct oskit_lock_ops lock_ops = {
	lock_query,
	lock_addref,
	lock_release,
	lock_lock,
	lock_unlock,
};

static lockimpl_t *
create_lockimpl(int critical)
{
        lockimpl_t	*l = osenv_mem_alloc(sizeof(*l), 0, 0);
	
        if (l == NULL)
		panic("create_lockimpl: NULL allocation");
	
        memset(l, 0, sizeof(*l));

	l->critical = critical;
        l->count    = 1;
        l->lock.ops = &lock_ops;
	pthread_mutex_init(&l->mutex, 0);

	return l;
}

/*
 * This the condvar interface.
 */
typedef struct cvarimpl {
	oskit_condvar_t	condi;
	pthread_cond_t  condition;
	int		count;
} cvarimpl_t;

static OSKIT_COMDECL 
condvar_query(oskit_condvar_t *intf,
	      const oskit_iid_t *iid, void **out_ihandle)
{
	cvarimpl_t	*c = (cvarimpl_t *) intf;
	
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_condvar_iid, sizeof(*iid)) == 0) {
                *out_ihandle = intf;
		++c->count;
                return 0; 
        }
  
        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
condvar_addref(oskit_condvar_t *intf)
{
	cvarimpl_t	*c = (cvarimpl_t *) intf;

        if (c == NULL)
                panic("condvar_addref: NULL condvar");
	
        if (c->count == 0)
                panic("condvar_addref: Zero count for 0x%x", c);
	
        return ++c->count;
}

static OSKIT_COMDECL_U
condvar_release(oskit_condvar_t *intf)
{
	cvarimpl_t	*c = (cvarimpl_t *) intf;
	int		newcount;

        if (c == NULL)
                panic("condvar_release: NULL condvar");
	
        if (c->count == 0)
                panic("condvar_release: Zero count for 0x%x", c);
	
        if ((newcount = --c->count) == 0) {
		osenv_mem_free(c, 0, sizeof(*c));
	}

	return newcount;
}

static OSKIT_COMDECL
condvar_wait(oskit_condvar_t *cintf, oskit_lock_t *lintf)
{
	cvarimpl_t	*c = (cvarimpl_t *) cintf;
	lockimpl_t	*l = (lockimpl_t *) lintf;

#ifdef	THREADS_DEBUG
        if (c == NULL)
                panic("condvar_wait: NULL condvar");
	
        if (c->count == 0)
                panic("condvar_wait: Zero count for condvar 0x%x", c);

        if (l == NULL)
                panic("condvar_wait: NULL lock");
	
        if (l->count == 0)
                panic("condvar_wait: Zero count for lock 0x%x", l);
#endif
	pthread_cond_wait(&c->condition, &l->mutex);

	return 0;
}

static OSKIT_COMDECL
condvar_signal(oskit_condvar_t *intf)
{
	cvarimpl_t	*c = (cvarimpl_t *) intf;

#ifdef	THREADS_DEBUG
        if (c == NULL)
                panic("condvar_signal: NULL condvar");
	
        if (c->count == 0)
                panic("condvar_signal: Zero count for 0x%x", c);
#endif
	pthread_cond_signal(&c->condition);

	return 0;
}

static OSKIT_COMDECL
condvar_broadcast(oskit_condvar_t *intf)
{
	cvarimpl_t	*c = (cvarimpl_t *) intf;

#ifdef	THREADS_DEBUG
        if (c == NULL)
                panic("condvar_broadcast: NULL condvar");
	
        if (c->count == 0)
                panic("condvar_broadcast: Zero count for 0x%x", c);
#endif
	pthread_cond_broadcast(&c->condition);

	return 0;
}

static struct oskit_condvar_ops condvar_ops = {
	condvar_query,
	condvar_addref,
	condvar_release,
	condvar_wait,
	condvar_signal,
	condvar_broadcast,
};

static cvarimpl_t *
create_cvarimpl(void)
{
        cvarimpl_t	*c = osenv_mem_alloc(sizeof(*c), 0, 0);
	
        if (c == NULL)
		panic("create_cvarimpl: NULL allocation");
	
        memset(c, 0, sizeof(*c));

        c->count     = 1;
        c->condi.ops = &condvar_ops;
	pthread_cond_init(&c->condition, 0);

	return c;
}

/*
 * This is the lock manager interface.
 */
static oskit_lock_mgr_t	lock_mgr;

static OSKIT_COMDECL 
lock_mgr_query(oskit_lock_mgr_t *intf,
	       const oskit_iid_t *iid, void **out_ihandle)
{ 
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_lock_mgr_iid, sizeof(*iid)) == 0) {
                *out_ihandle = intf;
                return 0; 
        }
  
        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
lock_mgr_addref(oskit_lock_mgr_t *intf)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL_U
lock_mgr_release(oskit_lock_mgr_t *intf)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL
lock_mgr_allocate_lock(oskit_lock_mgr_t *intf, oskit_lock_t **out_lock)
{
	*out_lock = (oskit_lock_t *) create_lockimpl(0);
	
	return 0;
}

static OSKIT_COMDECL
lock_mgr_allocate_critical_lock(oskit_lock_mgr_t *intf,
				oskit_lock_t **out_lock)
{
	*out_lock = (oskit_lock_t *) create_lockimpl(1);
	
	return 0;
}

static OSKIT_COMDECL
lock_mgr_allocate_condvar(oskit_lock_mgr_t *intf,
			  oskit_condvar_t **out_condvar)
{
	*out_condvar = (oskit_condvar_t *) create_cvarimpl();
	
	return 0;
}

static struct oskit_lock_mgr_ops lock_mgr_ops = {
	lock_mgr_query,
	lock_mgr_addref,
	lock_mgr_release,
	lock_mgr_allocate_lock,
	lock_mgr_allocate_critical_lock,
	lock_mgr_allocate_condvar,
};

#ifdef KNIT
oskit_lock_mgr_t *oskit_lock_mgr = &lock_mgr;
#else
/*
 * Create and register the lock manager interface with COM database.
 */
int
pthread_init_comlock(void)
{
	lock_mgr.ops = &lock_mgr_ops;

	return oskit_register(&oskit_lock_mgr_iid, (void *) &lock_mgr);
}
#endif /* !KNIT */
