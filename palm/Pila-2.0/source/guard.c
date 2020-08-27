/**********************************************************************
 *
 *      GUARD.C
 *
 *      This module allows the storage of information for a specific
 *      source line. It is used i.e. to record long or short jumps to
 *      make sure no changes are done between pass 2 and pass 3
 *      even if it were possible or to remember which operand of a
 *      movem instruction is the register list (to suppress error
 *      messages on pass 3 since it is evaluated via trial and
 *      error).
 *
 *      The information is stored in a special symbol. The symbol's
 *      name is built from the current filename, the line number of
 *      the currently processed source line, the number of the
 *      current expand line and a passed in sub-id.
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Created for Pila Version 2.0
 *
 *********************************************************************/
#include "pila.h"
#include "asm.h"
#include "symbol.h"
#include "expand.h"

extern int giPass;

boolean Guard(long value,int subId)
{
  boolean ret = false;
  if (giPass)
  {
    SymbolDef *symbol;
    char guardId[512];
    sprintf(guardId,":guard:%s:%d:%d:%d",gpsseCur->szFile,gpsseCur->iLineNum,ExpandGetLineNum(),subId);
    if (giPass<2)
      SymbolCreate(guardId,symbolKindGuard,NULL,value);
    else
    {
      symbol = SymbolLookup(guardId);
      if (!symbol)
      {
        Error(INTERNAL_ERROR_GUARD_NOT_DEF,guardId);
        ret = true;
      }
      else if (SymbolGetValue(symbol)!=value)
      {
        Error(GUARD_ERROR,guardId);
        ret = true;
      }
    }
  }
  return ret;
}

long GuardGet(int subId)
{
  long ret = 0;
  SymbolDef *symbol;
  char guardId[512];
  
  sprintf(guardId,":guard:%s:%d:%d:%d",gpsseCur->szFile,gpsseCur->iLineNum,ExpandGetLineNum(),subId);
  symbol = SymbolLookup(guardId);
  if (!symbol)
  {
    ret = 0;
    Error(INTERNAL_ERROR_GUARD_NOT_DEF,guardId);
  }
  else
    ret = SymbolGetValue(symbol);

  return ret;
}
