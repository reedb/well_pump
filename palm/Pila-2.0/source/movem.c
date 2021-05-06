/***********************************************************************
 *
 *      MOVEM.C
 *      Routines for the MOVEM instruction and the REG directive
 *
 *    Function: movem()
 *      Builds MOVEM instructions. The size of the instruction
 *      is given by the size argument (assumed to be word if
 *      not specified). The label argument points to the label
 *      appearing on the source line containing the MOVEM
 *      instruction, if any, and the op argument points to the
 *      operands of the MOVEM instruction. Errors are reported
 *      through a call to function Error(<code>).
 *
 *      reg()
 *      Defines a special register list symbol to be used as an
 *      argument for the MOVEM instruction. The size argument
 *      reflects the size code appended to the REG directive,
 *      which should be empty. The label argument points to the
 *      label appearing on the source line containing the REG
 *      directive (which must be specified), and the op
 *      argument points to a register list which is the new
 *      value of the symbol. Errors are reported through a call
 *      to function Error(<code>).
 *
 *   Usage: movem(size, label, op)
 *      int size;
 *      char *label, *op;
 *
 *      reg(size, label, op)
 *      int size;
 *      char *label, *op;
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 12/9/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *
 ************************************************************************/

#include "pila.h"
#include "asm.h"
#include "parse.h"

#include "safe-ctype.h"
#include "strcap.h"
#include "guard.h"

/* Define bit masks for the legal addressing modes of MOVEM */

#define ControlAlt  (AnInd | AnIndDisp | AnIndIndex | AbsShort | AbsLong)
#define DestModes   (ControlAlt | AnIndPre)
#define SourceModes (ControlAlt | AnIndPost | PCDisp | PCIndex)


extern long gulOutLoc;
extern int  giPass;


int movem(int size, char *label, char *op)
{
  char *p;
  unsigned short regList, temp, instMask;
  int i;
  opDescriptor memOp;

  /* Pick mask according to size code (only .W and .L are valid) */
  if (size == WORD) {
      instMask = 0x4880;
  } else if (size == LONG) {
      instMask = 0x48C0;
  } else {
      if (size) {
          Error(INV_SIZE_CODE,NULL);
      }
      instMask = 0x4880;
  }
  /* Define the label attached to this instruction */
  if (*label) {
      SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);
  }

  // I have to use the Guard mechanism to remember which of the two MOVEM
  // instructions was successful in pass 1 so that we don't produce unwanted
  // error messages in pass 2 by trying the wrong instruction form.
  // Once I tackle opParse to re-write it (to allow more precise error locations
  // within a line by reporting the line position as well) I may include the
  // parsing of the register list in opParse and rewrite this whole module.
  
  if (giPass<2 || GuardGet(0)==GUARD_REGLIST_LEFT)
  {
    /* See if the instruction is of the form MOVEM <reg_list>,<ea> */
    /* Parse the register list */
    p = evalList(op, &regList);
    if (ErrorStatusIsOK() && *p == ',') {
        /* Parse the memory address */
        p = opParse(++p, &memOp, 2, false);
        if (!ErrorStatusIsError()) {
            /* Check legality of addressing mode */
            if (memOp.mode & DestModes) {
                /* It's good, now generate the instruction */
                if (giPass==2) {
                    output((long int) (instMask | effAddr(&memOp)), WORD);
                    gulOutLoc += 2;
                    /* If the addressing mode is address
                       register indirect with predecrement,
                       reverse the bits in the register
                       list mask */
                    if (memOp.mode == AnIndPre) {
                        temp = regList;
                        regList = 0;
                        for (i = 0; i < 16; i++) {
                            regList <<= 1;
                            regList |= (temp & 1);
                            temp >>= 1;
                        }
                    }
                    output((long) regList, WORD);
                    gulOutLoc += 2;
                } else {
					if (giPass==1)
	                    Guard(GUARD_REGLIST_LEFT,0); // remember that we found MOVEM <reg_list>,<ea>
                    gulOutLoc += 4;
                }
                extWords(&memOp, size);
                return NORMAL;
            } else {
                Error(INV_ADDR_MODE,NULL);
                return NORMAL;
            }
        }
    }
  }
    
  /* See if the instruction is of the form MOVEM <ea>,<reg_list> */
  /* Parse the effective address */
  ErrorStatusReset();
  p = opParse(op, &memOp, 1, false);
  if (!ErrorStatusIsError() && *p == ',') {
      /* Check the legality of the addressing mode */
      if (memOp.mode & SourceModes) {
          /* Parse the register list */
          p = evalList(++p, &regList);
          if (ErrorStatusIsOK()) {
              /* Everything's OK, now build the instruction */

              if (giPass==2) {
                  output((long) (instMask | 0x0400 | effAddr(&memOp)), WORD);
                  gulOutLoc += 2;
                  output((long) (regList), WORD);
                  gulOutLoc += 2;
              } else {
                  if (giPass==1)
					  Guard(GUARD_REGLIST_RIGHT,0); // remember that we found MOVEM <ea>,<reg_list>
                  gulOutLoc += 4;
              }
              extWords(&memOp, size);
              return NORMAL;
          }
      } else {
          Error(INV_ADDR_MODE,NULL);
          return NORMAL;
      }
  }

  return NORMAL;
}


int reg(int size, char *label, char *op)
{
    unsigned short regList;

    if (size) {
        Error(INV_SIZE_CODE,NULL);
    }
    if (!*op) {
        Error(SYNTAX,NULL);
        return NORMAL;
    }
    op = evalList(op, &regList);
    if (!ErrorStatusIsSevere()) {
        if (!*label) {
            Error(LABEL_REQUIRED,NULL);
        } else {
            SymbolCreate(label,symbolKindRegList,NULL,(long)regList);
        }
    }

    return NORMAL;
}


/* Define a couple of useful tests */

#define isTerm(c)   (c == ',' || c == '/' || c == '-' || ISSPACE(c) || !c)
#define isRegNum(c) ((c >= '0') && (c <= '7'))


char *evalList(char *p, unsigned short *listPtr)
{
    char reg1, reg2, r;
    unsigned short regList;
    char szT[strlen(p)+1];
    char *pOrig;

    pOrig = p;
    strcap(szT, p);
    p = szT;

    *listPtr = 0;
    regList = 0;
    /* Check whether the register list is specified
       explicitly or as a register list symbol */
    if ((p[0] == 'A' || p[0] == 'D') && isRegNum(p[1]) && isTerm(p[2])) {
        /* Assume it's explicit */
        while (true) {  /* Loop will be exited via return */
            if ((p[0] == 'A' || p[0] == 'D') && isRegNum(p[1])) {
                if (p[0] == 'A')
                    reg1 = (char)(8) + p[1] - '0';
                else
                    reg1 = p[1] - '0';
                if (p[2] == '/') {
                    /* Set the bit the for a single register */
                    regList |= (1 << reg1);
                    p += 3;
                } else if (p[2] == '-') {
                    if ((p[3] == 'A' || p[3] == 'D') && isRegNum(p[4]) && isTerm(p[5])) {
                        if (p[5] == '-') {
                            Error(SYNTAX,pOrig+5);
                            return NULL;
                        }
                        if (p[3] == 'A') {
                            reg2 = (char)(8) + p[4] - '0';
                        } else {
                            reg2 = p[4] - '0';
                        }
                        /* Set all the bits corresponding to registers
                           in the specified range */
                        if (reg1 < reg2) {
                            for (r = reg1; r <= reg2; r++) {
                                regList |= (1 << r);
                            }
                        } else {
                            for (r = reg2; r <= reg1; r++) {
                                regList |= (1 << r);
                            }
                        }
                        if (p[5] != '/') {
                            /* End of register list found - return its value */
                            *listPtr = regList;
                            return pOrig+(p-szT)+5;
                        }
                        p += 6;
                    } else {
                        /* Invalid character found - return the error */
                        Error(SYNTAX,pOrig+3);
                        return NULL;
                    } 
                } else {
                    /* Set the bit the for a single register */
                    regList |= (1 << reg1);
                    /* End of register list found - return its value */
                    *listPtr = regList;
                    return pOrig+(p-szT)+2;
                }
            } else {
                /* Invalid character found - return the error */
                Error(SYNTAX,pOrig);
                return NULL;
            }
        }
    }
    else if (ISALPHA(*p) || *p=='_' || *p=='?' || *p=='@')
    {
        /* Try looking in the symbol table for a register list symbol */
        char id[SIGCHARS+1];
        SymbolDef *symbol;

        p = ParseId(p,id);
        symbol = SymbolLookupScopeProc(id);
        if (!symbol)
          symbol = SymbolLookup(id);
        if (symbol==NULL)
          Error(REG_LIST_UNDEF,id);
        else 
        {
          if (SymbolGetKind(symbol)==symbolKindProcEntry && *p=='.')
          {
            p = ParseId(p+1,id);
            symbol = SymbolLookupScopeSymbol(symbol,id);
          }
          if (symbol==NULL)
            Error(REG_LIST_UNDEF,id);
          else if (SymbolGetKind(symbol)!=symbolKindRegList)
            Error(NOT_REG_LIST,SymbolGetId(symbol));
          else
            *listPtr = (unsigned short)SymbolGetValue(symbol);
        }
        return pOrig+(p-szT);
    }
    else
    {
      Error(SYNTAX,p);
      return NULL;
    }

    return NORMAL;
}

