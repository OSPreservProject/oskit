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
 * Initialize the security server by reading the policy
 * database and initializing the SID table.
 */

#include "policydb.h"
#include "services.h"

#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/net/socket.h>
#include <oskit/flask/avc_ss.h>
#include <oskit/flask/security.h>
#include <oskit/c/stdarg.h>
 
typedef struct avc_node {
        oskit_avc_ss_t *avc;
        ebitmap_t classes;
        struct avc_node *next;
} avc_node_t;
 
typedef struct {
        oskit_security_t securityi;
        unsigned count;
        oskit_osenv_mem_t *mem;
        oskit_osenv_log_t *log;
        avc_node_t *avc_list;
} security_server_t;
 
static security_server_t ss;
  
static struct oskit_security_ops ss_ops;
 
 
int oskit_security_avc_ss_reset(oskit_u32_t seqno)
{
        avc_node_t *cur;
 
        for (cur = ss.avc_list; cur; cur = cur->next)
                oskit_avc_ss_reset(cur->avc, seqno);
        return 0;
}
        
      
void *oskit_security_malloc(unsigned size)
{               
        return oskit_osenv_mem_alloc(ss.mem, size, OSENV_AUTO_SIZE, 0);
}
                
#undef free
void oskit_security_free(void *addr) 
{       
        return oskit_osenv_mem_free(ss.mem, addr, OSENV_AUTO_SIZE, 0);
}
 
 
int oskit_security_printf(const char * fmt, ...)
{
        va_list ap;
 
        va_start(ap, fmt);
        oskit_osenv_log_vlog(ss.log, OSENV_LOG_INFO, fmt, ap);
        va_end(ap);
        return 0;
}
 
 
void oskit_security_panic(const char * fmt, ...)
{
        va_list ap;
        
        va_start(ap, fmt);
        oskit_osenv_log_vpanic(ss.log, fmt, ap);
        va_end(ap);
}


oskit_error_t
oskit_security_init(oskit_services_t *osenv,
                    struct oskit_openfile *f,
                    oskit_security_t **out_security)
{
        oskit_services_lookup_first(osenv, &oskit_osenv_log_iid,
                                    (void **) &ss.log);
        if (!ss.log)
                return OSKIT_EINVAL;
        oskit_osenv_log_addref(ss.log);
 
        oskit_services_lookup_first(osenv, &oskit_osenv_mem_iid,
                                    (void **) &ss.mem);
        if (!ss.mem)
                return OSKIT_EINVAL;
        oskit_osenv_mem_addref(ss.mem);
        
        ss.securityi.ops = &ss_ops;
        ss.count = 1;
        ss.avc_list = NULL;
 
        *out_security = &ss.securityi; 

	printf("security:  starting up (compiled " __DATE__ ")\n");

	if (policydb_read(&policydb, f)) 
		return OSKIT_EINVAL;

	if (policydb_load_isids(&policydb, &sidtab)) {
		policydb_destroy(&policydb);
		return OSKIT_EINVAL;
	}

	ss_initialized = 1;

	return 0;
}


static OSKIT_COMDECL ss_query(oskit_security_t *s,
                              const struct oskit_guid *iid,
                              void **out_ihandle)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_security_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &ss.securityi;
                ++ss.count;
                return 0;
        }
 
        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}
 
 
static OSKIT_COMDECL_U ss_addref(oskit_security_t *s)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return ++ss.count;
}
 
 
static OSKIT_COMDECL_U ss_release(oskit_security_t *s)
{
        oskit_osenv_mem_t *mem;
        avc_node_t *node, *tmp; 
        unsigned newcount;      
                        
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
                                
        newcount = --ss.count;  
        if (newcount == 0) { 
                /*              
                 * This is not complete.  Still need to add
                 * support for cleaning up the sidtab. 
                 */             
                policydb_destroy(&policydb);
                oskit_osenv_log_release(ss.log);
                mem = ss.mem;
                node = ss.avc_list;
                while (node) {
                        tmp = node;
                        node = node->next;
                        oskit_avc_ss_release(tmp->avc);
                        ebitmap_destroy(&tmp->classes);
                        oskit_osenv_mem_free(mem, tmp, 0,
                                             sizeof(struct avc_node));
                }
                oskit_osenv_mem_release(mem);
        }
 
        return newcount;
}

static OSKIT_COMDECL ss_compute_av(oskit_security_t *s,
                                   oskit_security_id_t ssid,
                                   oskit_security_id_t tsid,
                                   oskit_security_class_t tclass,
                                   oskit_access_vector_t requested,
                                   oskit_access_vector_t *allowed,
                                   oskit_access_vector_t *decided,
                                   oskit_access_vector_t *auditallow,
                                   oskit_access_vector_t *auditdeny,
				   oskit_access_vector_t *notify,
                                   oskit_u32_t *seqno)
{               
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_compute_av(ssid,tsid,tclass,requested,
                                   allowed,decided,auditallow,auditdeny,notify,
                                   seqno);
}
 

static OSKIT_COMDECL ss_notify_perm(oskit_security_t *s,
                                   oskit_security_id_t ssid,
                                   oskit_security_id_t tsid,
                                   oskit_security_class_t tclass,
                                   oskit_access_vector_t requested)
{               
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_notify_perm(ssid,tsid,tclass,requested);
}
 
 
static OSKIT_COMDECL ss_transition_sid(oskit_security_t *s,
                                       oskit_security_id_t ssid,
                                       oskit_security_id_t tsid,
                                       oskit_security_class_t tclass,
                                       oskit_security_id_t *out_sid)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_transition_sid(ssid,tsid,tclass,out_sid);
}
        
 
static OSKIT_COMDECL ss_member_sid(oskit_security_t *s,
                                       oskit_security_id_t ssid,
                                       oskit_security_id_t tsid,
                                       oskit_security_class_t tclass,
                                       oskit_security_id_t *out_sid)
{       
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
                 
        return security_member_sid(ssid,tsid,tclass,out_sid);
}                
                
            
static OSKIT_COMDECL ss_sid_to_context(oskit_security_t *s,
                                       oskit_security_id_t sid,
                                       oskit_security_context_t *scontext,
                                       oskit_u32_t *scontext_len)
{                       
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
                        
        return security_sid_to_context(sid,scontext,scontext_len);
}               
                
 
static OSKIT_COMDECL ss_context_to_sid(oskit_security_t *s,
                                       oskit_security_context_t scontext,
                                       oskit_u32_t scontext_len,
                                       oskit_security_id_t *out_sid)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_context_to_sid(scontext,scontext_len,out_sid);
}                                  
                                   
 
static OSKIT_COMDECL ss_register_avc(oskit_security_t *s,
                                     oskit_security_class_t *classes,
                                     oskit_u32_t nclasses,
                                     struct oskit_avc_ss *avc)
{                
        struct avc_node *node;
        int i;   
                
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
                                       
        node = oskit_osenv_mem_alloc(ss.mem,sizeof(struct avc_node),0,0); 
        if  (!node)                    
                return OSKIT_ENOMEM;
        node->avc = avc;
        oskit_avc_ss_addref(avc);
        ebitmap_init(&node->classes);
        for (i = 0; i < nclasses; i++) {
                ebitmap_set_bit(&node->classes, classes[i], TRUE);
        }       
 
        node->next = ss.avc_list;
        ss.avc_list = node;
        return 0;
}
 

static OSKIT_COMDECL ss_unregister_avc(oskit_security_t *s,
                                       struct oskit_avc_ss *avc)
{
        struct avc_node *prev, *cur;
                                   
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
                                        
        prev = NULL;                    
        cur = ss.avc_list;
        while (cur) {                   
                if (cur->avc == avc)    
                        break;
                prev = cur;
                cur = cur->next;
        }
 
        if (!cur)
                return OSKIT_ENOENT;
 
        if (prev)                      
                prev->next = cur->next;
        else                            
                ss.avc_list = cur->next;
 
        oskit_avc_ss_release(cur->avc); 
        ebitmap_destroy(&cur->classes); 
        oskit_osenv_mem_free(ss.mem, cur, 0, sizeof(struct avc_node));
        return 0;
}
        
 
static OSKIT_COMDECL ss_load_policy(oskit_security_t *s,
                                    struct oskit_openfile *f)
{                                      
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
                                     
        return security_load_policy(f);
}       


static OSKIT_COMDECL ss_fs_sid(oskit_security_t *s,
                               char *name,
                                oskit_security_id_t *fs_sid,
                                oskit_security_id_t *file_sid)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_fs_sid(name,fs_sid,file_sid);
}


static OSKIT_COMDECL ss_port_sid(oskit_security_t *s,
                               oskit_u16_t domain,
                                oskit_u16_t type,
                                oskit_u8_t protocol,
                                oskit_u16_t port,
                                oskit_security_id_t *sid)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_port_sid(domain,type,protocol,port,sid);
}


static OSKIT_COMDECL ss_netif_sid(oskit_security_t *s,
                               char *name,
                                oskit_security_id_t *if_sid,
                                oskit_security_id_t *msg_sid)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;
 
        return security_netif_sid(name,if_sid,msg_sid);
}


static OSKIT_COMDECL ss_node_sid(oskit_security_t *s, 
                                oskit_u16_t domain,
                                void *addr,
                                oskit_u32_t addrlen,
                                oskit_security_id_t *sid)
{
        if (s != ((oskit_security_t *) &ss))
                return OSKIT_EINVAL;

        return security_node_sid(domain,addr,addrlen,sid);
}
                             

static struct oskit_security_ops ss_ops = {
        ss_query,
        ss_addref,
        ss_release,
        ss_compute_av,
        ss_notify_perm,
        ss_transition_sid, 
        ss_member_sid,
        ss_sid_to_context,
        ss_context_to_sid,
        ss_register_avc,
        ss_unregister_avc,
        ss_load_policy,
	ss_fs_sid,
	ss_port_sid,
	ss_netif_sid,
	ss_node_sid
};

/* FLASK */
