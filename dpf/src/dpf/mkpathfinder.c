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


#include <old-dpf.h>

static struct dpf_frozen_ir df[] = {
	/*	op		len	r0	r1	imm	*/
#if 0
	{	DPF_EQI,	2,	1,	0,	0xc00c4501}, /* 0 */
	{	DPF_BITS32I,	0,	0,	0,	0xc}, /* 1 */
	{	DPF_EQI,	3,	1,	0,	0x20}, /* 2 */
	{	DPF_ANDI,	0,	1,	0,	0x20}, /* 3 */
	{	DPF_BITS8I,	0,	0,	0,	0x6}, /* 4 */
	{	DPF_EQI,	2,	1,	0,	0xed}, /* 5 */
	{	DPF_BITS16I,	0,	0,	0,	0x4}, /* 6 */
	{	DPF_EQI,	2,	1,	0,	0x6}, /* 7 */
	{	DPF_BITS8I,	0,	0,	0,	0x9}, /* 8 */
#endif
	{	DPF_EQI,	2,	1,	0,	0x4d2}, /* 9 */
	{	DPF_BITS16I,	0,	0,	0,	0x14}, /* 10 */
	{	DPF_EQI,	2,	1,	0,	0x10e1}, /* 11 */
	{	DPF_BITS16I,	0,	0,	0,	0x16}, /* 12 */
};
void *mk_pathfinder(int *sz, int src_port, int dst_port) {
	df[0].imm = src_port;
	df[2].imm = dst_port;

	*sz = sizeof df / sizeof df[0];
	return (void *)df;
}

/* filter missing check for src port */
static struct dpf_frozen_ir short_path2[] = {
	/*	op		len	r0	r1	imm	*/
#if 0
	{	DPF_EQI,	2,	1,	0,	0xc00c4501}, /* 0 */
	{	DPF_BITS32I,	0,	0,	0,	0xc}, /* 1 */
	{	DPF_EQI,	3,	1,	0,	0x20}, /* 2 */
	{	DPF_ANDI,	0,	1,	0,	0x20}, /* 3 */
	{	DPF_BITS8I,	0,	0,	0,	0x6}, /* 4 */
	{	DPF_EQI,	2,	1,	0,	0xed}, /* 5 */
	{	DPF_BITS16I,	0,	0,	0,	0x4}, /* 6 */
	{	DPF_EQI,	2,	1,	0,	0x6}, /* 7 */
	{	DPF_BITS8I,	0,	0,	0,	0x9}, /* 8 */
#endif
	{	DPF_EQI,	2,	1,	0,	0x10e1}, /* 11 */
	{	DPF_BITS16I,	0,	0,	0,	0x16}, /* 12 */
};

void *mk_short_path2(int *sz, int dst_port) {
	short_path2[0].imm = dst_port;

	*sz = sizeof short_path2 / sizeof short_path2[0];
	return (void *)short_path2;
}


static struct dpf_frozen_ir short_path[] = {
        /*      op              len     r0      r1      imm     */
#if 0
        {       DPF_EQI,        2,      1,      0,      0xc00c4501}, /* 0 */
        {       DPF_BITS32I,    0,      0,      0,      0xc}, /* 1 */
        {       DPF_EQI,        3,      1,      0,      0x20}, /* 2 */
        {       DPF_ANDI,       0,      1,      0,      0x20}, /* 3 */
        {       DPF_BITS8I,     0,      0,      0,      0x6}, /* 4 */
        {       DPF_EQI,        2,      1,      0,      0xed}, /* 5 */
        {       DPF_BITS16I,    0,      0,      0,      0x4}, /* 6 */
        {       DPF_EQI,        2,      1,      0,      0x6}, /* 7 */
        {       DPF_BITS8I,     0,      0,      0,      0x9}, /* 8 */
#endif
        {       DPF_EQI,        2,      1,      0,      0x4d2}, /* 9 */
        {       DPF_BITS16I,    0,      0,      0,      0x14}, /* 10 */
};
void *mk_short_path(int *sz, int src_port) {
	short_path[0].imm = src_port;
        *sz = sizeof short_path / sizeof short_path[0];
        return (void *)short_path;
}
