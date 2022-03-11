/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Written by H. Peter Anvin <hpa@zytor.com>
 * Brought in from Linux v4.4 and modified for U-Boot
 * From Linux arch/um/sys-i386/setjmp.S
 */

#ifndef __setjmp_h
#define __setjmp_h

struct jmp_buf_data {
	unsigned long __rip;
	unsigned long __rsp;
	unsigned long __rbp;
	unsigned long __rbx;
	unsigned long __r12;
	unsigned long __r13;
	unsigned long __r14;
	unsigned long __r15;
};

int setjmp(struct jmp_buf_data *jmp_buf);
void longjmp(struct jmp_buf_data *jmp_buf, int val);

#endif
