/***********************************************************************
 *
 *      PROTO.H
 *      Global Definitions for 68000 Assembler
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 9/5/88
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Modified for Pila Version 2.0
 *
 ************************************************************************/

#ifndef __PROTO_H__
#define __PROTO_H__

#include "pila.h"
#include "asm.h"
#include "symbol.h"

/* ANSI C function prototype definitions */

int processFile(char *);

int assemble(char *);

int pickMask(int, flavor *);

int output(long, int);

int effAddr(opDescriptor *);

int extWords(opDescriptor *, int);

char *evaluate(char *, Value *);

char *instLookup(char *, instruction *(*), char *);

char *skipSpace(char *);

void help(void);

int movem(int, char *, char *);

int reg(int, char *, char *);

char *evalList(char *, unsigned short *);

int initObj(char *);

int outputObj(long, long, int);

long checkValue(long);

int finishObj(void);

char *opParse(char *, opDescriptor *, int, boolean);

int writeObj(void);

#endif /* __PROTO_H__ */
