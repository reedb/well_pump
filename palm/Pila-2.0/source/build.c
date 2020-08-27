/***********************************************************************
 *
 *      BUILD.C
 *      Instruction Building Routines for 68000 Assembler
 *
 * Description: The functions in this file build instructions, that is,
 *      they assemble the instruction word and its extension
 *      words given the skeleton bit mask for the instruction
 *      and opDescriptors for its operand(s). The instructions
 *      that each routine builds are noted above it in a
 *      comment. All the functions share the same calling
 *      sequence (except zeroOp, which has no argument and
 *      hence omits the operand descriptors), which is as
 *      follows:
 *
 *          general_name(mask, size, source, dest);
 *          int mask, size;
 *          opDescriptor *source, *dest;
 *
 *      except
 *
 *          zeroOp(mask, size);
 *          int mask, size;
 *
 *      The mask argument is the skeleton mask for the
 *      instruction, i.e., the instruction word before the
 *      addressing information has been filled in. The size
 *      argument contains the size code that was specified with
 *      the instruction (using the definitions in ASM.H) or 0
 *      if no size code was specified. Arguments source and
 *      dest are pointers to opDescriptors for the two
 *      operands (only source is valid in some cases). The last
 *      argument is used to return a status via the standard
 *      mechanism.
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
#include "insttabl.h"
#include "guard.h"

extern long gulOutLoc;
extern int  giPass;


/***********************************************************************
 *
 *  Function move builds the MOVE and MOVEA instructions
 *
 ***********************************************************************/

int move(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    unsigned short moveMask;
    char destCode;

    /* Check whether the instruction can be assembled as MOVEQ */
    if (source->mode == Immediate
        && size == LONG && dest->mode == DnDirect
        && source->data.value >= -128 && source->data.value <= 127
        && source->data.kind!=symbolKindUndefined)
    {
      if (giPass==1)
        Guard(GUARD_USE_MOVEQ,0); // remember that we used a MOVEQ

      // only use MOVEQ if we are in pass 1 or if we are in
      // pass 2 and we used MOVEQ already in pass 1
	  if (giPass==1 || (giPass==2 && GuardGet(0)==GUARD_USE_MOVEQ))
      {
        moveq(0x7000, size, source, dest);
        return NORMAL;
      }
    }
    else
    {
      if (giPass==1)
        Guard(GUARD_USE_MOVE,0); // remember that we used a plain MOVE
      else if (giPass==2 && GuardGet(0)!=GUARD_USE_MOVE) // if we didn't use plain move in pass 1 we have a problem
        Error(GUARD_ERROR,NULL);
    }

    /* Otherwise assemble it as plain MOVE */
    moveMask = mask | effAddr(source);
    destCode = (char) (effAddr(dest) & 0xff);
    moveMask |= (destCode & 0x38) << 3 | (destCode & 7) << 9;
    if (giPass==2) {
        output((long) (moveMask), WORD);
    }
    gulOutLoc += 2;
    extWords(source, size);
    extWords(dest, size);
    
    return NORMAL;
}


/***********************************************************************
 *
 *  Function zeroOp builds the following instructions:
 *   ILLEGAL
 *   NOP
 *   RESET
 *   RTE
 *   RTR
 *   RTS
 *   TRAPV
 *
 ***********************************************************************/

int zeroOp(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask), WORD);
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function oneOp builds the following instructions:
 *   ASd  <ea>
 *   CLR
 *   JMP
 *   JSR
 *   LSd  <ea>
 *   MOVE <ea>,CCR
 *   MOVE <ea>,SR
 *   NBCD
 *   NEG
 *   NEGX
 *   NOT
 *   PEA
 *   ROd  <ea>
 *   ROXd <ea>
 *   TAS
 *   TST
 *
 ***********************************************************************/

int oneOp(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(source)), WORD);
    }
    gulOutLoc += 2;
    extWords(source, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function arithReg builds the following instructions:
 *   ADD <ea>,Dn
 *   ADDA
 *   AND <ea>,Dn
 *   CHK
 *   CMP
 *   CMPA
 *   DIVS
 *   DIVU
 *   LEA
 *   MULS
 *   MULU
 *   OR <ea>,Dn
 *   SUB <ea>,Dn
 *   SUBA
 *
 ***********************************************************************/

int arithReg(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(source) | (dest->reg << 9)), WORD);
    }
    gulOutLoc += 2;
    extWords(source, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function arithAddr builds the following instructions:
 *   ADD Dn,<ea>
 *   AND Dn,<ea>
 *   BCHG Dn,<ea>
 *   BCLR Dn,<ea>
 *   BSET Dn,<ea>
 *   BTST Dn,<ea>
 *   EOR
 *   OR Dn,<ea>
 *   SUB Dn,<ea>
 *
 ***********************************************************************/

int arithAddr(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(dest) | (source->reg << 9)), WORD);
    }
    gulOutLoc += 2;
    extWords(dest, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function immedInst builds the following instructions:
 *   ADDI
 *   ANDI
 *   CMPI
 *   EORI
 *   ORI
 *   SUBI
 *
 ***********************************************************************/

int immedInst(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
  unsigned short type;

  // Check whether the instruction is an immediate ADD or SUB
  // that can be assembled as ADDQ or SUBQ
  // Check the mask to determine the operation
  type = mask & 0xFF00;
  if ((type == 0x0600 || type == 0x0400)
      && source->data.value>=1 
      && source->data.value<=8
      && source->data.kind!=symbolKindUndefined)
  {
    if (giPass==1)
      Guard(GUARD_USE_QUICKMATH,0); // remember that we used quickMath

    // only use quick math if we are in pass 1 or if we are
	// in pass 2 and used quick math in pass 1 already        
    if (giPass==1 || (giPass==2 && GuardGet(0)==GUARD_USE_QUICKMATH))
    {
      if (type == 0x0600) {
          /* Assemble as ADDQ */
          quickMath(0x5000 | (mask & 0x00C0), size, source, dest);
          return NORMAL;
      } else {
          /* Assemble as SUBQ */
          quickMath(0x5100 | (mask & 0x00C0), size, source, dest);
          return NORMAL;
      }
    }
  }
  else
  {
    if (giPass==1)
      Guard(GUARD_NO_QUICKMATH,0); // remember that we didn't use quickMath
    else if (giPass==2 && GuardGet(0)!=GUARD_NO_QUICKMATH) // if we did use quickMath in pass 1 we have a problem
      Error(GUARD_ERROR,NULL);
  }

  /* Otherwise assemble as an ordinary instruction */
  if (giPass==2) {
      output((long) (mask | effAddr(dest)), WORD);
  }
  gulOutLoc += 2;
  extWords(source, size);
  extWords(dest, size);

  return NORMAL;
}


/***********************************************************************
 *
 *  Function quickMath builds the following instructions:
 *   ADDQ
 *   SUBQ
 *
 ***********************************************************************/

int quickMath(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(dest) | ((source->data.value & 7) << 9)), WORD);
        if (source->data.value < 1 || source->data.value > 8) {
            Error(INV_QUICK_CONST,NULL);
        }
    }
    gulOutLoc += 2;
    extWords(dest, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function movep builds the MOVEP instruction.
 *
 ***********************************************************************/

int movep(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        if (source->mode == DnDirect) {
            /* Convert plain address register indirect to address
               register indirect with displacement of 0 */
            if (dest->mode == AnInd) {
                dest->mode = AnIndDisp;
                dest->data.value = 0;
                dest->data.kind  = symbolKindConst;
            }
            output((long) (mask | (source->reg << 9) | (dest->reg)), WORD);
            gulOutLoc += 2;
            extWords(dest, size);
        } else {
            /* Convert plain address register indirect to address
               register indirect with displacement of 0 */
            if (source->mode == AnInd) {
                source->mode = AnIndDisp;
                source->data.value = 0;
                source->data.kind  = symbolKindConst;
            }
            output((long) (mask | (dest->reg << 9) | (source->reg)), WORD);
            gulOutLoc += 2;
            extWords(source, size);
        } 
    } else {
        gulOutLoc += 4;
    }

    return NORMAL;
}


/***********************************************************************
 *
 *  Function moves builds the MOVES instruction.
 *
 ***********************************************************************/

int moves(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        if (source->mode & (DnDirect | AnDirect)) {
            output((long) (mask | effAddr(dest)), WORD);
            gulOutLoc += 2;
            if (source->mode == DnDirect) {
                output((long) (0x0800 | (source->reg << 12)), WORD);
            } else {
                output((long) (0x8800 | (source->reg << 12)), WORD);
            }
            gulOutLoc += 2;
        } else {
            output((long) mask | effAddr(source), WORD);
            gulOutLoc += 2;
            if (dest->mode == DnDirect) {
                output((long) (dest->reg << 12), WORD);
            } else {
                output((long) (0x8000 | (dest->reg << 12)), WORD);
            }
            gulOutLoc += 2;
        } 
    } else {
        gulOutLoc += 4;
    }
    extWords((source->mode&(DnDirect|AnDirect))?dest:source, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function moveReg builds the following instructions:
 *   MOVE from CCR
 *   MOVE from SR
 *
 ***********************************************************************/

int moveReg(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(dest)), WORD);
    }
    gulOutLoc += 2;
    extWords(dest, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function staticBit builds the following instructions:
 *   BCHG #n,<ea>
 *   BCLR #n,<ea>
 *   BSET #n,<ea>
 *   BTST #n,<ea>
 *
 ***********************************************************************/

int staticBit(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(dest)), WORD);
        gulOutLoc += 2;
        output(source->data.value & 0xFF, WORD);
        gulOutLoc += 2;
    } else {
        gulOutLoc += 4;
    }
    extWords(dest, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function movec builds the MOVEC instruction.
 *
 ***********************************************************************/

int movec(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    unsigned short mask2;
    opDescriptor *regOp;
    long    controlMode;

    if (giPass==2) {
        output((long) (mask), WORD);
        gulOutLoc += 2;
        if (mask & 1) {
            regOp = source;
            controlMode = dest->mode;
        } else {
            regOp = dest;
            controlMode = source->mode;
        }
        mask2 = regOp->reg << 12;
        if (regOp->mode == AnDirect) {
            mask2 |= 0x8000;
        }
        if (controlMode == SFCDirect) {
            mask2 |= 0x000;
        } else if (controlMode == DFCDirect) {
            mask2 |= 0x001;
        } else if (controlMode == USPDirect) {
            mask2 |= 0x800;
        } else if (controlMode == VBRDirect) {
            mask2 |= 0x801;
        }
        output((long) (mask2), WORD);
        gulOutLoc += 2;
    } else {
        gulOutLoc += 4;
    }

    return NORMAL;
}


/***********************************************************************
 *
 *  Function trap builds the TRAP instruction.
 *
 ***********************************************************************/

int trap(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output(mask | (source->data.value & 0xF), WORD);
        if (source->data.value < 0 || source->data.value > 15) {
            Error(INV_VECTOR_NUM,NULL);
        }
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function branch builds the following instructions:
 *   BCC (BHS)   BGT     BLT         BRA
 *   BCS (BLO)   BHI     BMI         BSR
 *   BEQ         BLE     BNE         BVC
 *   BGE         BLS     BPL         BVS
 *
 ***********************************************************************/

int branch(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    long    disp;

    if (source->data.kind==symbolKindUndefined)
		disp = 32000;		// ensure long jump if target is undefined
	else
		disp = source->data.value-gulOutLoc-2;

											// make a short branch if
    if ((size==SHORT								// 'short' was given as size
         || (size!=LONG								// or 'long' wasn't given
		     && disp >= -128 && disp <= 127 && disp)// and the distance is short
        )
        && (giPass<2 || GuardGet(0)==GUARD_SHORT_BRANCH) // and no long branch was used in pass 1
       )
    {
		if (giPass==1)
			Guard(GUARD_SHORT_BRANCH,0); // remember using short branch
		else if (giPass==2)
		{
			output((long) (mask | (disp & 0xFF)), WORD);
			if (disp < -128 || disp > 127 || !disp)
				Error(INV_BRANCH_DISP,NULL);
		}
		gulOutLoc += 2;
	}
    else
    {
		if (giPass==2)
		{
			if (GuardGet(0)!=GUARD_LONG_BRANCH) // if we used a short branch in pass 1 we have a problem
			{
				Error(UNSUCCESSFULL_SHORT_BRANCH,NULL);
				gulOutLoc += 2;
			}
			else
			{
				output((long) (mask), WORD);
				gulOutLoc += 2;
				output((long) (disp), WORD);
				gulOutLoc += 2;
				if (disp < -32768 || disp > 32767)
					Error(INV_BRANCH_DISP,NULL);
			}
		}
		else
		{
			if (giPass==1)
				Guard(GUARD_LONG_BRANCH,0); // remember using a long branch
			gulOutLoc += 4;
		}
	}    
    return NORMAL;
}


/***********************************************************************
 *
 *  Function moveq builds the MOVEQ instruction.
 *
 ***********************************************************************/

int moveq(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output(mask | (dest->reg << 9) | (source->data.value & 0xFF), WORD);
        if (source->data.value < -128 || source->data.value > 127) {
            Error(INV_QUICK_CONST,NULL);
        }
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function immedToCCR builds the following instructions:
 *   ANDI to CCR
 *   EORI to CCR
 *   ORI to CCR
 *
 ***********************************************************************/

int immedToCCR(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask), WORD);
        gulOutLoc += 2;
        output(source->data.value & 0xFF, WORD);
        gulOutLoc += 2;
        if ((source->data.value & 0xFF) != source->data.value) {
            Error(INV_8_BIT_DATA,NULL);
        }
    } else {
        gulOutLoc += 4;
    }

    return NORMAL;
}


/***********************************************************************
 *
 *  Function immedWord builds the following instructions:
 *   ANDI to SR
 *   EORI to SR
 *   ORI to SR
 *   RTD
 *   STOP
 *
 ***********************************************************************/

int immedWord(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask), WORD);
        gulOutLoc += 2;
        output(source->data.value & 0xFFFF, WORD);
        gulOutLoc += 2;
        if (source->data.value < -32768 || source->data.value > 65535) {
            Error(INV_16_BIT_DATA,NULL);
        }
    } else {
        gulOutLoc += 4;
    }

    return NORMAL;
}


/***********************************************************************
 *
 *  Function dbcc builds the following instructions:
 *   DBCC (DBHS)  DBGE   DBLS        DBPL
 *   DBCS (DBLO)  DBGT   DBLT        DBT
 *   DBEQ         DBHI   DBMI        DBVC
 *   DBF (DBRA)   DBLE   DBNE        DBVS
 *
 ***********************************************************************/

int dbcc(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    long disp;

    disp = dest->data.value - gulOutLoc - 2;
    if (giPass==2) {
        output((long) (mask | source->reg), WORD);
        gulOutLoc += 2;
        output(disp, WORD);
        gulOutLoc += 2;
        if (disp < -32768 || disp > 32767) {
            Error(INV_BRANCH_DISP,NULL);
        }
    } else {
        gulOutLoc += 4;
    }

    return NORMAL;
}


/***********************************************************************
 *
 *  Function scc builds the following instructions:
 *   SCC (SHS)   SGE     SLS        SPL
 *   SCS (SLO)   SGT     SLT        ST
 *   SEQ         SHI     SMI        SVC
 *   SF      SLE     SNE        SVS
 *
 ***********************************************************************/

int scc(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | effAddr(source)), WORD);
    }
    gulOutLoc += 2;
    extWords(source, size);

    return NORMAL;
}


/***********************************************************************
 *
 *  Function shiftReg builds the following instructions:
 *   ASd    Dx,Dy
 *   LSd    Dx,Dy
 *   ROd    Dx,Dy
 *   ROXd   Dx,Dy
 *   ASd    #<data>,Dy
 *   LSd    #<data>,Dy
 *   ROd    #<data>,Dy
 *   ROXd   #<data>,Dy
 *
 ***********************************************************************/

int shiftReg(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        mask |= dest->reg;
        if (source->mode == Immediate) {
            mask |= (source->data.value & 7) << 9;
            if (source->data.value < 1 || source->data.value > 8) {
                Error(INV_SHIFT_COUNT,NULL);
            }
        } else {
            mask |= source->reg << 9;
        }
        output((long) (mask), WORD);
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function exg builds the EXG instruction.
 *
 ***********************************************************************/

int exg(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        /* Are a data register and an address register being exchanged? */
        if (source->mode != dest->mode) {
            /* If so, the address register goes in bottom three bits */
            if (source->mode == AnDirect) {
                mask |= source->reg | (dest->reg << 9);
            } else {
                mask |= dest->reg | (source->reg << 9);
            }
        } else {
            /* Otherwise it doesn't matter which way they go */
            mask |= dest->reg | (source->reg << 9);
        }
        output((long) (mask), WORD);
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function twoReg builds the following instructions:
 *   ABCD
 *   ADDX
 *   CMPM
 *   SBCD
 *   SUBX
 *
 ***********************************************************************/

int twoReg(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | (dest->reg << 9) | source->reg), WORD);
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function oneReg builds the following instructions:
 *   EXT
 *   SWAP
 *   UNLK
 *
 ***********************************************************************/

int oneReg(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | source->reg), WORD);
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function moveUSP builds the following instructions:
 *   MOVE   USP,An
 *   MOVE   An,USP
 *
 ***********************************************************************/

int moveUSP(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        if (source->mode == AnDirect) {
            output((long) (mask | source->reg), WORD);
        } else {
            output((long) (mask | dest->reg), WORD);
        }
    }
    gulOutLoc += 2;

    return NORMAL;
}


/***********************************************************************
 *
 *  Function link builds the LINK instruction
 *
 ***********************************************************************/

int linkOp(int mask, int size, opDescriptor *source, opDescriptor *dest)
{
    if (giPass==2) {
        output((long) (mask | source->reg), WORD);
        gulOutLoc += 2;
        output(dest->data.value, WORD);
        gulOutLoc += 2;
        if (dest->data.value < -32768 || dest->data.value > 32767) {
            Error(INV_16_BIT_DATA,NULL);
        }
    } else {
        gulOutLoc += 4;
    }

    return NORMAL;
}

