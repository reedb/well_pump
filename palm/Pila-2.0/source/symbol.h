/***********************************************************************
 *
 *      SYMBOL.H
 *      Typedefs etc for Symbol Handling Routines for 68000 Assembler
 *
 *      Inspired by original work by Wes Cherry found in pp.c
 *
 *      Author: F. Schaeckermann (frmipg602@sneakemail.com)
 *      Date: 2003-07-23
 *
 ************************************************************************/

#ifndef __SYMBOL3_H__
#define __SYMBOL3_H__

#define KIND_LIST \
  KI(symbolKindUndefined,symbolCategoryNone)		\
  KI(symbolKindProcDef,symbolCategoryNone)		\
  KI(symbolKindProcEntry,symbolCategoryCode)		\
  KI(symbolKindProcLocal,symbolCategoryStack)		\
  KI(symbolKindProcLabel,symbolCategoryCode)		\
  KI(symbolKindProcParm,symbolCategoryStack)		\
  KI(symbolKindRegList,symbolCategoryNone)		\
  KI(symbolKindProxyEntry,symbolCategoryCode)		\
  KI(symbolKindTrapDef,symbolCategoryNone)		\
  KI(symbolKindCode,symbolCategoryCode)			\
  KI(symbolKindLabel,symbolCategoryNone)		\
  KI(symbolKindData,symbolCategoryData)			\
  KI(symbolKindRes,symbolCategoryRes)			\
  KI(symbolKindConst,symbolCategoryConst)		\
  KI(symbolKindGuard,symbolCategoryNone)		\
  KI(symbolKindExtern,symbolCategoryNone)		\
  KI(symbolKindInclude,symbolCategoryNone)		\
  KI(symbolKindMacro,symbolCategoryNone)		\
  KI(symbolKindTypeProc,symbolCategoryType)		\
  KI(symbolKindTypeProxy,symbolCategoryType)		\
  KI(symbolKindTypeTrapSimple,symbolCategoryType)	\
  KI(symbolKindTypeTrapSelector,symbolCategoryType)	\
  KI(symbolKindTypeTrap16BitSel,symbolCategoryType)	\
  KI(symbolKindTypeEnum,symbolCategoryType)		\
  KI(symbolKindTypeStruct,symbolCategoryType)		\
  KI(symbolKindTypeUnion,symbolCategoryType)		\
  KI(symbolKindTypeBitmapMember,symbolCategoryType)	\
  KI(symbolKindTypeMemberList,symbolCategoryType)	\
  KI(symbolKindTypePointer,symbolCategoryType)		\
  KI(symbolKindTypeArray,symbolCategoryType)		\
  KI(symbolKindTypeAlias,symbolCategoryType)		\
  KI(symbolKindTypeSimple,symbolCategoryType)

// Definitions of  Symbol Category
typedef enum _SymbolCategory
{
  symbolCategoryNone = 0,
  symbolCategoryCode,
  symbolCategoryData,
  symbolCategoryStack,
  symbolCategoryRes,
  symbolCategoryConst,
  symbolCategoryType,
} SymbolCategory;
 
// Definitions of Symbol Kinds
#define KI(a,b) a,
typedef enum _SymbolKind
{
  KIND_LIST
} SymbolKind;
#undef KI

// Value Definition
typedef struct
{
  long               value;	// value of this symbol
  SymbolKind	     kind;	// value kind
  struct _SymbolDef *type;	// type of value (b, w, l, ...)
} Value;

// Symbol Definition
typedef struct _SymbolDef
{
  // the following entry MUST BE THE FIRST ENTRY!!! (SEE FUNCTION SymbolAdd)
  struct _SymbolDef  *next;	// for procEntry and typeXXX symbols this is a list of sub-members
				// all other symbols are just members of such a list
  char		         *id;	// name of the symbol
  Value		       value;	// symbol's value
  struct _SymbolDef *derived;	// list of derived types
  boolean       redefineable;	// true if the symbol was created by the set directive
} SymbolDef;

void       SymbolInitialize();
SymbolDef *SymbolSetCurrentProc(SymbolDef *proc);
boolean    SymbolHasCurrentProc();

SymbolDef *SymbolFactory(char         *id,
                         SymbolKind  kind,
                         SymbolDef  *type,
                         long       value);

SymbolDef *SymbolCreate(char         *id,
                        SymbolKind  kind,
                        SymbolDef  *type,
                        long       value);

SymbolDef *SymbolCreateScopeProc(char         *id,
                                 SymbolKind  kind,
                                 SymbolDef  *type,
                                 long       value);

SymbolDef *SymbolCreateParameter(SymbolDef *parmList,
                                 SymbolKind     kind,
                                 int          parmNo,
                                 char            *id,
                                 SymbolDef     *type);

SymbolDef *SymbolCreateDerivedType(SymbolDef *baseType,
                                   SymbolKind     kind,
                                   long          value);

SymbolDef *SymbolCreateTypeMember(SymbolDef *target,
                                  char          *id,
                                  SymbolDef   *type,
                                  long        value);

SymbolDef     *SymbolLookupScopeSymbol(SymbolDef *symbol, char *id);
SymbolDef     *SymbolLookupScopeProc(char *id);
SymbolDef     *SymbolLookup(char *id);

SymbolDef     *SymbolCreateTempLabel(char id);
SymbolDef     *SymbolLookupTempLabel(char id,char direction);

SymbolDef     *SymbolGetNext(SymbolDef *symbol);
char          *SymbolGetId(SymbolDef *symbol);
int            SymbolGetSize(SymbolDef *symbol);

long           SymbolGetValue(SymbolDef *symbol);
void           SymbolSetValue(SymbolDef *symbol, Value *value);

SymbolKind     SymbolGetKind(SymbolDef *symbol);
/* void        SymbolSetKind(SymbolDef *symbol, SymbolKind kind); */
SymbolCategory SymbolGetCategory(SymbolKind kind);

SymbolDef     *SymbolGetType(SymbolDef *symbol);
void           SymbolSetType(SymbolDef *symbol,SymbolDef *type);

boolean        SymbolGetRedefineable(SymbolDef *symbol);
void           SymbolSetRedefineable(SymbolDef *symbol);

/* void           SymbolDestroy(SymbolDef *symbol); */

#endif // __SYMBOL3_H__
