/*
 * Copyright (c) 1998, 1999 University of Utah and the Flux Group.
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
/*	$NetBSD: gprof.c,v 1.8 1995/04/19 07:15:59 cgd Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)gprof.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$NetBSD: gprof.c,v 1.8 1995/04/19 07:15:59 cgd Exp $";
#endif
#endif /* not lint */

#include "gprof.h"
#include <string.h>

#define u_char unsigned char
void dumpsum( char *sumfile );
void alignentries();

char	*whoami = "gprof";

    /*
     *	things which get -E excluded by default.
     */
char	*defaultEs[] = { "mcount" , "_mcount", "__mcount", "__mcleanup" , 0 };

static struct gmonhdr	gmonhdr;


/* XXX fluke stuff */
int
setlinebuf(FILE *stream) 
{
	return EOF;
}

#if 0	/* we have close, right? */
int 
close(int d)
{
	return 0;
}
#endif

void
gprof_main(argc, argv)
    int argc;
    char **argv;
{
    char	**sp;
    nltype	**timesortnlp;

#if 0
    printf("gprof_main:\n");

    printf("in %s\n", __FUNCTION__);
#endif

    --argc;
    argv++;
    debug = 0;
    bflag = TRUE;
    while ( *argv != 0 && **argv == '-' ) {
	(*argv)++;
	switch ( **argv ) {
	case 'a':
	    aflag = TRUE;
	    break;
	case 'b':
	    bflag = FALSE;
	    break;
	case 'C':
	    Cflag = TRUE;
	    cyclethreshold = atoi( *++argv );
	    break;
	case 'c':
#if defined(vax) || defined(tahoe) || defined(sparc)
	    cflag = TRUE;
#else
	    fprintf(stderr, "gprof: -c isn't supported on this architecture yet\n");
	    /* exit(1); */ printf("error!\n");
#endif
	    break;
	case 'd':
	    dflag = TRUE;
	    setlinebuf(stdout);
	    debug |= atoi( *++argv );
	    debug |= ANYDEBUG;
#	    ifdef DEBUG
		printf("[main] debug = %d\n", debug);
#	    else not DEBUG
		printf("%s: -d ignored\n", whoami);
#	    endif DEBUG
	    break;
	case 'E':
	    ++argv;
	    addlist( Elist , *argv );
	    Eflag = TRUE;
	    addlist( elist , *argv );
	    eflag = TRUE;
	    break;
	case 'e':
	    addlist( elist , *++argv );
	    eflag = TRUE;
	    break;
	case 'F':
	    ++argv;
	    addlist( Flist , *argv );
	    Fflag = TRUE;
	    addlist( flist , *argv );
	    fflag = TRUE;
	    break;
	case 'f':
	    addlist( flist , *++argv );
	    fflag = TRUE;
	    break;
	case 'k':
	    addlist( kfromlist , *++argv );
	    addlist( ktolist , *++argv );
	    kflag = TRUE;
	    break;
	case 's':
	    sflag = TRUE;
	    break;
	case 'z':
	    zflag = TRUE;
	    break;
	}
	argv++;
    }
    if ( *argv != 0 ) {
	a_outname  = *argv;
	argv++;
    } else {
	a_outname  = A_OUTNAME;
    }
    if ( *argv != 0 ) {
	gmonname = *argv;
	argv++;
    } else {
	gmonname = GMONNAME;
    }
	/*
	 *	turn off default functions
	 */
    for ( sp = &defaultEs[0] ; *sp ; sp++ ) {
	Eflag = TRUE;
	addlist( Elist , *sp );
	eflag = TRUE;
	addlist( elist , *sp );
    }


	/*
	 *	get information about a.out file.
	 */
    getnfile();


	/*
	 *	get information about mon.out file(s).
	 */
    do	{
	getpfile( gmonname );
	if ( *argv != 0 ) {
	    gmonname = *argv;
	}
    } while ( *argv++ != 0 );
	/*
	 *	how many ticks per second?
	 *	if we can't tell, report time in ticks.
	 */
    if (hz == 0) {
	hz = 1;
	fprintf(stderr, "time is in ticks, not seconds\n");
    }
	/*
	 *	dump out a gmon.sum file if requested
	 */
    if ( sflag ) {
	dumpsum( GMONSUM );
    }
	/*
	 *	assign samples to procedures
	 */
    asgnsamples();
	/*
	 *	assemble the dynamic profile
	 */
    timesortnlp = doarcs();
	/*
	 *	print the dynamic profile
	 */
    printgprof( timesortnlp );	
	/*
	 *	print the flat profile
	 */
    printprof();	
	/*
	 *	print the index
	 */
    printindex();	
    done();
}

    /*
     * Set up string and symbol tables from a.out.
     *	and optionally the text space.
     * On return symbol table is sorted by value.
     */
void
getnfile()
{
    FILE	*nfile;
    int		valcmp();

    nfile = fopen( a_outname ,"r" );
    if (nfile == NULL) {
	char buf[256];
	
	/* Try bmod root */
	strcpy(buf, "/bmod/");
	strcat(buf, a_outname);
	nfile = fopen( buf , "r" );
	if (nfile == NULL) {
		perror( a_outname );
		done();
	}
    }
    fread( &xbuf, 1, sizeof(xbuf), nfile );
#ifdef ELF
    if (xbuf.e_ident[0] != 0x7f || strncmp(&xbuf.e_ident[1], "ELF", 3)) {
	fprintf( stderr, "%s: %s: not ELF format!\n", whoami , a_outname );
	done();	    
    }
    
    stable = (Elf32_Shdr *)malloc( xbuf.e_shnum * xbuf.e_shentsize );
    if ( stable == NULL ) {
        fprintf( stderr, "%s: %s: can't allocate section table!\n",
		whoami, a_outname );
        done();
    }
    fseek( nfile, xbuf.e_shoff, SEEK_SET );
    fread( stable, xbuf.e_shentsize, xbuf.e_shnum, nfile );
#else
    if (N_BADMAG( xbuf )) {
	fprintf( stderr, "%s: %s: bad format\n", whoami , a_outname );
	done();
    }
#endif
    getstrtab( nfile );
    getsymtab( nfile );
    gettextspace( nfile );
    qsort( nl, nname, sizeof(nltype), valcmp );
    fclose( nfile );
#   ifdef DEBUG
	if ( debug & AOUTDEBUG ) {
	    register int j;

	    for ( j = 0; j < nname; j++ ){
		printf( "[getnfile] 0X%08x\t%s\n", nl[j].value, nl[j].name );
	    }
	}
#   endif DEBUG
}

#ifdef ELF
void
getstrtab(nfile)
    FILE	*nfile;
{
    Elf32_Shdr *sp;
    int i;

    sp = stable;
    for ( i = xbuf.e_shnum; i; i--, sp++ ) {
	    if ( sp->sh_type == SHT_DYNSYM ) {
	        fprintf( stderr, "%s: %s: can't handle dynamic symbols yet!\n" ,
			whoami , a_outname );
	    }
	    if ( sp->sh_type == SHT_SYMTAB )
		    break;
    }

    if ( sp == NULL || sp->sh_type != SHT_SYMTAB ) {
        fprintf( stderr, "%s: %s: can't find symbol table!\n" ,
                whoami , a_outname );
    }

    sp = &stable[sp->sh_link];

    if ( sp == NULL || sp->sh_type != SHT_STRTAB ) {
        fprintf( stderr, "%s: %s: can't find string table corresponding to symbol table!\n" ,
		whoami , a_outname );
	done();
    }

    fseek( nfile, sp->sh_offset, SEEK_SET );

    strtab = calloc( sp->sh_size, 1 );
    if ( strtab == NULL ) {
	fprintf( stderr, "%s: %s: no room for %d bytes of string table\n",
		whoami , a_outname , sp->sh_size );
	done();
    }

    if ( fread(strtab, sp->sh_size, 1, nfile) != 1 ) {
	fprintf( stderr, "%s: %s: error reading string table\n",
		whoami , a_outname );
	done();
    }
}

    /*
     * Read in symbol table
     */
void
getsymtab(nfile)
    FILE	*nfile;
{
    register long	i;
    int			askfor;
    Elf32_Sym 	nbuf;
    Elf32_Shdr 	*sp;

    sp = stable;
    for ( i = xbuf.e_shnum; i; i--, sp++ ) {
	if ( sp->sh_type == SHT_DYNSYM ) {
                fprintf( stderr, "%s: %s: can't handle dynamic symbols yet!\n" ,
                        whoami , a_outname );
	}
            if (sp->sh_type == SHT_SYMTAB)
                    break;
    }

    if (sp == NULL || sp->sh_type != SHT_SYMTAB) {
        fprintf( stderr, "%s: %s: no symbol table (old format?)\n" ,
                whoami , a_outname );
    }
    nname = sp->sh_size / sp->sh_entsize;
    
    if (nname == 0) {
	fprintf( stderr, "%s: %s: no symbols\n", whoami , a_outname );
	done();
    }

    askfor = nname + 1;
    nl = (nltype *) calloc( askfor , sizeof(nltype) );
    if (nl == 0) {
	fprintf( stderr, "%s: No room for %d bytes of symbol table\n",
		whoami, askfor * sizeof(nltype) );
	done();
    }

    /* pass2 - read symbols */
    fseek( nfile, sp->sh_offset, SEEK_SET );

    npe = nl;
    nname = 0;
    for ( i = sp->sh_size; i > 0; i -= sp->sh_entsize ) {
	fread( &nbuf, sizeof(nbuf), 1, nfile );
	if ( ! funcsymbol( &nbuf ) ) {
#	    ifdef DEBUG
		if ( debug & AOUTDEBUG ) {
		    printf( "[getsymtab] rejecting: 0x%x %s\n" ,
			    ELF32_ST_TYPE(nbuf.st_info), 
			    strtab + nbuf.st_name );
		}
#	    endif DEBUG
	    continue;
	}
	npe->value = nbuf.st_value;
	npe->name = strtab + nbuf.st_name;

#	ifdef DEBUG
	    if ( debug & AOUTDEBUG ) {
		printf( "[getsymtab] %d %s 0x%08x\n" ,
			nname , npe -> name , npe -> value );
	    }
#	endif DEBUG
	npe++;
	nname++;
    }
    npe->value = -1;
}

    /*
     *	read in the text space of an ELF file
     */
void
gettextspace( nfile )
    FILE	*nfile;
{
    Elf32_Shdr *sp;


    /* XXX This is probably the wrong thing to do, there can probably
     * be more than one SHT_PROGBITS section around. =(
     */
    for ( sp = stable; sp->sh_type != SHT_PROGBITS; sp++ );

    if ( sp == NULL || sp->sh_type != SHT_PROGBITS ) {
        fprintf( stderr, "%s: %s: no ELF text section!\n" ,
                whoami , a_outname );
    }

    if ( cflag == 0 ) {
	return;
    }
    textspace = (u_char *) malloc( (sp->sh_size + 3) & ~0xff );
    if ( textspace == 0 ) {
	fprintf( stderr , "%s: ran out room for %d bytes of text space:  " ,
			whoami , sp->sh_size );
	fprintf( stderr , "can't do -c\n" );
	return;
    }
    (void) fseek( nfile , sp->sh_offset , 0 );
    if ( fread( textspace, (sp->sh_size + 3) & ~0xff, 1, nfile ) != 1 ) {
	fprintf( stderr , "%s: couldn't read text space:  " , whoami );
	fprintf( stderr , "can't do -c\n" );
	free( textspace );
	textspace = 0;
	return;
    }
}

#else
void
getstrtab(nfile)
    FILE	*nfile;
{

    fseek(nfile, (long)(N_SYMOFF(xbuf) + xbuf.a_syms), 0);
    if (fread(&ssiz, sizeof (ssiz), 1, nfile) == 0) {
	fprintf(stderr, "%s: %s: no string table (old format?)\n" ,
		whoami , a_outname );
	done();
    }
    strtab = calloc(ssiz, 1);
    if (strtab == NULL) {
	fprintf(stderr, "%s: %s: no room for %d bytes of string table\n",
		whoami , a_outname , ssiz);
	done();
    }
    if (fread(strtab+sizeof(ssiz), ssiz-sizeof(ssiz), 1, nfile) != 1) {
	fprintf(stderr, "%s: %s: error reading string table\n",
		whoami , a_outname );
	done();
    }
}

    /*
     * Read in symbol table
     */
void
getsymtab(nfile)
    FILE	*nfile;
{
    register long	i;
    int			askfor;
    struct nlist	nbuf;

    /* pass1 - count symbols */
    fseek(nfile, (long)N_SYMOFF(xbuf), 0);
    nname = 0;
    for (i = xbuf.a_syms; i > 0; i -= sizeof(struct nlist)) {
	fread(&nbuf, sizeof(nbuf), 1, nfile);
	if ( ! funcsymbol( &nbuf ) ) {
	    continue;
	}
	nname++;
    }
    if (nname == 0) {
	fprintf(stderr, "%s: %s: no symbols\n", whoami , a_outname );
	done();
    }
    askfor = nname + 1;
    nl = (nltype *) calloc( askfor , sizeof(nltype) );
    if (nl == 0) {
	fprintf(stderr, "%s: No room for %d bytes of symbol table\n",
		whoami, askfor * sizeof(nltype) );
	done();
    }

    /* pass2 - read symbols */
    fseek(nfile, (long)N_SYMOFF(xbuf), 0);
    npe = nl;
    nname = 0;
    for (i = xbuf.a_syms; i > 0; i -= sizeof(struct nlist)) {
	fread(&nbuf, sizeof(nbuf), 1, nfile);
	if ( ! funcsymbol( &nbuf ) ) {
#	    ifdef DEBUG
		if ( debug & AOUTDEBUG ) {
		    printf( "[getsymtab] rejecting: 0x%x %s\n" ,
			    nbuf.n_type , strtab + nbuf.n_un.n_strx );
		}
#	    endif DEBUG
	    continue;
	}
	npe->value = nbuf.n_value;
	npe->name = strtab+nbuf.n_un.n_strx;
#	ifdef DEBUG
	    if ( debug & AOUTDEBUG ) {
		printf( "[getsymtab] %d %s 0x%08x\n" ,
			nname , npe -> name , npe -> value );
	    }
#	endif DEBUG
	npe++;
	nname++;
    }
    npe->value = -1;
}

    /*
     *	read in the text space of an a.out file
     */
void
gettextspace( nfile )
    FILE	*nfile;
{

    if ( cflag == 0 ) {
	return;
    }
    textspace = (u_char *) malloc( xbuf.a_text );
    if ( textspace == 0 ) {
	fprintf( stderr , "%s: ran out room for %d bytes of text space:  " ,
			whoami , xbuf.a_text );
	fprintf( stderr , "can't do -c\n" );
	return;
    }
    (void) fseek( nfile , N_TXTOFF( xbuf ) , 0 );
    if ( fread( textspace , 1 , xbuf.a_text , nfile ) != xbuf.a_text ) {
	fprintf( stderr , "%s: couldn't read text space:  " , whoami );
	fprintf( stderr , "can't do -c\n" );
	free( textspace );
	textspace = 0;
	return;
    }
}
#endif


    /*
     *	information from a gmon.out file is in two parts:
     *	an array of sampling hits within pc ranges,
     *	and the arcs.
     */
void
getpfile(filename)
    char *filename;
{
    FILE		*pfile;
    FILE		*openpfile();
    struct rawarc	arc;
    int bar = 0;

    pfile = openpfile(filename);
    readsamples(pfile);
	/*
	 *	the rest of the file consists of
	 *	a bunch of <from,self,count> tuples.
	 */
    while ( fread( &arc , sizeof arc , 1 , pfile ) == 1 ) {
#	ifdef DEBUG
	    if ( debug & SAMPLEDEBUG ) {
		printf( "[getpfile] frompc 0x%x selfpc 0x%x count %d\n" ,
			arc.raw_frompc , arc.raw_selfpc , arc.raw_count );
	    }
#	endif DEBUG
	    if (arc.raw_count == 0)
		    bar++;

#if 0
	    /* XXX We don't seem to get EOF in fluke. =( */
	    if (bar > 100)
		    break;
#endif

	    /*
	     *	add this arc
	     */
	tally( &arc );
    }
    fclose(pfile);
}

FILE *
openpfile(filename)
    char *filename;
{
    struct gmonhdr	tmp;
    FILE		*pfile;
    int			size;
    int			rate;

    if((pfile = fopen(filename, "r")) == NULL) {
	perror(filename);
	done();
    }
    fread(&tmp, sizeof(struct gmonhdr), 1, pfile);
    if ( s_highpc != 0 && ( tmp.lpc != gmonhdr.lpc ||
	 tmp.hpc != gmonhdr.hpc || tmp.ncnt != gmonhdr.ncnt ) ) {
	fprintf(stderr, "%s: incompatible with first gmon file\n", filename);
	done();
    }
    gmonhdr = tmp;
    if ( gmonhdr.version == GMONVERSION ) {
	rate = gmonhdr.profrate;
	size = sizeof(struct gmonhdr);
    } else {
	fseek(pfile, sizeof(struct ophdr), SEEK_SET);
	size = sizeof(struct ophdr);
	gmonhdr.profrate = rate = OSKIT_PROFHZ;
	gmonhdr.version = GMONVERSION;
    }
    if (hz == 0) {
	hz = rate;
    } else if (hz != rate) {
	fprintf(stderr,
	    "%s: profile clock rate (%d) %s (%d) in first gmon file\n",
	    filename, rate, "incompatible with clock rate", hz);
	done();
    }
    s_lowpc = (unsigned long) gmonhdr.lpc;
    s_highpc = (unsigned long) gmonhdr.hpc;
    lowpc = (unsigned long)gmonhdr.lpc / sizeof(UNIT);
    highpc = (unsigned long)gmonhdr.hpc / sizeof(UNIT);
    sampbytes = gmonhdr.ncnt - size;
    nsamples = sampbytes / sizeof (UNIT);
    printf("nsamples:  %d  sampbytes:  %d UNIT: %d size: %d\n",
	   nsamples, sampbytes, sizeof(UNIT), size);
#   ifdef DEBUG
	if ( debug & SAMPLEDEBUG ) {
	    printf( "[openpfile] hdr.lpc 0x%x hdr.hpc 0x%x hdr.ncnt %d\n",
		gmonhdr.lpc , gmonhdr.hpc , gmonhdr.ncnt );
	    printf( "[openpfile]   s_lowpc 0x%x   s_highpc 0x%x\n" ,
		s_lowpc , s_highpc );
	    printf( "[openpfile]     lowpc 0x%x     highpc 0x%x\n" ,
		lowpc , highpc );
	    printf( "[openpfile] sampbytes %d nsamples %d\n" ,
		sampbytes , nsamples );
	    printf( "[openpfile] sample rate %d\n" , hz );
	}
#   endif DEBUG
    return(pfile);
}

void
tally( rawp )
    struct rawarc	*rawp;
{
    nltype		*parentp;
    nltype		*childp;

    parentp = nllookup( rawp -> raw_frompc );
    childp = nllookup( rawp -> raw_selfpc );
    if ( parentp == 0 || childp == 0 )
	return;
    if ( kflag
	 && onlist( kfromlist , parentp -> name )
	 && onlist( ktolist , childp -> name ) ) {
	return;
    }
    childp -> ncall += rawp -> raw_count;
#   ifdef DEBUG
	if ( debug & TALLYDEBUG ) {
	    printf( "[tally] arc from %s to %s traversed %d times\n" ,
		    parentp -> name , childp -> name , rawp -> raw_count );
	}
#   endif DEBUG
    addarc( parentp , childp , rawp -> raw_count );
}

/*
 * dump out the gmon.sum file
 */
void
dumpsum( sumfile )
    char *sumfile;
{
    register nltype *nlp;
    register arctype *arcp;
    struct rawarc arc;
    FILE *sfile;

    if ( ( sfile = fopen ( sumfile , "w" ) ) == NULL ) {
	perror( sumfile );
	done();
    }
    /*
     * dump the header; use the last header read in
     */
    if ( fwrite( &gmonhdr , sizeof gmonhdr , 1 , sfile ) != 1 ) {
	perror( sumfile );
	done();
    }
    /*
     * dump the samples
     */
    if (fwrite(samples, sizeof (UNIT), nsamples, sfile) != nsamples) {
	perror( sumfile );
	done();
    }
    /*
     * dump the normalized raw arc information
     */
    for ( nlp = nl ; nlp < npe ; nlp++ ) {
	for ( arcp = nlp -> children ; arcp ; arcp = arcp -> arc_childlist ) {
	    arc.raw_frompc = arcp -> arc_parentp -> value;
	    arc.raw_selfpc = arcp -> arc_childp -> value;
	    arc.raw_count = arcp -> arc_count;
	    if ( fwrite ( &arc , sizeof arc , 1 , sfile ) != 1 ) {
		perror( sumfile );
		done();
	    }
#	    ifdef DEBUG
		if ( debug & SAMPLEDEBUG ) {
		    printf( "[dumpsum] frompc 0x%x selfpc 0x%x count %d\n" ,
			    arc.raw_frompc , arc.raw_selfpc , arc.raw_count );
		}
#	    endif DEBUG
	}
    }
    fclose( sfile );
}

int
valcmp(p1, p2)
    nltype *p1, *p2;
{
    if ( p1 -> value < p2 -> value ) {
	return LESSTHAN;
    }
    if ( p1 -> value > p2 -> value ) {
	return GREATERTHAN;
    }
    return EQUALTO;
}

void
readsamples(pfile)
    FILE	*pfile;
{
    register i;
    UNIT	sample;
    
    if (samples == 0) {
	samples = (UNIT *) calloc(sampbytes, sizeof (UNIT));
	if (samples == 0) {
	    fprintf( stderr , "%s: No room for %d sample pc's\n", 
		whoami , sampbytes / sizeof (UNIT));
	    done();
	}
    }
    
    for (i = 0; i < nsamples; i++) {
#if 0
        fread(&sample, sizeof (UNIT), 1, pfile);
	if (feof(pfile))
#else
	/* XXX This is bad because fread can return < 1 if
	 * we hit eof, or there is an error!.
	 */
        if ( fread(&sample, sizeof (UNIT), 1, pfile) != 1 )
#endif
		break;
	samples[i] += sample;
    }
    if (i != nsamples) {
	fprintf( stderr,
	    "%s: unexpected EOF after reading %d/%d samples\n",
		whoami , --i , nsamples );
	done();
    }
}

/*
 *	Assign samples to the procedures to which they belong.
 *
 *	There are three cases as to where pcl and pch can be
 *	with respect to the routine entry addresses svalue0 and svalue1
 *	as shown in the following diagram.  overlap computes the
 *	distance between the arrows, the fraction of the sample
 *	that is to be credited to the routine which starts at svalue0.
 *
 *	    svalue0                                         svalue1
 *	       |                                               |
 *	       v                                               v
 *
 *	       +-----------------------------------------------+
 *	       |					       |
 *	  |  ->|    |<-		->|         |<-		->|    |<-  |
 *	  |         |		  |         |		  |         |
 *	  +---------+		  +---------+		  +---------+
 *
 *	  ^         ^		  ^         ^		  ^         ^
 *	  |         |		  |         |		  |         |
 *	 pcl       pch		 pcl       pch		 pcl       pch
 *
 *	For the vax we assert that samples will never fall in the first
 *	two bytes of any routine, since that is the entry mask,
 *	thus we give call alignentries() to adjust the entry points if
 *	the entry mask falls in one bucket but the code for the routine
 *	doesn't start until the next bucket.  In conjunction with the
 *	alignment of routine addresses, this should allow us to have
 *	only one sample for every four bytes of text space and never
 *	have any overlap (the two end cases, above).
 */
void
asgnsamples()
{
    register int	j;
    UNIT		ccnt;
    double		time;
    unsigned long	pcl, pch;
    register int	i;
    unsigned long	overlap;
    unsigned long	svalue0, svalue1;

    /* read samples and assign to namelist symbols */
    scale = highpc - lowpc;
    printf("scale start: %lf\n", scale);
    scale /= nsamples;

    printf("Scale:  %lf  nsamples:  %d\n", scale, nsamples);
    printf("lowpc: %d\n", lowpc);
    
    alignentries();
    for (i = 0, j = 1; i < nsamples; i++) {
	ccnt = samples[i];
	if (ccnt == 0)
		continue;
	pcl = lowpc + scale * i;
	pch = lowpc + scale * (i + 1);
	time = ccnt;
#	ifdef DEBUG
	    if ( debug & SAMPLEDEBUG ) {
		printf( "[asgnsamples] pcl 0x%x pch 0x%x ccnt %d\n" ,
			pcl , pch , ccnt );
	    }
#	endif DEBUG

	for (j = j - 1; j < nname; j++) {
	    svalue0 = nl[j].svalue;
	    svalue1 = nl[j+1].svalue;
		/*
		 *	if high end of tick is below entry address, 
		 *	go for next tick.
		 */
	    if (pch < svalue0)
		    break;
		/*
		 *	if low end of tick into next routine,
		 *	go for next routine.
		 */
	    if (pcl >= svalue1)
		    continue;
	    overlap = min(pch, svalue1) - max(pcl, svalue0);
	    if (overlap > 0) {
#		ifdef DEBUG
		    if (debug & SAMPLEDEBUG) {
			printf("[asgnsamples] (0x%x->0x%x-0x%x) %s gets %f ticks %d overlap\n",
				nl[j].value/sizeof(UNIT), svalue0, svalue1,
				nl[j].name, 
				overlap * time / scale, overlap);
		    }
#		endif DEBUG
		nl[j].time += overlap * time / scale;

		if ( onlist( Flist , nl[j].name )
		    || ( !onlist( Elist , nl[j].name ) ) )
			totime += time;

	    }
	}
    }
#   ifdef DEBUG
	if (debug & SAMPLEDEBUG) {
	    printf("[asgnsamples] totime %f\n", totime);
	}
#   endif DEBUG
}


unsigned long
min(a, b)
    unsigned long a,b;
{
    if (a<b)
	return(a);
    return(b);
}

unsigned long
max(a, b)
    unsigned long a,b;
{
    if (a>b)
	return(a);
    return(b);
}

    /*
     *	calculate scaled entry point addresses (to save time in asgnsamples),
     *	and possibly push the scaled entry points over the entry mask,
     *	if it turns out that the entry point is in one bucket and the code
     *	for a routine is in the next bucket.
     */
void
alignentries()
{
    register struct nl	*nlp;
    unsigned long	bucket_of_entry;
    unsigned long	bucket_of_code;

    for (nlp = nl; nlp < npe; nlp++) {
	nlp -> svalue = nlp -> value / sizeof(UNIT);
	bucket_of_entry = (nlp->svalue - lowpc) / scale;
	bucket_of_code = (nlp->svalue + UNITS_TO_CODE - lowpc) / scale;
	if (bucket_of_entry < bucket_of_code) {
#	    ifdef DEBUG
		if (debug & SAMPLEDEBUG) {
		    printf("[alignentries] pushing svalue 0x%x to 0x%x\n",
			    nlp->svalue, nlp->svalue + UNITS_TO_CODE);
		}
#	    endif DEBUG
	    nlp->svalue += UNITS_TO_CODE;
	}
    }
}

#ifdef ELF
bool
funcsymbol( nlistp )
    Elf32_Sym	*nlistp;
{
    extern char	*strtab;	/* string table from a.out */
    extern int	aflag;		/* if static functions aren't desired */
    char	*name, c;

	/*
	 *	must be a text symbol,
	 *	and static text symbols don't qualify if aflag set.
	 */
#if 0
    /* Dave threw this in while debugging */
    /* Gets pretty spammy, but not as bad as the full gprof debug */
    if ( ELF32_ST_TYPE( nlistp->st_info) != STT_FUNC) {
	    printf("  *** not STT_FUNC, it's a %d:  %s\n",
		   ELF32_ST_TYPE( nlistp->st_info),
		   strtab + nlistp->st_name);
	    return FALSE;
    }
    if ( ELF32_ST_BIND(nlistp->st_info == STB_LOCAL) && (aflag == 0))
    {
	    printf(" *** not AFLAG:  %s\n", strtab + nlistp->st_name);
	    return FALSE;
    }
#else
    if (( ELF32_ST_TYPE( nlistp->st_info ) != STT_FUNC ) 
	|| (( ELF32_ST_BIND(nlistp->st_info == STB_LOCAL ) && ( aflag == 0 ))))
	    return FALSE;
#endif

	/*
	 *	can't have any `funny' characters in name,
	 *	where `funny' includes	`.', .o file names
	 *			and	`$', pascal labels.
	 *	need to make an exception for sparc .mul & co.
	 *	perhaps we should just drop this code entirely...
	 */
    name = strtab + nlistp->st_name;

    while ( (c = *name++) ) {
	if ( c == '.' || c == '$' ) {
	    return FALSE;
	}
    }
    return TRUE;
}
#else
bool
funcsymbol( nlistp )
    struct nlist	*nlistp;
{
    extern char	*strtab;	/* string table from a.out */
    extern int	aflag;		/* if static functions aren't desired */
    char	*name, c;

	/*
	 *	must be a text symbol,
	 *	and static text symbols don't qualify if aflag set.
	 */
    if ( ! (  ( nlistp -> n_type == ( N_TEXT | N_EXT ) )
	   || ( ( nlistp -> n_type == N_TEXT ) && ( aflag == 0 ) ) ) ) {
	return FALSE;
    }
	/*
	 *	can't have any `funny' characters in name,
	 *	where `funny' includes	`.', .o file names
	 *			and	`$', pascal labels.
	 *	need to make an exception for sparc .mul & co.
	 *	perhaps we should just drop this code entirely...
	 */
    name = strtab + nlistp -> n_un.n_strx;
#ifdef sparc
    if (nlistp -> n_value & 3)
	return FALSE;
    if ( *name == '.' ) {
	char *p = name + 1;
	if ( *p == 'u' )
	    p++;
	if ( strcmp ( p, "mul" ) == 0 || strcmp ( p, "div" ) == 0 ||
	     strcmp ( p, "rem" ) == 0 )
		return TRUE;
    }
#endif
    while ( c = *name++ ) {
	if ( c == '.' || c == '$' ) {
	    return FALSE;
	}
    }
    return TRUE;
}
#endif

#include <setjmp.h>
static jmp_buf gprof_jmpbuf;

/*
 * Sine we are run out of atexit, we do not want to just bail from the
 * program when something goes wrong, since we still want the rest of
 * the atexit functions to run. The simplest is to use longjmp to zap
 * us back out, and continue with the atexit functions.
 */
void
done()
{
	_longjmp(gprof_jmpbuf, 1);
}

void
gprof_atexit()
{
	char *foo[] = {"gprof",NULL};

	if (_setjmp(gprof_jmpbuf) == 0)
		gprof_main(1, foo);
}

/*
 * Hook to run gprof standalone. See gprof sample kernel.
 */
void
gprof(argc, argv)
    int argc;
    char **argv;
{
	if (_setjmp(gprof_jmpbuf) == 0)
		gprof_main(argc, argv);
}
