/***********************************************************************
 *
 *      INSTLOOKUP.C
 *      Instruction Table Lookup Routine for 68000 Assembler
 *
 *    Function: instLookup()
 *      Parses an instruction and looks it up in the instruction
 *      table. The input to the function is a pointer to the
 *      instruction on a line of assembly code. The routine
 *      scans the instruction and notes the size code if
 *      present. It then (binary) searches the instruction
 *      table for the specified opcode. If it finds the opcode,
 *      it returns a pointer to the instruction table entry for
 *      that instruction (via the instPtrPtr argument) as well
 *      as the size code or 0 if no size was specified (via the
 *      sizePtr argument). If the opcode is not in the
 *      instruction table, then the routine returns INV_OPCODE.
 *      Errors are reported through a call to function Error().
 *
 *   Usage: char *instLookup(p, instPtrPtr, sizePtr)
 *      char *p;
 *      instruction *(*instPtrPtr);
 *      char *sizePtr;
 *
 *  Errors: SYNTAX
 *      INV_OPCODE
 *      INV_SIZE_CODE
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 9/24/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *
 ************************************************************************/


#include "pila.h"
#include "asm.h"

#include "safe-ctype.h"

extern instruction instTable[];
extern int tableSize;


char *instLookup(char *p, instruction **instPtrPtr, char *sizePtr)
{
    char opcode[MNEMONIC_SIZE];
    int ch;
    int i, hi, lo, mid, cmp;

    /*  printf("InstLookup: Input string is \"%s\"\n", p); */
    i = 0;
    while (ISALPHA(*p)) {
        if (i < sizeof(opcode) - 1) {
            opcode[i++] = *p;
        }
        p++;
    }
    opcode[i] = '\0';

    if (*p == '.') {
        if (p[1] && (ISSPACE(p[2]) || !p[2])) {
            ch = TOUPPER(p[1]);
            if (ch == 'B') {
                *sizePtr = BYTE;
            } else if (ch == 'W') {
                *sizePtr = WORD;
            } else if (ch == 'L') {
                *sizePtr = LONG;
            } else if (ch == 'S') {
                *sizePtr = SHORT;
            } else {
                *sizePtr = 0;
                Error(INV_SIZE_CODE,p+1);
            }
            p += 2;
        } else {
            Error(SYNTAX,p);
            return NULL;
        } 
    } else if (!ISSPACE(*p) && *p) {
        Error(SYNTAX,p);
        return NULL;
    } else {
        *sizePtr = 0;
    }

    lo = 0;
    hi = tableSize - 1;
    do {
        mid = (hi + lo) / 2;
        cmp = strcmpi(opcode, instTable[mid].mnemonic); // opcodes aren't case sensitive
        if (cmp > 0) {
            lo = mid + 1;
        } else if (cmp < 0) {
            hi = mid - 1;
        }
    } while (cmp && (hi >= lo));

    if (!cmp) {
        *instPtrPtr = &instTable[mid];
        return p;
    } else {
        Error(INV_OPCODE,opcode);
        return NULL;
    }

    return NORMAL;

}

