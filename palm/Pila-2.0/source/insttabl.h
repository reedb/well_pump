/***************************************************************
 *
 ***************************************************************/

#ifndef _INSTTABL_H_
#define _INSTTABL_H_

#include "asm.h"

int move(int, int, opDescriptor *, opDescriptor *);
int zeroOp(int, int, opDescriptor *, opDescriptor *);
int oneOp(int, int, opDescriptor *, opDescriptor *);
int arithReg(int, int, opDescriptor *, opDescriptor *);
int arithAddr(int, int, opDescriptor *, opDescriptor *);
int immedInst(int, int, opDescriptor *, opDescriptor *);
int quickMath(int, int, opDescriptor *, opDescriptor *);
int movep(int, int, opDescriptor *, opDescriptor *);
int moves(int, int, opDescriptor *, opDescriptor *);
int moveReg(int, int, opDescriptor *, opDescriptor *);
int staticBit(int, int, opDescriptor *, opDescriptor *);
int movec(int, int, opDescriptor *, opDescriptor *);
int trap(int, int, opDescriptor *, opDescriptor *);
int branch(int, int, opDescriptor *, opDescriptor *);
int moveq(int, int, opDescriptor *, opDescriptor *);
int immedToCCR(int, int, opDescriptor *, opDescriptor *);
int immedWord(int, int, opDescriptor *, opDescriptor *);
int dbcc(int, int, opDescriptor *, opDescriptor *);
int scc(int, int, opDescriptor *, opDescriptor *);
int shiftReg(int, int, opDescriptor *, opDescriptor *);
int exg(int, int, opDescriptor *, opDescriptor *);
int twoReg(int, int, opDescriptor *, opDescriptor *);
int oneReg(int, int, opDescriptor *, opDescriptor *);
int moveUSP(int, int, opDescriptor *, opDescriptor *);
int linkOp(int, int, opDescriptor *, opDescriptor *);

#endif
