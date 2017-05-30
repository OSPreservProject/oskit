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

%{
#include "y.tab.h"

static unsigned int lineno = 1;

unsigned int policydb_errors = 0;
%}
%array
letter  [A-Za-z]
digit   [0-9]
%%
COMMON |
common				{ return(COMMON); }
CLASS |
class				{ return(CLASS); }
CONSTRAIN |
constrain			{ return(CONSTRAIN); }
INHERITS |
inherits			{ return(INHERITS); }
SID |
sid				{ return(SID); }
ROLE |
role				{ return(ROLE); }
ROLES |
roles				{ return(ROLES); }
TYPES |
types				{ return(TYPES); }
TYPE |
type				{ return(TYPE); }
ALIAS |
alias				{ return(ALIAS); }
TYPE_TRANSITION |
type_transition			{ return(TYPE_TRANSITION); }
SENSITIVITY |
sensitivity		{ return(SENSITIVITY); }
DOMINANCE |
dominance			{ return(DOMINANCE); }
CATEGORY |
category			{ return(CATEGORY); }
LEVEL |
level				{ return(LEVEL); }
RANGES |
ranges				{ return(RANGES); }
USER |
user				{ return(USER); }
DEFAULT |
default		        { return(DEFAULT); }
EXCLUDE |
exclude		        { return(EXCLUDE); }
ALLOW |
allow			        { return(ALLOW); }
AUDITALLOW |
auditallow		        { return(AUDITALLOW); }
AUDITDENY |
auditdeny		        { return(AUDITDENY); }
NOTIFY |
notify		                { return(NOTIFY); }
SOURCE |
source			        { return(SOURCE); }
TARGET |
target			        { return(TARGET); }
SAMEUSER |
sameuser			{ return(SAMEUSER);}
OR |
or     			        { return(OR);}
AND |
and				{ return(AND);}
NOT |
not				{ return(NOT);}
{letter}({letter}|{digit}|_)*	{ return(IDENTIFIER); }
{digit}{digit}*                 { return(NUMBER); }
#[^\n]*                         { /* delete comments */ }
[ \t\f]+			{ /* delete whitespace */ }
\n				{ /* delete whitespace */ lineno++; }
"," |
":" |
";" |
"(" | 
")" |
"{" | 
"}" |
"[" |
"-" |
"." |
"]"				{ return(yytext[0]); } 
%%
int yyerror(char *msg)
{
	fprintf(stderr, "\nerror in the statement ending on line %d (token '%s'): %s\n\n", lineno, yytext, msg);
	policydb_errors++;
	return -1;
}

