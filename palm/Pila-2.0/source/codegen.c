/***********************************************************************
 *
 *      CODEGEN.C
 *      Code Generation Routines for 68000 Assembler
 *
 *    Function: output()
 *      Places the data whose size and value are specified onto
 *      the output stream at the current location contained in
 *      global varible gulOutLoc. That is, if a listing is being
 *      produced, it calls ListPutData() to print the data in the
 *      object code field of the current listing line; if an
 *      object file is being produced, it calls outputObj() to
 *      output the data in the form of S-records.
 *
 *      effAddr()
 *      Computes the 6-bit effective address code used by the
 *      68000 in most cases to specify address modes. This code
 *      is returned as the value of effAddr(). The desired
 *      addressing mode is determined by the field of the
 *      opDescriptor which is pointed to by the operand
 *      argument. The lower 3 bits of the output contain the
 *      register code and the upper 3 bits the mode code.
 *
 *      extWords()
 *      Computes and outputs (using output()) the extension
 *      words for the specified operand. The generated
 *      extension words are determined from the data contained
 *      in the opDescriptor pointed to by the op argument and
 *      from the size code of the instruction, passed in
 *      the size argument. Any error is reported through a
 *      call to Error(<code>).
 *
 *   Usage: output(data, size)
 *      int data, size;
 *
 *      effAddr(operand)
 *      opDescriptor *operand;
 *
 *      extWords(op, size)
 *      opDescriptor *op;
 *      int size;
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 12/13/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *
 ************************************************************************/

#include "pila.h"
#include "asm.h"

extern long gulOutLoc;
extern int  giPass;
extern FILE *gpfilList;

int output(long data, int size)
{
    ListPutData(data, size);
    outputObj(gulOutLoc, data, size);

    return NORMAL;
}


int effAddr(opDescriptor *operand)
{
    if (operand->mode == DnDirect) {
        return 0x00 | operand->reg;
    }
    if (operand->mode == AnDirect) {
        return 0x08 | operand->reg;
    }
    if (operand->mode == AnInd) {
        return 0x10 | operand->reg;
    }
    if (operand->mode == AnIndPost) {
        return 0x18 | operand->reg;
    }
    if (operand->mode == AnIndPre) {
        return 0x20 | operand->reg;
    }
    if (operand->mode == AnIndDisp) {
        return 0x28 | operand->reg;
    }
    if (operand->mode == AnIndIndex) {
        return 0x30 | operand->reg;
    }
    if (operand->mode == AbsShort) {
        return 0x38;
    }
    if (operand->mode == AbsLong) {
        return 0x39;
    }
    if (operand->mode == PCDisp) {
        return 0x3A;
    }
    if (operand->mode == PCIndex) {
        return 0x3B;
    }
    if (operand->mode == Immediate) {
        return 0x3C;
    }
    printf("INVALID EFFECTIVE ADDRESSING MODE!\n");
    exit (0);
}


int extWords(opDescriptor *op, int size)
{
    long disp;

    if (op->mode == DnDirect ||
        op->mode == AnDirect ||
        op->mode == AnInd ||
        op->mode == AnIndPost ||
        op->mode == AnIndPre) {
        ;
    } else if (op->mode == AnIndDisp || 
               op->mode == PCDisp) {
        if (giPass==2) {
            disp = op->data.value;
            if (op->mode == PCDisp) {
                disp -= gulOutLoc;
            }
            else if (op->data.type!=NULL && size!=0)
            {
              int typeSize = SymbolGetSize(op->data.type);
              if (typeSize>0 && typeSize<=4 && typeSize!=size)
                Error(INSTR_AND_OPER_SIZE_MISMATCH,NULL);
            }
            output(disp & 0xFFFF, WORD);
            if (disp < -32768 || disp > 32767) {
                Error(INV_DISP,NULL);
            }
        }
        gulOutLoc += 2;
    } else if (op->mode == AnIndIndex ||
               op->mode == PCIndex) {
        if (giPass==2) {
            disp = op->data.value;
            if (op->mode == PCIndex) {
                disp -= gulOutLoc;
            }
            else if (op->data.type!=NULL && size!=0)
            {
              int typeSize = SymbolGetSize(op->data.type);
              if (typeSize>0 && typeSize<=4 && typeSize!=size)
                Error(INSTR_AND_OPER_SIZE_MISMATCH,NULL);
            }
            output((( (int) (op->size) == LONG) ? 0x800 : 0)
                   | (op->index << 12) | (disp & 0xFF), WORD);
            if (disp < -128 || disp > 127) {
                Error(INV_DISP,NULL);
            }
        }
        gulOutLoc += 2;
    } else if (op->mode == AbsShort) {
        if (giPass==2) {
            output(op->data.value & 0xFFFF, WORD);
            if (op->data.value < -32768 || op->data.value > 32767) {
                Error(INV_ABS_ADDRESS,NULL);
            }
        }
        gulOutLoc += 2;
    } else if (op->mode == AbsLong) {
        if (giPass==2) {
            output(op->data.value, LONG);
        }
        gulOutLoc += 4;
    } else if (op->mode == Immediate) {
        if (!size || size == WORD) {
            if (giPass==2) {
                output(op->data.value & 0xFFFF, WORD);
                /*
                if (op->data.value < -32768 || op->data.value > 32767)
                Error(INV_16_BIT_DATA,NULL);
                */
                if (op->data.value > 0xffff) {
                    Error(INV_16_BIT_DATA,NULL);
                }
            }
            gulOutLoc += 2;
        } else if (size == BYTE) {
            if (giPass==2) {
                output(op->data.value & 0xFF, WORD);
                if (op->data.value < -32768 || op->data.value > 32767) {
                    Error(INV_8_BIT_DATA,NULL);
                }
            }
            gulOutLoc += 2;
        } else if (size == LONG) {
            if (giPass==2) {
                output(op->data.value, LONG);
            }
            gulOutLoc += 4;
        }
    } else {
        printf("INVALID EFFECTIVE ADDRESSING MODE!\n");
        exit(0);
    }

    return NORMAL;
}
