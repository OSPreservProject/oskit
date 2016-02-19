/*
   ** regextr.c  -  extract graphics modes and register information
   **               from C source file
   **
   ** This file is part of SVGALIB (C) 1993 by Tommy Frandsen and
   **                                       Harm Hanemaayer
   **
   ** Copyright (C) 1993 by Hartmut Schirmer
   **
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vga.h"
#include "libvga.h"
#include "driver.h"

#ifndef FALSE
#define FALSE (1==0)
#endif
#ifndef TRUE
#define TRUE (1==1)
#endif

#define WordLen 100
typedef char WordStr[WordLen];

typedef struct ML {
    WordStr x, y, c;
    int mnum;
    int equ;
    void *regs;
    struct ML *nxt;
} ModeList;

static void *
 Malloc(unsigned long bytes)
{
    void *res;

    res = (void *) malloc(bytes);
    if (res == NULL) {
	fprintf(stderr, "regextr.c: Can't allocate memory\n");
	exit(1);
    }
    return res;
}

static void store_equ(ModeList ** root, char *x1, char *y1, char *c1,
		      int mnum, char *x2, char *y2, char *c2)
{
    ModeList *p;

    p = *root;
    while (p != NULL) {
	if (strcmp(p->x, x1) == 0
	    && strcmp(p->y, y1) == 0
	    && strcmp(p->c, c1) == 0) {
	    fprintf(stderr, "regextr.c: Duplicate g%sx%sx%s_regs !\n", x1, y1, c1);
	    exit(1);
	}
	p = p->nxt;
    }
    p = (ModeList *) Malloc(sizeof(ModeList));
    strcpy(p->x, x1);
    strcpy(p->y, y1);
    strcpy(p->c, c1);
    p->mnum = mnum;
    p->equ = TRUE;
    p->nxt = *root;
    *root = p;
    p = (ModeList *) Malloc(sizeof(ModeList));
    strcpy(p->x, x2);
    strcpy(p->y, y2);
    strcpy(p->c, c2);
    p->mnum = 0;
    p->equ = FALSE;
    p->regs = NULL;
    p->nxt = NULL;
    (*root)->regs = (void *) p;
}

static int check_new_mode(ModeList * p, char *x, char *y, char *c)
{
    while (p != NULL) {
	if (strcmp(p->x, x) == 0
	    && strcmp(p->y, y) == 0
	    && strcmp(p->c, c) == 0)
	    return FALSE;
	p = p->nxt;
    }
    return TRUE;
}

static void store_regs(ModeList ** root, char *x, char *y, char *c, int mnum, const char *r)
{
    ModeList *p;

    if (!check_new_mode(*root, x, y, c)) {
	fprintf(stderr, "regextr.c: Duplicate g%sx%sx%s_regs !\n", x, y, c);
	exit(1);
    }
    p = (ModeList *) Malloc(sizeof(ModeList));
    strcpy(p->x, x);
    strcpy(p->y, y);
    strcpy(p->c, c);
    p->mnum = mnum;
    p->equ = FALSE;
    p->regs = (void *) r;
    p->nxt = *root;
    *root = p;
}


static void __store_regs(ModeList ** root, int mnum, const char *r)
{
    WordStr x, y, c;

    sprintf(x, "%d", infotable[mnum].xdim);
    sprintf(y, "%d", infotable[mnum].ydim);
    switch (infotable[mnum].colors) {
    case 1 << 15:
	strcpy(c, "32K");
	break;
    case 1 << 16:
	strcpy(c, "64K");
	break;
    case 1 << 24:
	strcpy(c, "16M");
	break;
    default:
	sprintf(c, "%d", infotable[mnum].colors);
    }
    if (check_new_mode(*root, x, y, c))
	store_regs(root, x, y, c, mnum, r);
}


static char *
 mode2name(char *x, char *y, char *c)
{
    static char mn[WordLen];

    sprintf(mn, "G%sx%sx%s", x, y, c);
    return mn;
}

/* -------------------------------------- Scanner --- */
static int get_nextchar(FILE * inp)
{
    int nch;

    nch = fgetc(inp);
    if (nch == '\\') {
	int nnch;
	nnch = fgetc(inp);
	if (nnch == '\n')
	    return ' ';
	ungetc(nnch, inp);
    }
    if (isspace(nch))
	return ' ';
    return nch;
}

static int next_ch = ' ';

static int get_char(FILE * inp)
{
    int ch;

    do {
	ch = next_ch;
	do
	    next_ch = get_nextchar(inp);
	while (ch == ' ' && next_ch == ' ');
	if (ch != '/' || next_ch != '*')
	    return ch;
	do {
	    ch = next_ch;
	    next_ch = get_nextchar(inp);
	}
	while (ch != EOF && !(ch == '*' && next_ch == '/'));
	next_ch = get_nextchar(inp);
    }
    while (1);
}

static char *
 get_word(FILE * inp)
{
    int ch;
    static char buf[1000];
    char *p;

    do
	ch = get_char(inp);
    while (ch == ' ');
    p = buf;
    switch (ch) {
    case '[':
    case ']':
    case '{':
    case '}':
    case ',':
    case ';':
    case '=':
    case '(':
    case ')':
	*(p++) = ch;
	*p = '\0';
	return buf;
    case EOF:
	buf[0] = '\0';
	return buf;
    }
    for (;;) {
	*(p++) = ch;
	switch (next_ch) {
	case EOF:
	case '[':
	case ']':
	case '{':
	case '}':
	case ',':
	case ';':
	case '=':
	case '(':
	case ')':
	case ' ':
	    *p = '\0';
	    return buf;
	}
	ch = get_char(inp);
    }
}

/* ----------------------------------------------- parser -- */
static int is_res(char *rp, char *x, char *y, char *c, int *mnum)
{
    char *p;

    if (*(rp++) != 'g')
	return FALSE;
    /* X resolution */
    p = x;
    if (!isdigit(*rp))
	return FALSE;
    *(p++) = *(rp++);
    if (!isdigit(*rp))
	return FALSE;
    *(p++) = *(rp++);
    if (!isdigit(*rp))
	return FALSE;
    *(p++) = *(rp++);
    if (isdigit(*rp))
	*(p++) = *(rp++);
    if (*(rp++) != 'x')
	return FALSE;
    *p = '\0';

    /* Y resolution */
    p = y;
    if (!isdigit(*rp))
	return FALSE;
    *(p++) = *(rp++);
    if (!isdigit(*rp))
	return FALSE;
    *(p++) = *(rp++);
    if (!isdigit(*rp))
	return FALSE;
    *(p++) = *(rp++);
    if (isdigit(*rp))
	*(p++) = *(rp++);
    if (*(rp++) != 'x')
	return FALSE;
    *p = '\0';

    /* colors */
    p = c;
    *(p++) = *rp;
    switch (*(rp++)) {
    case '1':
	*(p++) = *rp;
	if (*(rp++) != '6')
	    return FALSE;
	if (*rp == 'M')
	    *(p++) = *(rp++);
	break;
    case '2':
	if (*rp == '5' && *(rp + 1) == '6') {
	    *(p++) = *(rp++);
	    *(p++) = *(rp++);
	}
	break;
    case '3':
	if (*rp != '2')
	    return FALSE;
	*(p++) = *(rp++);
	if (*rp != 'k' && *rp != 'K')
	    return FALSE;
	*(p++) = 'K';
	++rp;
	break;
    case '6':
	if (*rp != '4')
	    return FALSE;
	*(p++) = *(rp++);
	if (*rp != 'k' && *rp != 'K')
	    return FALSE;
	*(p++) = 'K';
	++rp;
	break;
    default:
	return FALSE;
    }
    *p = '\0';
    *mnum = __svgalib_name2number(mode2name(x, y, c));
    if (*mnum < 0) {
	int cols = 0;
	int xbytes = 0;

	if (strcmp("16M", c) == 0) {
	    cols = 1 << 24;
	    xbytes = atoi(x) * 3;
	} else if (strcmp("32K", c) == 0) {
	    cols = 1 << 15;
	    xbytes = atoi(x) * 2;
	} else if (strcmp("64K", c) == 0) {
	    cols = 1 << 16;
	    xbytes = atoi(x) * 2;
	} else if (strcmp("256", c) == 0) {
	    cols = 256;
	    xbytes = atoi(x);
	} else if (strcmp("16", c) == 0) {
	    cols = 16;
	    xbytes = atoi(x) / 4;
	} else
	    return FALSE;
	*mnum = __svgalib_addmode(atoi(x), atoi(y), cols, xbytes, xbytes / atoi(x));
    }
    return (*mnum > TEXT && *mnum != GPLANE16);
}

static int read_regs(FILE * inp, unsigned char **regs)
{
    unsigned char r[MAX_REGS];
    char *w;
    int c;
    unsigned u;

    if (strcmp("[", get_word(inp)) != 0)
	return 0;
    if (strcmp("]", get_word(inp)) != 0)
	if (strcmp("]", get_word(inp)) != 0)
	    return 0;
    if (strcmp("=", get_word(inp)) != 0)
	return 0;
    if (strcmp("{", get_word(inp)) != 0)
	return 0;

    c = 0;
    do {
	w = get_word(inp);
	if (strcmp(w, "}") == 0)
	    continue;
	if (sscanf(w, "%x", &u) == EOF)
	    if (sscanf(w, "%u", &u) == EOF) {
		fprintf(stderr, "regextr.c: Invalid register value %s\n", w);
		exit(1);
	    }
	r[c++] = u;
	w = get_word(inp);
    }
    while (strcmp(",", w) == 0);
    *regs = (char *) Malloc(c);
    memcpy(*regs, r, c);
    return c;
}


void __svgalib_readmodes(FILE * inp, ModeTable ** mt, int *dac, unsigned *clocks)
{
    WordStr x1, y1, c1, x2, y2, c2;
    WordStr w1, w2;
    int mnum1, mnum2;
    ModeList *modes = NULL;
    ModeList *p, *q, *cmp;
    int regs_count = -1;
    int change;
    int mode_cnt, i;

    /* read the register information from file */
    while (!feof(inp)) {
	char *wp;

	wp = get_word(inp);
	if (strcmp(wp, "#define") == 0) {
	    strcpy(w1, get_word(inp));
	    strcpy(w2, get_word(inp));
	    if (clocks != NULL && strcmp(w1, "CLOCK_VALUES") == 0) {
		unsigned freq;

		if (strcmp(w2, "{") == 0) {
		    do {
			strcpy(w2, get_word(inp));
			if (sscanf(w2, "%u", &freq) == EOF)
			    if (sscanf(w2, "%x", &freq) == EOF) {
				fprintf(stderr, "regextr.c: Invalid clock definition (%s)\n", w2);
				exit(1);
			    }
			*(clocks++) = freq;
			strcpy(w1, get_word(inp));
		    }
		    while (strcmp(",", w1) == 0);
		    clocks = NULL;
		}
	    } else if (dac != NULL && strcmp(w1, "DAC_TYPE") == 0) {
		int new_dac;

		if (sscanf(w2, "%d", &new_dac) == EOF)
		    if (sscanf(w2, "%x", &new_dac) == EOF) {
			fprintf(stderr, "regextr.c: Invalid dac definition (%s)\n", w2);
			exit(1);
		    }
		*dac = new_dac;
	    } else if (is_res(w1, x1, y1, c1, &mnum1)) {
		if (is_res(w2, x2, y2, c2, &mnum2))
		    store_equ(&modes, x1, y1, c1, mnum1, x2, y2, c2);
		else if (strcmp(w2, "DISABLE_MODE") == 0)
		    store_regs(&modes, x1, y1, c1, mnum1, DISABLE_MODE);
	    }
	} else if (strcmp(wp, "char") == 0) {
	    strcpy(w1, get_word(inp));
	    if (is_res(w1, x1, y1, c1, &mnum1)) {
		unsigned char *regs;
		int rv;

		rv = read_regs(inp, &regs);
		if (rv == 0)
		    continue;
		if (regs_count > 0 && rv != regs_count) {
		    fprintf(stderr, "regextr.c: Expected %d register values in %s, found %d\n",
			    regs_count, w1, rv);
		    exit(1);
		}
		regs_count = rv;
		store_regs(&modes, x1, y1, c1, mnum1, regs);
	    }
	}
    }
    /* resolve all equates */
    do {
	change = FALSE;
	p = modes;
	while (p != NULL) {
	    if (p->equ) {
		q = modes;
		cmp = (ModeList *) p->regs;
		while (q != NULL) {
		    if (!q->equ &&
			!strcmp(q->x, cmp->x) &&
			!strcmp(q->y, cmp->y) &&
			!strcmp(q->c, cmp->c)) {
			free(p->regs);
			p->regs = q->regs;
			p->equ = FALSE;
			change = TRUE;
			break;
		    }
		    q = q->nxt;
		}
	    }
	    p = p->nxt;
	}
    }
    while (change);
    /* Store modes from *mt */
    if (*mt != NULL)
	while ((*mt)->regs != NULL) {
	    __store_regs(&modes, (*mt)->mode_number, (*mt)->regs);
	    (*mt)++;
	}
    /* Check equates, count modes */
    mode_cnt = 0;
    p = modes;
    while (p != NULL) {
	if (p->equ) {
	    fprintf(stderr, "regextr.c: Unresolved equate (%sx%sx%s)\n", p->x, p->y, p->c);
	    exit(1);
	}
	p = p->nxt;
	++mode_cnt;
    }
    ++mode_cnt;
    /* Now generate the mode table */
    *mt = (ModeTable *) Malloc(mode_cnt * sizeof(ModeTable));
    i = 0;
    p = modes;
    while (p != NULL) {
#ifdef DEBUG
	printf("Found mode %2d: %s\n", p->mnum, mode2name(p->x, p->y, p->c));
#endif
	(*mt)[i].mode_number = p->mnum;
	(*mt)[i].regs = p->regs;
	q = p;
	p = p->nxt;
	free(q);
	++i;
    }
    (*mt)[i].mode_number = 0;
    (*mt)[i].regs = NULL;
}
