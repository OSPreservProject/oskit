/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_SYSLOG_H_  
#define _OSKIT_C_SYSLOG_H_  

#include <oskit/compiler.h>
#include <oskit/dev/dev.h>	/* osenv_log */

#define LOG_EMERG       OSENV_LOG_EMERG    /* system is unusable */
#define LOG_ALERT       OSENV_LOG_ALERT    /* action must be taken immediately */
#define LOG_CRIT        OSENV_LOG_CRIT     /* critical conditions */
#define LOG_ERR         OSENV_LOG_ERR      /* error conditions */
#define LOG_WARNING     OSENV_LOG_WARNING  /* warning conditions */
#define LOG_NOTICE      OSENV_LOG_NOTICE   /* normal but significant condition */
#define LOG_INFO        OSENV_LOG_INFO     /* informational */
#define LOG_DEBUG       OSENV_LOG_DEBUG    /* debug-level messages */

/*
 * The rest is temporary - XXX
 * 
 * these facilities codes are just ored in in BSD code. This is presumably
 * not the behavior as defined in CAE. They should be passed in openlog
 */

/* facility codes */    
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */

OSKIT_BEGIN_DECLS

/* XXX */

/* if you want syslog to work */
typedef void (*oskit_syslogger_t)(int, const char *, ...);
extern oskit_syslogger_t	oskit_libc_syslogger;

#define syslog(priority, format...)	oskit_libc_syslogger(priority, ##format) 

OSKIT_END_DECLS

#endif /* _OSKIT_C_SYSLOG_H_ */
