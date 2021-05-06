/***********************************************************************
 *
 *      SYMBOL.C
 *      Symbol Handling Routines for 68000 Assembler
 *
 *      Inspired by the original works of
 *        Wes Cherry (found in the (now deleted) pp.c) and
 *        Paul McKee (found in the (former) symbol.c)
 *
 *      Author: F. Schaeckermann (frmipg602@sneakemail.com)
 *      Date: 2003-07-23
 *
 * *** NOTE ***
 *
 * Don't spend too much energy understanding the code in this file since
 * the next version of Pila will have a much reworked symbol handling
 * and a generalized concept of symbol context!!!!
 *
 * The idea is to have a logical and physical context for each symbol.
 *
 * Logical context could be the structure or union a symbol belongs to,
 * the parameter list a symbol is defined in, the procedure a label or
 * local variable is defined in (local variables would actually be created
 * within the context of the parameter list of the procedure they belong to
 * since that would not be accessible from outside of the procedure
 * and therefore accessing the parameters or local variables of a procedure
 * from outside the procedure will not be possible anymore - not even by
 * prefixing the symbol name with the procedure's name since that would only
 * give you the context of the procedure but not of it's parameter list).
 *
 * The physical context is always the file a symbol is defined in. By
 * prefixing the symbol name in its definition with a . (dot) you can bind
 * a symbol to it's physical context (if it doesn't have a logical context)
 * and thus make it visible only within the boundaries of the source file
 * it's definition appears in.
 * This greatly improves the ability (in fact it makes it at last possible)
 * to write modular code not having to worry about name-clashes with other
 * included source files. To reference such a 'local' symbol you won't need
 * to specify the dot in front of it since the symbol lookup will always
 * follow the sequence of physical context, logical context, global (or
 * no) context.
 *
 ************************************************************************/

#define _INCLUDE_SYMBOL_DEF_STRUCT_

#include "pila.h"
#include "asm.h"
#include "symbol.h"
#include "safe-ctype.h"
#include "libiberty.h"

extern int  giPass;         /* The assembler's pass counter */
extern long gulOutLoc;      /* The assembler's location counter */

SymbolDef *symbolCurrentProcedure;

#define MAXHASH 1024
SymbolDef *symbolHashTable[MAXHASH+1];

int tempLabelPass = -1;
int tempLabelCounter[9];

void breakpoint()
{
//  printf("\nbreakpoint\n");
}

long checkCounter = 0;
SymbolDef *checkSymbol = NULL;
SymbolDef *check(SymbolDef *symbol)
{
  checkCounter++;
  printf("%d-%ld ",giPass,checkCounter);

  if (checkCounter>=125957)
    breakpoint();

  if (symbol && symbol->id && strcmp(symbol->id,"DmNewHandle")==0)
  {
    printf("\n%d-%ld %s identified ",giPass,checkCounter,symbol->id);
    if (checkSymbol && checkSymbol!=symbol)
    {
      printf("boom - redefined\n");
      exit(2000);
    }
    if (checkSymbol && checkSymbol->value.type!=symbol->value.type)
      printf("type redefined ");
    checkSymbol = symbol;
  }
  
  if (checkSymbol)
  {
    if (!checkSymbol->value.type)
      printf("no type yet\n");
    else if (!checkSymbol->value.type->id)
      printf("no type id yet\n");
    else if (strcmp(checkSymbol->value.type->id,"MemHandle")==0)
      printf("passed\n");
    else
    {
      printf("boom - invalid type\n");
      exit(2000);
    }
  }
  
  return symbol;
}
#define check(x) x

/**********************************************************************/
/* Routine: SymbolInitialize                                          */
/*   Initializing hash table                                          */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*     void                                                           */
/* Returns:                                                           */
/*     void                                                           */
/**********************************************************************/
void SymbolInitialize()
{
  int i;
  symbolCurrentProcedure = NULL;
  for (i=0; i<=MAXHASH; i++)
    symbolHashTable[i] = NULL;
  SymbolCreate("void",symbolKindTypeSimple,NULL,0);
  SymbolCreate("int",symbolKindTypeSimple,NULL,2);
  SymbolCreate("float",symbolKindTypeSimple,NULL,4);
  SymbolCreate("double",symbolKindTypeSimple,NULL,8);
  SymbolCreate("char",symbolKindTypeSimple,NULL,1);
  SymbolCreate("b",symbolKindTypeSimple,NULL,1);
  SymbolCreate("w",symbolKindTypeSimple,NULL,2);
  SymbolCreate("l",symbolKindTypeSimple,NULL,4);
  SymbolCreate("d",symbolKindTypeSimple,NULL,8);
}


/**********************************************************************/
/* Routine: SymbolSetCurrentProc                                      */
/*   Setting the symbol of the currently worked on procedure.         */
/*   This symbol is used as the context for parameters, local         */
/*   variables and local labels.                                      */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*     proc - pointer to symbol representing current procedure        */
/* Returns:                                                           */
/*     void                                                           */
/**********************************************************************/
SymbolDef *SymbolSetCurrentProc(SymbolDef *proc)
{
  SymbolDef *save = symbolCurrentProcedure;
  symbolCurrentProcedure = proc;
  return save;
}


/**********************************************************************/
/* Routine: SymbolHasCurrentProc                                      */
/*   Returns true if there is a currently worked on procedure.       */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*     void                                                           */
/* Returns:                                                           */
/*     boolean                                                           */
/**********************************************************************/
boolean SymbolHasCurrentProc()
{
  return (symbolCurrentProcedure!=NULL);
}


/**********************************************************************/
/* Routine: SymbolGetCategory                                         */
/*   Returning category of a given kind                               */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*     kind - kind identifer to be translated                         */
/* Returns:                                                           */
/*     SymbolCategory                                                 */
/**********************************************************************/

#define KI(a,b) b,
SymbolCategory categoryMap[] = {KIND_LIST};
#undef KI

SymbolCategory SymbolGetCategory(SymbolKind kind)
{
  return categoryMap[kind];
}
		
/**********************************************************************/
/* Routine: SymbolHashCode                                            */
/*   Calculates a hash code from the symbol id                        */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     hash code                                                      */
/**********************************************************************/
int SymbolHashCode(char *id)
{
    short sum = 0;
    while (*id)
    {
        sum += (ISUPPER(*id)) ? (*id - 'A') : MAXHASH;
        id++;
    }
    return sum%((short)(MAXHASH+1));
}

/**********************************************************************/
/* Routine: SymbolFactory                                             */
/*   Allocate and initialize symbol structure                         */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - pointer to symbol name                                  */
/*     kind - kind spec of symbol                                     */
/*     type - pointer to symbol representing this symbol's type       */
/*            (can be NULL)                                           */
/*    value - the symbol's value (not used for type symbols)          */
/* Returns:                                                           */
/*     SymbolDef * - Pointer to newly allocated structure             */
/**********************************************************************/
SymbolDef * SymbolFactory(char        *id,
                          SymbolKind kind,
                          SymbolDef *type,
                          long      value)
{
  SymbolDef *symbolPtr = xmalloc(sizeof(SymbolDef));

  if (id && strcmp("DmNewHandle",id)==0)
    breakpoint();

  if (symbolPtr)
  {
    if (id!=NULL)
      symbolPtr->id = xstrdup(id);
    else
      symbolPtr->id = NULL;
      
    symbolPtr->next         = NULL;
    symbolPtr->value.value  = value;
    symbolPtr->value.kind   = kind;
    symbolPtr->value.type         = check(type);
    symbolPtr->derived      = NULL;
    symbolPtr->redefineable = false;
  }
    
  return check(symbolPtr);
}

/**********************************************************************/
/* Routine: SymbolAdd                                                 */
/*   adding a new symbol to a list                                    */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*  listPtr - pointer to first symbol of list to add to               */
/*   sorted - true if the list is sorted by symbol name               */
/*       id - pointer to symbol name                                  */
/*     kind - kind spec of symbol                                     */
/*     type - pointer to symbol representing this symbol's type       */
/*            (can be NULL)                                           */
/*    value - the symbol's value (not used for type symbols)          */
/* Returns:                                                           */
/*     pointer to newly created symbol                                */
/**********************************************************************/
SymbolDef *SymbolAdd(SymbolDef **listPtr,
                     boolean      sorted,
                     char            *id,
                     SymbolKind     kind,
                     SymbolDef     *type,
                     long          value)
{
  int cmp;
  SymbolDef *symbolPtr  = NULL;
  SymbolDef *lastSymbol = (SymbolDef *)listPtr; // THIS IS WHY NEXT MUST BE FIRST ENTRY IN SymbolDef!!!
  SymbolDef *listSymbol = check(*listPtr);

  // now go off and look for the string pointed to by id as the symbol id
  cmp = -1;
  while (listSymbol && !symbolPtr && (!sorted || cmp<0))
  {
    cmp = strcmp(listSymbol->id,id);
    if (cmp==0)
      symbolPtr = listSymbol;
    else if (cmp<0 || !sorted)
    {
      lastSymbol = listSymbol;
      listSymbol = listSymbol->next;
    }
  }
  
  if (symbolPtr)
  {
    if (kind==symbolKindProcDef || 	// any symbol that might have been created
        kind==symbolKindProcEntry ||	// implicitly by a referencing call statement?
        kind==symbolKindProxyEntry ||
        kind==symbolKindTrapDef)
    {
      SymbolKind symKind = symbolPtr->value.kind;
      if (((kind==symbolKindProcDef || kind==symbolKindProcEntry)
             && symKind!=symbolKindProcDef && symKind!=symbolKindProcEntry
          ) ||
          ((kind==symbolKindProxyEntry || kind==symbolKindTrapDef)
             && symKind!=kind && symKind!=symbolKindProcDef
          )
         )
      {
        Error(KIND_DIFFERENT,id);
        symbolPtr = SymbolFactory(id,kind,type,value); // return dummy, unconnected symbol
      }
      else
      {
        if (giPass==2 && type && symbolPtr->value.type!=type)
          Error(PHASE_ERROR,id);
          
        if (kind==symbolKindProcEntry || kind==symbolKindProxyEntry)
        {
          if (giPass==2 && symbolPtr->value.value!=value)
            Error(PHASE_ERROR,id);
          symbolPtr->value.value = value;
          symbolPtr->value.kind = kind;
        }
        
        if (type)
          symbolPtr->value.type = type;
      }
    }
    else if (symbolPtr->value.kind!=kind) // using the same id for two different symbol kinds?
    {
      Error(KIND_DIFFERENT,id);
      symbolPtr = SymbolFactory(id,kind,type,value); // return dummy, unconnected symbol
    }
    else
    {
      if (giPass==2 && !symbolPtr->redefineable &&
          (symbolPtr->value.value!=value || (type && symbolPtr->value.type!=type)))
        Error(PHASE_ERROR,id);
        
      symbolPtr->value.value = value;
      if (type)
        symbolPtr->value.type = type;
    }
  }
  else
  {
    symbolPtr = SymbolFactory(id,kind,type,value);
    symbolPtr->next  = lastSymbol->next;
    lastSymbol->next = symbolPtr;
  }
  return symbolPtr;
}
                           
                           
/**********************************************************************/
/* Routine: SymbolCreateScopeProc                                     */
/*   creating a new symbol in the current procedure name space        */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - pointer to symbol name                                  */
/*     kind - kind specification of this symbol                       */
/*     type - pointer to symbol representing this symbol's type       */
/*            (can be NULL)                                           */
/*    value - the symbol's value (not used for type symbols)          */
/* Returns:                                                           */
/*    pointer to created symbol                                       */
/**********************************************************************/
SymbolDef *SymbolCreateScopeProc(char         *id,
                                 SymbolKind  kind,
                                 SymbolDef  *type,
                                 long       value)
{
  SymbolDef *symbolPtr = NULL;
  long min;
  boolean createSymbol = true;

  if (symbolCurrentProcedure)
  {
    symbolPtr = symbolCurrentProcedure->value.type->next; // ptr to first of proc-member-symbols
    switch (kind)
    {
      case symbolKindCode:
      case symbolKindLabel:
      case symbolKindProcLabel:
        kind = symbolKindProcLabel;
        if (check(type)!=NULL)
          Error(TYPE_IGNORED,type->id);
        type = NULL;
        break;
        
      case symbolKindProcLocal:
        if (check(type))
        {
          min = 0;
          while (symbolPtr && strcmp(symbolPtr->id,id)!=0)
          {
            if (symbolPtr->value.kind==symbolKindProcLocal && symbolPtr->value.value<min)
              min = symbolPtr->value.value;
            symbolPtr = symbolPtr->next;
          }
          value = min-SymbolGetSize(type);
          if (value&1)
           value -= 1; // make sure it is an even offset
        }
        else
        {
          Error(MISSING_TYPE_SPEC,id);
          createSymbol = false;
        }
        break;
        
      case symbolKindRegList:
        symbolPtr = symbolCurrentProcedure->value.type;
        symbolPtr->id = xstrdup(id);
        createSymbol = false;
        break;
      default:
        Error(INTERNAL_ERROR_SYMBOL_KIND,id);
        createSymbol = false;
        break;
    }
    
    if (createSymbol)
      symbolPtr = SymbolAdd((SymbolDef **)(symbolCurrentProcedure->value.type),false,id,kind,type,value);
    else
      symbolPtr = NULL;
  }
  else
    Error(INTERNAL_ERROR_NO_CURR_PROC,id);

  return check(symbolPtr);
}

/**********************************************************************/
/* Routine: SymbolCreateParameter                                     */
/*   creating a new parameter in the passed in parameter list symbol  */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - pointer to symbol name                                  */
/*     kind - kind specification of this symbol                       */
/*     type - pointer to symbol representing this symbol's type       */
/*            (can be NULL)                                           */
/*    value - the symbol's value (not used for type symbols)          */
/* Returns:                                                           */
/*    pointer to created symbol                                       */
/**********************************************************************/
SymbolDef *SymbolCreateParameter(SymbolDef *parmList,
                                 SymbolKind     kind,
                                 int          parmNo,
                                 char            *id,
                                 SymbolDef     *type)
{
  SymbolDef *symbolPtr = check(check(parmList)->next); // Ptr to first symbol
  long value = 8; // first parameter will have to have offset 8
  long max;
  
  if (parmList->value.kind==symbolKindTypeProxy)
    value = 0; // for proxies the first parameter is found at offset 0 (using a0 as base)
  else
    value = 8; // for all others the first parameter is found at offset 8 (using a6 as base)

  while (symbolPtr && --parmNo>0)
  {
    if (symbolPtr->value.kind==symbolKindProcParm)
    {
      max = symbolPtr->value.value+SymbolGetSize(symbolPtr);
      if (max&1) // ensure even value
        max = max+1;
      if (max>value)
        value = max;
      symbolPtr = symbolPtr->next;
    }
    else
      symbolPtr = NULL;
  }
  if (symbolPtr)
  {
    if (kind==symbolKindProcEntry || kind==symbolKindProxyEntry)
    {
      // update parameter names of symbols possibly created implicitly by call directive or via procdef
      if (symbolPtr->id==NULL)
        symbolPtr->id = xstrdup(id);
      else if (strcmp(symbolPtr->id,id)!=0)
      {
        free(symbolPtr->id);
        symbolPtr->id = xstrdup(id);
      }
    }
    if (giPass==2 && (symbolPtr->value.value!=value || symbolPtr->value.type!=type))
      Error(PHASE_ERROR,id);
    symbolPtr->value.value = value;
    symbolPtr->value.type        = check(type);
    return check(symbolPtr);
  }
  else
  {
    // for proxies the value of parmList holds the offset of the placeholder for the return address
    // this place holder lays before the last parameter value
    if (kind==symbolKindProxyEntry)
    {
      parmList->value.value = value+SymbolGetSize(type);
      if (parmList->value.value&1) // make sure it is an even number
        parmList->value.value++;
    }
      
    return SymbolAdd((SymbolDef **)(parmList),false,id,symbolKindProcParm,type,value);
  }
}

/**********************************************************************/
/* Routine: SymbolCreateDerivedType                                   */
/*   creating a new type symbol as a derivation of a given type       */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*  baseType - pointer to symbol representing the base type           */
/*     kind  - kind specification of this symbol                      */
/*    value  - the symbol's value (not used for type symbols)         */
/* Returns:                                                           */
/*    pointer to created symbol                                       */
/**********************************************************************/
SymbolDef *SymbolCreateDerivedType(SymbolDef *baseType,
                                   SymbolKind     kind,
                                   long          value)
{
  char typeId[255+SIGCHARS];
  if (kind==symbolKindTypePointer)
  {
    strcpy(typeId,check(baseType)->id);
    strcat(typeId,"*");
  }
  else if (kind==symbolKindTypeArray)
  {
    sprintf(typeId,"%s[%0ld]",check(baseType)->id,value);
  }
  else
  {
    Error(INTERNAL_ERROR_INVALID_DERIVATION_TYPE,check(baseType)->id);
    return NULL;
  }

  return SymbolAdd(&(baseType->derived),false,typeId,kind,baseType,value);
}


/**********************************************************************/
/* Routine: SymbolCreate                                              */
/*   creating a new symbol                                            */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - pointer to symbol name                                  */
/*     kind - kind specification of this symbol                       */
/*     type - pointer to symbol representing this symbol's type       */
/*            (can be NULL)                                           */
/*    value - the symbol's value (not used for type symbols)          */
/* Returns:                                                           */
/*    pointer to created symbol                                       */
/**********************************************************************/
SymbolDef *SymbolCreate(char          *id,
                        SymbolKind   kind,
                        SymbolDef   *type,
                        long        value)
{
  if (kind==symbolKindLabel)
  {
    switch (gbt)
    {
      case kbtData:     kind = symbolKindData; break;
      case kbtCode:     kind = symbolKindCode; break;
      case kbtResource: kind = symbolKindRes;  break;
    }
  }
  
  if (symbolCurrentProcedure &&
      ((SymbolGetCategory(kind)==symbolCategoryCode && 
        strcmp(id,symbolCurrentProcedure->id)!=0) ||
       kind==symbolKindRegList
     ))
  {
    return SymbolCreateScopeProc(id,kind,check(type),value);
  }
  else
  {
    int hash = SymbolHashCode(id);
    return SymbolAdd(&symbolHashTable[hash],true,id,kind,check(type),value);
  }
}


/**********************************************************************/
/* Routine: SymbolCreateTypeMember                                    */
/*   Adding a new type member                                         */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   target - pointer to type symbol the member is to be added to     */
/*       id - pointer to symbol name                                  */
/*     kind - kind of symbol                                          */
/*     type - pointer to symbol representing this symbol's type       */
/*            (can be NULL)                                           */
/*    value - the symbol's value (not used for type symbols)          */
/* Returns:                                                           */
/*     pointer to newly created member                                */
/**********************************************************************/
SymbolDef *SymbolCreateTypeMember(SymbolDef *target,
                                  char          *id,
                                  SymbolDef   *type,
                                  long        value
                                 )
{
  int first = 1;
  SymbolKind targetKind = target->value.kind;
  SymbolDef *symbolPtr  = NULL;
  SymbolDef *lastSymbol = NULL;
  SymbolDef *listSymbol = NULL;

  if (targetKind==symbolKindTypeEnum ||
      targetKind==symbolKindTypeStruct ||
      targetKind==symbolKindTypeUnion)
  {
    // for membered types we use the type's type to store the members
    // since they can be part of the symbol-list themselves
    lastSymbol = target->value.type;
  }
  else if (targetKind==symbolKindTypeBitmapMember)
  {
    // for bitmap members we use the type directly since this
    // type will never be reachable through the symbol-list.
    lastSymbol = target;
  }
    
  if (lastSymbol)
  {
    listSymbol = lastSymbol->next;
    
    // now go off and look for the string pointed to by id as the symbol id
    while (listSymbol && !symbolPtr)
    {
      first = 0;
      if (strcmp(listSymbol->id,id)==0)
        symbolPtr = listSymbol;
      else
      {
        lastSymbol = listSymbol;
        listSymbol = listSymbol->next;
      }
    }
  
    if (!symbolPtr)
    {
      symbolPtr = SymbolFactory(id,symbolKindConst,type,value);
      symbolPtr->next  = lastSymbol->next;
      lastSymbol->next = symbolPtr;
    }
    else
    {
      if (giPass==0)
        Error(MULTIPLE_DEFS,id);
      // symbol exists already... but it's type may have changed
      symbolPtr->value.type = type;
    }
    
    if (targetKind==symbolKindTypeEnum)
    {
      // for enums the enum-type is itself the type of each of its members!
      symbolPtr->value.type = target;
      // create symbol in global list as well */
      SymbolCreate(id,symbolKindConst,target,value);
    }
    else if (targetKind==symbolKindTypeStruct)
    {
      if (first)
        value = 0; // first struct member!
      else
      {
        if (SymbolGetKind(lastSymbol->value.type)==symbolKindTypeBitmapMember && 
            SymbolGetValue(lastSymbol->value.type)>0 &&
            SymbolGetKind(type)==symbolKindTypeBitmapMember)
        {
          // if last symbol was a bitmap member and it wasn't the last one
          // (shift-value>0) and the new symbol is a bitmap member as well,
          // then the value of this one is the same as the one before.
          value = lastSymbol->value.value;
        }
        else
        {
          // if the new symbol is not a bitmap member or the last bitmap structure
          // was complete, this member gets a new address pointing behind the last one.
          value = lastSymbol->value.value+SymbolGetSize(lastSymbol->value.type);
        }
          
        if (SymbolGetSize(type)>1 && (value&1))
          value++;
      }
    }
    else if (targetKind==symbolKindTypeUnion)
      value = 0; // in a union each member has offset 0
      
    if (giPass==2 && symbolPtr->value.value!=value)
      Error(PHASE_ERROR,id);

    symbolPtr->value.value = value;
  }
  else
    Error(INTERNAL_ERROR_INVALID_MEMBERED_TYPE,id);
    
  return check(symbolPtr);
}


/**********************************************************************/
/* Routine: SymbolRetrieveFromList                                    */
/*   retrieving a symbol from a list of symbols                       */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*    first - pointer to first symbol in list                         */
/*   sorted - true if list is sorted, false otherwise                 */
/*       id - pointer to symbol name                                  */
/* Returns:                                                           */
/*     pointer to symbol if found, NULL otherwise                     */
/**********************************************************************/
SymbolDef *SymbolRetrieveFromList(SymbolDef *first, boolean sorted, char *id)
{
  int cmp;
  SymbolDef *symbolPtr = NULL;

  // now go off and look for the string pointed to by id as the symbol id
  while (check(first) && !symbolPtr)
  {
    cmp = strcmp(first->id,id);
    if (cmp==0)
      symbolPtr = first;
    else if (cmp>0 && sorted)
      first = NULL;
    else
      first = first->next;
  }
  
  return check(symbolPtr);
}


/**********************************************************************/
/* Routine: SymbolLookupScopeSymbol                                   */
/*   looking up a symbol in it's type's member list                   */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol inside which to look                  */
/*       id - pointer to symbol name                                  */
/* Returns:                                                           */
/*    pointer to fouund symbol, NULL if not found                     */
/**********************************************************************/
SymbolDef *SymbolLookupScopeSymbol(SymbolDef *symbol, char *id)
{
  SymbolKind kind;

  // if the symbol is not a type symbol get it's type
  if (symbol && SymbolGetCategory(symbol->value.kind)!=symbolCategoryType)
    symbol = symbol->value.type;
    
  // if it is an alias-type resolve it to the original type
  while(symbol && symbol->value.kind==symbolKindTypeAlias)
    symbol = symbol->value.type;
  
  if (symbol)
  {
    // if we still have a symbol to work with check it's kind
    kind = symbol->value.kind;
    if (kind==symbolKindTypeEnum   ||
        kind==symbolKindTypeStruct ||
        kind==symbolKindTypeUnion)
    {
      // in case of a membered type switch to the type
      // and get it's kind if there is a type
      // if not, kind stays what it was and falls through the next test
      symbol = symbol->value.type;
      if (symbol)
        kind = symbol->value.kind;
    }
    
    // now if we finally have a type that has a member list look in that list
    if (kind==symbolKindTypeProc         ||
        kind==symbolKindTypeProxy        ||
        kind==symbolKindTypeTrapSimple   ||
        kind==symbolKindTypeTrapSelector ||
        kind==symbolKindTypeTrap16BitSel ||
        kind==symbolKindTypeBitmapMember ||
        kind==symbolKindTypeMemberList)
    {
      return SymbolRetrieveFromList(symbol->next,false,id);
    }
  }
  return NULL;
}


/**********************************************************************/
/* Routine: SymbolLookupScopeProc                                     */
/*   looking up a symbol in the current procedure name space          */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - pointer to symbol name                                  */
/* Returns:                                                           */
/*    pointer to fouund symbol, NULL if not found                     */
/**********************************************************************/
SymbolDef *SymbolLookupScopeProc(char *id)
{
  return SymbolLookupScopeSymbol(symbolCurrentProcedure,id);
}


/**********************************************************************/
/* Routine: SymbolLookup                                              */
/*   getting a pointer to a symbol (can not be a composite id)        */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - pointer to symbol name                                  */
/* Returns:                                                           */
/*     pointer to symbol if found, NULL otherwise                     */
/**********************************************************************/
SymbolDef *SymbolLookup(char *id)
{
  // retrieve the symbol from the according hash-entry list
  return SymbolRetrieveFromList(symbolHashTable[SymbolHashCode(id)],true,id);
}

/**********************************************************************/
/* Routine: SymbolGetNext                                             */
/*   getting a symbol's linked member                                 */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     symbol's linked member                                         */
/**********************************************************************/
SymbolDef *SymbolGetNext(SymbolDef *symbol)
{
  if (check(symbol))
    return check(symbol->next);
  return NULL;
}

/**********************************************************************/
/* Routine: SymbolGetId                                               */
/*   getting a symbol's id                                            */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     id of symbol                                                   */
/**********************************************************************/
char *SymbolGetId(SymbolDef *symbol)	// get symbol's id
{
  if (check(symbol))
    return symbol->id;
  return "";
}

/**********************************************************************/
/* Routine: SymbolGetSizeOfEnumType                                   */
/*   getting an enum type's size                                      */ 
/*   Loop through all members of this enum to calculate the type      */
/*   size by finding the lowest and highest value and calculating     */
/*   the number of bytes to hold this range                           */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to enum symbol type                             */
/* Returns:                                                           */
/*     size of value of symbol                                        */
/**********************************************************************/
int SymbolGetSizeOfEnumType(SymbolDef *enumType)
{
  long min = 0;
  long max = 0;
  SymbolDef *symbolPtr = enumType->value.type->next;
  
  while (symbolPtr)
  {
    if (min>symbolPtr->value.value)
      min = symbolPtr->value.value;
    else if (max<symbolPtr->value.value)
      max = symbolPtr->value.value;
    symbolPtr = symbolPtr->next;
  }
  
  if (min<-32768 || (min<0 && max>32767) || max>65535)
    return 4;
  else if (min<-128 || (min<0 && max>127) || max>255)
    return 2;
  else
    return 1;
}

/**********************************************************************/
/* Routine: SymbolGetSizeOfStructType                                 */
/*   getting a struct types's size                                    */ 
/*   Loop through all members of this struct to find the last member. */
/*   It's offset plus the length of it's size is the size of the      */
/*   struct.                                                          */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to struct symbol type                           */
/* Returns:                                                           */
/*     size of value of symbol                                        */
/**********************************************************************/
int SymbolGetSizeOfStructType(SymbolDef *structType)
{
  SymbolDef *symbolPtr  = structType->value.type->next;
  SymbolDef *lastSymbol = NULL;
  while (symbolPtr)
  {
    lastSymbol = symbolPtr;
    symbolPtr  = symbolPtr->next;
  }
  if (lastSymbol)
    return lastSymbol->value.value+SymbolGetSize(lastSymbol);
  return 0;
}


/**********************************************************************/
/* Routine: SymbolGetSizeOfUnionType                                  */
/*   getting a union types's size                                     */ 
/*   Loop through all members of this union to calculate the maximal  */
/*   type size of all members. That will be the union's type size.    */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to union symbol type                            */
/* Returns:                                                           */
/*     size of value of symbol                                        */
/**********************************************************************/
int SymbolGetSizeOfUnionType(SymbolDef *unionType)
{
  SymbolDef *symbolPtr = unionType->value.type->next;
  int max = 0;
  int sze = 0;
  while (symbolPtr)
  {
    sze = SymbolGetSize(symbolPtr);
    if (max<sze)
      max = sze;
    symbolPtr = symbolPtr->next;
  }
  return max;
}


/**********************************************************************/
/* Routine: SymbolGetSize                                             */
/*   getting a symbol's size (size of it's type)                      */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     size of value of symbol                                        */
/**********************************************************************/
int SymbolGetSize(SymbolDef *symbol)	// get symbol's size
{
  if (symbol)
  {
    if (SymbolGetCategory(symbol->value.kind)==symbolCategoryType)
    {
      int size;
      switch(symbol->value.kind)
      {
        case  symbolKindTypeEnum:
          return SymbolGetSizeOfEnumType(symbol);
        case  symbolKindTypeStruct:
          return SymbolGetSizeOfStructType(symbol);
        case  symbolKindTypeUnion:
          return SymbolGetSizeOfUnionType(symbol);
        case  symbolKindTypeBitmapMember:
          return SymbolGetSize(symbol->value.type);
        case  symbolKindTypePointer:
          return sizeof(void *);
        case  symbolKindTypeArray:
          if (symbol->value.value==0)
            return 0;
          size = SymbolGetSize(symbol->value.type);
          if (size>1 && size&1)
            size++;
          return symbol->value.value*size;
        case  symbolKindTypeAlias:
          return SymbolGetSize(symbol->value.type);
        case  symbolKindTypeSimple:
          return symbol->value.value;
        default:
          return 0;
      }
    }
    else if (symbol->value.type)
      return SymbolGetSize(symbol->value.type);
  }
  return 0;
}

/**********************************************************************/
/* Routine: SymbolGetKind                                             */
/*   getting a symbol's kind specification                            */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     kind identifier                                                */
/**********************************************************************/
SymbolKind SymbolGetKind(SymbolDef *symbol)
{
  if (check(symbol))
    return symbol->value.kind;
  return symbolKindUndefined;
}

/**********************************************************************/
/* Routine: SymbolGetValue                                            */
/*   getting a symbol's value                                         */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     value of symbol                                                */
/**********************************************************************/
long SymbolGetValue(SymbolDef *symbol)	// get symbol's value
{
  if (check(symbol))
    return symbol->value.value;
  return 0;
}

/**********************************************************************/
/* Routine: SymbolSetValue                                            */
/*   setting a symbol's value and kind                                */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/*    value - new value and kind                                      */
/* Returns:                                                           */
/*     void                                                           */
/**********************************************************************/
void SymbolSetValue(SymbolDef *symbol,Value *value)
{
  if (check(symbol))
  {
    symbol->value.value = value->value;
    symbol->value.kind  = value->kind;
  }
}

/**********************************************************************/
/* Routine: SymbolGetType                                             */
/*   getting a symbol's type                                          */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     pointer to type symbol                                         */
/**********************************************************************/
SymbolDef *SymbolGetType(SymbolDef *symbol)	// get symbol's type
{
  if (check(symbol))
    return symbol->value.type;
  return NULL;
}

/**********************************************************************/
/* Routine: SymbolSetType                                             */
/*   setting a symbol's type                                          */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol                                       */
/*     type - pointer to type symbol                                  */
/* Returns:                                                           */
/*     void                                                           */
/**********************************************************************/
void SymbolSetType(SymbolDef *symbol,SymbolDef *type)
{
  if (check(symbol) && symbol->id && strcmp("DmNewHandle",symbol->id)==0)
    breakpoint();
  symbol->value.type = check(type);
  if (check(symbol))
    ;
}

/**********************************************************************/
/* Routine: SymbolGetRedefineable                                     */
/*   getting a symbol's flag if it is redefineable                    */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol definition                            */
/* Returns:                                                           */
/*     redefineable-flag                                              */
/**********************************************************************/
boolean SymbolGetRedefineable(SymbolDef *symbol)
{
  if (check(symbol))
    return symbol->redefineable;
  return false;
}

/**********************************************************************/
/* Routine: SymbolSetRedefineable                                     */
/*   setting a symbol to be redefineable                              */ 
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*   symbol - pointer to symbol                                       */
/* Returns:                                                           */
/*     void                                                           */
/**********************************************************************/
void SymbolSetRedefineable(SymbolDef *symbol)
{
  if (check(symbol))
    symbol->redefineable = true;
}

/**********************************************************************/
/* Routine: SymbolCreateTempLabel                                     */
/*   Creating a new temporary code label                              */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*       id - symbol number must be '1'..'9'!!!!                      */
/*    value - the symbol's value                                      */
/* Returns:                                                           */
/*     pointer to newly created member                                */
/**********************************************************************/
SymbolDef *SymbolCreateTempLabel(char tempLabelId)
{
  char idName[20];
  if (tempLabelPass!=giPass)
  {
    int counter;
    for (counter=0; counter<9; counter++) tempLabelCounter[counter] = 0;
    tempLabelPass = giPass;
  }
  sprintf(idName,":temp:%c:%08lX",tempLabelId,(long)(++(tempLabelCounter[tempLabelId-'1'])));
  return SymbolCreate(idName,symbolKindCode,NULL,gulOutLoc);
}


/**********************************************************************/
/* Routine: SymbolLookupTempLabel                                     */
/*   Creating a new temporary code label                              */
/*--------------------------------------------------------------------*/
/* Parameters:                                                        */
/*         id - symbol number must be '1'..'9'!!!!                    */
/*  direction - direction must be 'f' (forward) or 'b' (backward)     */
/* Returns:                                                           */
/*     pointer to newly created member                                */
/**********************************************************************/
SymbolDef *SymbolLookupTempLabel(char tempLabelId,char direction)
{
  char idName[20];
  int  counter;
  SymbolDef *symbol;
  
  if (tempLabelPass!=giPass)
  {
    for (counter=0; counter<9; counter++) tempLabelCounter[counter] = 0;
    tempLabelPass = giPass;
  }
  
  if (direction=='B')
    direction = 'b';
  else if (direction=='F')
    direction = 'f';
    
  if (tempLabelId<'1' || tempLabelId>'9' || (direction!='f' && direction!='b'))
    return NULL; // no such temporary label

  counter = tempLabelCounter[tempLabelId-'1'];
  if (direction=='f')
    counter++;
  else if (counter==0)
    return NULL; // no backwards label before the first one is defined
    
  sprintf(idName,":temp:%c:%08lX",tempLabelId,(long)counter);
  
  symbol = SymbolLookupScopeProc(idName);
  if (!symbol)
    symbol = SymbolLookup(idName);

  return symbol;
}


