/***********************************************************************
 *
 *              ASM.H
 *              Global Definitions for 68000 Assembler
 *
 *      Author: Paul McKee
 *              ECE492    North Carolina State University
 *
 *        Date: 12/13/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *
 ************************************************************************/

#ifndef __ASM_H__
#define __ASM_H__

#include "symbol.h"

/* global flags */
#define CASE_SENSITIVE

/* Significant length of a symbol */
#define SIGCHARS 52

/* Structure for operand descriptors */
typedef struct {
    long int mode;	/* Mode number (see below) */
    Value    data;	/* Immediate value, displacement, or absolute address */
    char     reg;	/* Principal register number (0-7) */
    char     index;	/* Index register number (0-7 = D0-D7, 8-15 = A0-A7) */
    char     size;	/* Size of index register (WORD or LONG, see below) */
} opDescriptor;

/* Instruction table definitions */

/* Structure to describe one "flavor" of an instruction */

typedef struct _flavor
{
    long int source;        /* Bit masks for the legal source... */
    long int dest;          /*  and destination addressing modes */
    char sizes;             /* Bit mask for the legal sizes */
    int (*exec)(int, int, opDescriptor *, opDescriptor *);
                            /* Pointer to routine to build the instruction */
    short int bytemask;     /* Skeleton instruction masks for byte size... */
    short int wordmask;     /*  word size, ... */
    short int longmask;     /*  and long sizes of the instruction */
} flavor;


/* Structure for the instruction table */

#define MNEMONIC_SIZE 16
typedef struct _instruction
{
    char mnemonic[MNEMONIC_SIZE]; /* Mnemonic */
    flavor *flavorPtr;            /* Pointer to flavor list */
    char flavorCount;             /* Number of flavors in flavor list */
    boolean parseFlag;            /* Should assemble() parse the operands? */
    int (*exec)(int, char *, char *);
                                  /* Routine to be called if parseFlag is FALSE */
} instruction;


/* Addressing mode codes/bitmasks */

#define DnDirect                        0x00001
#define AnDirect                        0x00002
#define AnInd                           0x00004
#define AnIndPost                       0x00008
#define AnIndPre                        0x00010
#define AnIndDisp                       0x00020
#define AnIndIndex                      0x00040
#define AbsShort                        0x00080
#define AbsLong                         0x00100
#define PCDisp                          0x00200
#define PCIndex                         0x00400
#define Immediate                       0x00800
#define SRDirect                        0x01000
#define CCRDirect                       0x02000
#define USPDirect                       0x04000
#define SFCDirect                       0x08000
#define DFCDirect                       0x10000
#define VBRDirect                       0x20000


/* Register and operation size codes/bitmasks */

#define BYTE    ((int) 1)
#define WORD    ((int) 2)
#define LONG    ((int) 4)
#define SHORT   ((int) 8)

/* function return codes */

#define NORMAL       0
#define CONTINUATION 1

//

typedef int BlockType;  // bt
#define kbtCode         ((BlockType)1)
#define kbtData         ((BlockType)2)
#define kbtResource     ((BlockType)3)


// Global variables

extern BlockType gbt;       // type of block (code, data, etc) being assembled
extern long gulCodeLoc;     // output location for next code byte
extern long gulDataLoc;     // output location for next data byte
extern long gulResLoc;      // output location for next resource byte
extern long gcbResTotal;    // total size of all resources

#define kcbCodeMax      (0x10000)
#define kcbDataMax      (0x10000)
#define kcbResMax       (0x40000)

extern unsigned char *gpbCode;
extern unsigned char *gpbData;
extern unsigned char *gpbResource;
extern unsigned char *gpbOutput;

extern unsigned long gfcResType;
extern long gidRes;

struct SourceStackEntry {
    int iLineNum;
    char szFile[_MAX_PATH];
    FILE *pfil;
};

extern struct SourceStackEntry *gpsseCur;

/* function prototype definitions */
#include "proto.h"

#endif /* __ASM_H__ */
