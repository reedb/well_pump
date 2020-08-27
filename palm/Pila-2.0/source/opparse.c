/***********************************************************************
 *
 *      OPPARSE.C
 *      Operand Parser for 68000 Assembler
 *
 *    Function: opParse()
 *      Parses an operand of the 68000 assembly language
 *      instruction and attempts to recognize its addressing
 *      mode. The p argument points to the string to be
 *      evaluated, and the function returns a pointer to the
 *      first character beyond the end of the operand.
 *      The function returns a description of the operands that
 *      it parses in an opDescriptor structure. The fields of
 *      the operand descriptor are filled in as appropriate for
 *      the mode of the operand that was found:
 *
 *       mode      returns the address mode (symbolic values
 *             defined in ASM.H)
 *       reg       returns the address or data register number
 *       data      returns the displacement or address or
 *             immediate value
 *       index     returns the index register
 *             (0-7 = D0-D7, 8-15 = A0-A7)
 *       size      returns the size to be used for the index
 *             register
 *
 *      Errors are returned through a call to function Error().
 *
 *   Usage: char *opParse(p, d, guardSubId)
 *      char *p;
 *      opDescriptor *d;
 *      int guardSubId; // used to distinguish between first and second operand
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 10/10/86
 *
 *    Revision: 10/26/87
 *      Altered the immediate mode case to correctly flag
 *      constructs such as "#$1000(A5)" as syntax errors.
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
#include "strcap.h"
#include "guard.h"

extern int giPass;

#define isTerm(c)   (ISSPACE(c) || (c==',') || c=='\0' || c==';')
#define isRegNum(c) ((c >= '0') && (c <= '7'))

char *_opParse(char *p, opDescriptor *d, int guardSubId)
{
    char szT[strlen(p)+1];
    char *pOrig;
    char *pTmp;
    int cch;
    int quoted;

    /* Check for immediate mode */
    if (p[0]=='#')
    {
      p = evaluate(++p, &(d->data));
      /* If expression evaluates OK, then return */
      if (p && !ErrorStatusIsSevere())
      {
        if (isTerm(*p))
        {
          d->mode = Immediate;
          return p;
        }
        else
          Error(SYNTAX,p);
      }
      return NULL;
    }

    // remove all blanks and the comment from the line
    // but honor quoted strings that may appear in expression
    pOrig  = p;
    pTmp   = p;
    quoted = 0;
    while(*p)
    {
      if (!quoted)
      {
        if (*p==';')
          *p = '\0'; // cut off comment
        else if (!ISSPACE(*p))
        {
          if (*p=='\'')
            quoted = 1;
          *(pTmp)++ = *(p++);
        }
        else
          p++;
      }
      else
      {
        if (*p=='\\')
        {
          *(pTmp++) = *(p++);
          if (*p)
            *(pTmp++) = *(p++);
        }
        else
        {
          if (*p=='\'')
            quoted = 0;
          *(pTmp++) = *(p++);
        }
      }
    }
    *pTmp = '\0';
    
    if (quoted)
    {
      Error(UNTERMINATED_STRING,pOrig);
      return NULL;
    }

    strcap(szT, pOrig);
    p = szT;

    /* Check for address or data register direct */
    if (isRegNum(p[1]) && isTerm(p[2])) {
        if (p[0] == 'D') {
            d->mode = DnDirect;
            d->reg = p[1] - '0';
            return (pOrig + 2);
        } else if (p[0] == 'A') {
            d->mode = AnDirect;
            d->reg = p[1] - '0';
            return (pOrig + 2);
        }
    }
    /* Check for Stack Pointer (i.e., A7) direct */
    if (p[0] == 'S' && p[1] == 'P' && isTerm(p[2])) {
        d->mode = AnDirect;
        d->reg = 7;
        return (pOrig + 2);
    }
    /* Check for address register indirect */
    if (p[0] == '(' &&
        ((p[1] == 'A' && isRegNum(p[2])) || (p[1] == 'S' && p[2] == 'P'))) {

        if (p[1] == 'S') {
            d->reg = 7;
        } else {
            d->reg = p[2] - '0';
        }
        if (p[3] == ')') {
            /* Check for plain address register indirect */
            if (isTerm(p[4])) {
                d->mode = AnInd;
                return pOrig+4;
            /* Check for postincrement */
            } else if (p[4] == '+') {
                d->mode = AnIndPost;
                return pOrig+5;
            }
        /* Check for address register indirect with index */
        } else if (p[3] == ',' && (p[4] == 'A' || p[4] == 'D')
                 && isRegNum(p[5])) {
            d->mode = AnIndIndex;
            /* Displacement is zero */
            d->data.value = 0;
            d->data.kind  = symbolKindConst;
            d->index = p[5] - '0';
            if (p[4] == 'A') {
                d->index += 8;
            }
            if (p[6] == '.') {
                /* Determine size of index register */
                if (p[7] == 'W') {
                    d->size = WORD;
                    return pOrig+9;
                } else if (p[7] == 'L') {
                    d->size = LONG;
                    return pOrig+9;
                } else {
                    Error(SYNTAX,pOrig+7);
                    return NULL;
                }
            } else if (p[6] == ')') {
                /* Default index register size is Word */
                d->size = WORD;
                return pOrig+7;
            } else {
                Error(SYNTAX,pOrig+6);
                return NULL;
            }
        }
    }
    /* Check for address register indirect with predecrement */
    if (p[0] == '-' && p[1] == '(' && p[4] == ')' &&
        ((p[2] == 'A' && isRegNum(p[3])) || (p[2] == 'S' && p[3] == 'P'))) {

        if (p[2] == 'S') {
            d->reg = 7;
        } else {
            d->reg = p[3] - '0';
        }
        d->mode = AnIndPre;
        return pOrig+5;
    }
    /* Check for PC relative */
    if (p[0] == '(' && p[1] == 'P' && p[2] == 'C') {
        /* Displacement is zero */
        d->data.value = 0;
        d->data.kind  = symbolKindCode;
        /* Check for plain PC relative */
        if (p[3] == ')') {
            d->mode = PCDisp;
            return pOrig+4;
        }
        /* Check for PC relative with index */
        else if (p[3] == ',' && (p[4] == 'A' || p[4] == 'D')
                 && isRegNum(p[5])) {
            d->mode = PCIndex;
            d->index = p[5] - '0';
            if (p[4] == 'A'){ 
                d->index += 8;
            }
            if (p[6] == '.') {
                /* Determine size of index register */
                if (p[7] == 'W') {
                    d->size = WORD;
                    return pOrig+9;
                } else if (p[7] == 'L') {
                    d->size = LONG;
                    return pOrig+9;
                } else {
                    Error(SYNTAX,pOrig+7);
                    return NULL;
                } 
            } else if (p[6] == ')') {
                /* Default size of index register is Word */
                d->size = WORD;
                return pOrig+7;
            } else {
                Error(SYNTAX,pOrig+6);
                return NULL;
            }
        }
    }

    /* Check for Status Register direct */
    if (p[0] == 'S' && p[1] == 'R' && isTerm(p[2])) {
        d->mode = SRDirect;
        return pOrig+2;
    }
    /* Check for Condition Code Register direct */
    if (p[0] == 'C' && p[1] == 'C' && p[2] == 'R' && isTerm(p[3])) {
        d->mode = CCRDirect;
        return pOrig+3;
    }
    /* Check for User Stack Pointer direct */
    if (p[0] == 'U' && p[1] == 'S' && p[2] == 'P' && isTerm(p[3])) {
        d->mode = USPDirect;
        return pOrig+3;
    }
    /* Check for Source Function Code register direct (68010) */
    if (p[0] == 'S' && p[1] == 'F' && p[2] == 'C' && isTerm(p[3])) {
        d->mode = SFCDirect;
        return pOrig+3;
    }
    /* Check for Destination Function Code register direct (68010) */
    if (p[0] == 'D' && p[1] == 'F' && p[2] == 'C' && isTerm(p[3])) {
        d->mode = DFCDirect;
        return pOrig+3;
    }
    /* Check for Vector Base Register direct (68010) */
    if (p[0] == 'V' && p[1] == 'B' && p[2] == 'R' && isTerm(p[3])) {
        d->mode = VBRDirect;
        return pOrig+3;
    }

    /* All other addressing modes start with a constant expression */
    pTmp = evaluate(pOrig, &(d->data));
    if (pTmp==NULL)
      return NULL;
      
    cch = pTmp-pOrig;
    p += cch;
    pOrig += cch;
    if (!ErrorStatusIsSevere()) {
        /* Check for absolute */
        if (isTerm(p[0]))
        {
          if (giPass<2)
          {
            /* Determine size of absolute address (must be long if
               the symbol isn't defined or if the value is too big */
            if (d->data.value > 32767 || d->data.value < -32768
					|| d->data.kind==symbolKindUndefined)
				d->mode = AbsLong;
            else
                d->mode = AbsShort;

			if (giPass>0)
				Guard(d->mode,guardSubId);
          }
          else
          {
            long guardValue = GuardGet(guardSubId);
            if (guardValue==AbsShort && (d->data.value>32767 || d->data.value<-32768))
            {
              guardValue = AbsLong;
              Error(GUARD_ERROR,NULL);
            }
            d->mode = guardValue;
          }
          return pOrig;
        }
        /* Check for address register indirect with displacement */
        if (p[0] == '(' &&
            ((p[1] == 'A' && isRegNum(p[2])) || (p[1] == 'S' && p[2] == 'P'))) {
            if (p[1] == 'S') {
                d->reg = 7;
            } else {
                d->reg = p[2] - '0';
            }
            /* Check for plain address register indirect with displacement */
            if (p[3] == ')') {
                if (d->data.value==0)
                  d->mode = AnInd;
                else
                  d->mode = AnIndDisp;
                return pOrig+4;
            /* Check for address register indirect with index */
            } else if (p[3] == ',' && (p[4] == 'A' || p[4] == 'D')
                && isRegNum(p[5])) {
                d->mode = AnIndIndex;
                d->index = p[5] - '0';
                if (p[4] == 'A') {
                    d->index += 8;
                }
                if (p[6] == '.') {
                    /* Determine size of index register */
                    if (p[7] == 'W') {
                        d->size = WORD;
                        return pOrig+9;
                    } else if (p[7] == 'L') {
                        d->size = LONG;
                        return pOrig+9;
                    } else {
                        Error(SYNTAX,pOrig+7);
                        return NULL;
                    }
                } else if (p[6] == ')') {
                    /* Default size of index register is Word */
                    d->size = WORD;
                    return pOrig+7;
                } else {
                    Error(SYNTAX,pOrig+6);
                    return NULL;
                }
            }
        }

        /* Check for PC relative */
        if (p[0] == '(' && p[1] == 'P' && p[2] == 'C') {
            /* Check for plain PC relative */
            if (p[3] == ')') {
                d->mode = PCDisp;
                return pOrig+4;
            }
            /* Check for PC relative with index */
            else if (p[3] == ',' && (p[4] == 'A' || p[4] == 'D')
                && isRegNum(p[5])) {
                d->mode = PCIndex;
                d->index = p[5] - '0';
                if (p[4] == 'A') {
                    d->index += 8;
                }
                if (p[6] == '.') {
                    /* Determine size of index register */
                    if (p[7] == 'W') {
                        d->size = WORD;
                        return pOrig+9;
                    } else if (p[7] == 'L') {
                        d->size = LONG;
                        return pOrig+9;
                    } else {
                        Error(SYNTAX,pOrig+7);
                        return NULL;
                    }
                } else if (p[6] == ')') {
                    /* Default size of index register is Word */
                    d->size = WORD;
                    return pOrig+7;
                } else {
                    Error(SYNTAX,pOrig+6);
                    return NULL;
                }
            }
        }
        /* If the operand doesn't match any pattern, return an error status */
        Error(SYNTAX,pOrig);
    }

    return NULL;
}

char *opParse(char *p, opDescriptor *d, int guardSubId, boolean branchInstruction)
{
  char *rtnVal = _opParse(p,d,guardSubId);
  if (rtnVal)
  {
    switch(d->mode)
    {
      case AnIndDisp:
      case AnIndIndex:
        switch(SymbolGetCategory(d->data.kind))
        {
          case symbolCategoryCode:
            Error(CODE_ADDRESS_NOT_PC,p);
            break;
          case symbolCategoryData:
            if (d->reg!=5)
              Error(GLOBAL_DATA_ADDRESS_NOT_A5,p);
            break;
          case symbolCategoryStack:
            if (d->reg!=6)
              Error(STACK_ADDRESS_NOT_A6,p);
            break;
          case symbolCategoryNone:
          case symbolCategoryRes:
          case symbolCategoryConst:
          case symbolCategoryType:
            break;
        }
        break;
      case AbsShort:
      case AbsLong:
        if (!branchInstruction || SymbolGetCategory(d->data.kind)!=symbolCategoryCode)
          Error(ABSOLUTE_ADDRESS,p);
        break;
      case PCDisp:
      case PCIndex:
        if (SymbolGetCategory(d->data.kind)!=symbolCategoryCode)
          Error(PC_WITH_NON_CODE_ADDR,p);
        break;
      case Immediate:
        if (SymbolGetCategory(d->data.kind)!=symbolCategoryConst)
          Error(IMMEDIATE_NOT_A_CONSTANT,p);
        break;
    }
  }
  
  return rtnVal;
}

