/*
 * Copyright (c) 1997 M.I.T.
 * All rights reserved.
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
 *      This product includes software developed by MIT.
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


/*
 * Public dpf interfaces. 
 */
#ifndef __DPF_H__
#define __DPF_H__

/* Errors */
enum { DPF_TOOMANYFILTERS = -2, DPF_TOOMANYELEMS = -3, DPF_BOGUSOP = -4, 
	DPF_BOGUSID = -5 , DPF_OVERLAP = -6, DPF_NILFILTER = -7};
	
/* Types we use.  Will be parameterized for the different machines. */
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#include <dpf-config.h>

/*
 * (*DANGER*): we rely on op, offset, nbits and mask being located in
 * the same place in struct eq and struct shift.
 */

/* size of the load is captured in the opcode. */
struct eq {
        uint8           op;     /* opcode. */
        uint16          offset; /* message offset */
        uint8           nbits;  /* length (in bits) */
        uint32          mask;   /* maximum width */
        uint32          val;    /* maximum size value */
};

/* size of the shift is captured in the opcode */
struct shift {
        uint8           op;     /* opcode */
        uint16          offset;
        uint8           nbits;  /* length (in bits) */
        uint32          mask;
        uint8           shift;
        uint8           align;  /* alignement given by shift */
};

struct ir {
        /**
         * The following fields are computed by dpf during insertion (we
         * don't trust clients).
         */

        /*
         * Maximum offset into message of code that we have to take
         * if this node succeeds.  Basically, for each or and shift
         * maxoffset holds the largest message offset of all nodes until
         * the next or, shift or end-of-filter.  This allows us to aggregate
         * message checks rather than having to do them on every atom.
         */
        uint16 maxoffset;

        /* Level of atom: is used as an index for various per-filter tables. */
        uint16 level;

        /* Alignment of atom at this point. */
        uint8   alignment;

        /* Whether a shift has occured upstream in the atomlist. */
        uint8 shiftp;

        union {
                struct eq eq;
                struct shift shift;
        } u;
};


/* container for code. */
struct dpf_ir {
        uint32  version;        /* ir may change from version to version. */
        uint16  irn;
        uint32  moffset;        /* the current offset added by dpf_shifti's. */
        struct ir ir[DPF_MAXELEM+1];            /* pointer to code */
};

/* Filter insertion/deletion. */
int dpf_insert_really(struct dpf_ir *filter);
int dpf_insert(void *p, int sz); 
int dpf_delete(unsigned pid);

/* dump a filter */
void dpf_printir(struct dpf_ir *ir);

/* initialize a filter structure. */
void dpf_init(struct dpf_ir *ir);

/*
 * This is so we can call the var oskit_dpf_iptr from user
 * code instead of having to call dpf_iptr. There are no constructors
 * in which we could make an assignment from one to the other.
 */
#ifdef OSKIT
#define dpf_iptr oskit_dpf_iptr
#endif

/* Called to classify a packet. */
extern int (*dpf_iptr)(uint8 *msg, unsigned nbytes);

/* 
 *  Filter creation routines.  nbits corresponds to 8, 16, 32 depending on
 * the operation.  msg[byte_offset:nbits] means to load nbits of the message
 * at byte_offset.
 */

/*
 * Compare message value to constant:
 * 	msg[byte_offset:nbits] == val
 */
void dpf_eq8(struct dpf_ir *ir, uint16 byte_offset, uint8 val);
void dpf_eq16(struct dpf_ir *ir, uint16 byte_offset, uint16 val);
void dpf_eq32(struct dpf_ir *ir, uint16 byte_offset, uint32 val);

/* 
 * Compare message value to constant:
 * 	msg[byte_offset:nbits] & mask == val
 */
void dpf_meq8(struct dpf_ir *ir, uint16 byte_offset, uint8 mask, uint8 val);
void dpf_meq16(struct dpf_ir *ir, uint16 byte_offset, uint16 mask, uint16 val);
void dpf_meq32(struct dpf_ir *ir, uint16 byte_offset, uint32 mask, uint32 val);

/* 
 * Shift the base message pointer: 
 * 	msg += msg[byte_offset:nbits] << shift; 
 */
void dpf_shift8(struct dpf_ir *ir, uint16 byte_offset, uint8 shift);
void dpf_shift16(struct dpf_ir *ir, uint16 byte_offset, uint8 shift);
void dpf_shift32(struct dpf_ir *ir, uint16 byte_offset, uint8 shift);

/* 
 * Shift the base message pointer: 
 *	msg += (msg[byte_offset:nbits] & mask) << shift; 
 */
void dpf_mshift8(struct dpf_ir *ir, uint16 offset, uint8 mask, uint8 shift);
void dpf_mshift16(struct dpf_ir *ir, uint16 offset, uint16 mask, uint8 shift);
void dpf_mshift32(struct dpf_ir *ir, uint16 offset, uint32 mask, uint8 shift);

/* Shift the base message pointer by a constant: msg += nbytes. */
void dpf_shifti(struct dpf_ir *ir, uint16 nbytes);


#endif /* __DPF_H__ */
