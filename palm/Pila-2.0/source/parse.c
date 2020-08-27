/**********************************************************************
 *
 *      PARSE.C
 *
 *      Various functions to parse higher level syntactical units
 *      like symbols, types, parameters. 
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Created for Pila Version 2.0
 *
 **********************************************************************/
#include "pila.h"
#include "asm.h"

#include "safe-ctype.h"

/**********************************************************************
 * Parsing a quoted string. The output string is padded with four
 * nulls at the end for easier processing of LONG blocks.
 **********************************************************************/
char *ParseQuotedString(char *s, char *d)
{
  char delim = *s++;
    while (*s)
    {
      if (*s==delim)
      {
            if (*(s+1)==delim)
            {
                *d++ = *s;
                s += 2;
            }
            else
            {
                *d++ = '\0';
                *d++ = '\0';
                *d++ = '\0';
                *d++ = '\0';
                return skipSpace(s+1);
            }
        } else {
            *d++ = *s++;
        }
    }

    return skipSpace(s);
}

/*************************************************************************************
 * Parsing an ID which can start with an alphabetic character or '_', '@', '?'
 * and include alphabetic and numeric characters and '_', '$', '?', '@'
 ************************************************************************************/
char *ParseId(char *s,char *d)
{
  int i = 0;
  if (ISALPHA(*s) || *s=='_' || *s=='?' || *s=='@')
  {
    do
    {
      if (i<SIGCHARS)
        d[i++] = *s;
      s++;
    } while (ISALNUM(*s) || *s=='_' || *s=='$' || *s=='?' || *s=='@');
  }
  d[i] = '\0';
  return s;
}

/*************************************************************************************
 * Parsing a symbol which can start with an alphabetic character or '_', '@', '?'
 * and include alphabetic and numeric characters and '.', '_', '$', '?', '@'
 ************************************************************************************/
char *ParseSymbol(char *s,SymbolDef **symbol, Value *value)
{
  char id[SIGCHARS+1];
  SymbolCategory cat;
  SymbolDef *lastSymbol;
  
  *symbol      = NULL;
  value->kind  = symbolKindUndefined;
  value->value = 0;
  value->type  = NULL;
  
  s = ParseId(skipSpace(s),id);
  if (*id)
  {
    *symbol = SymbolLookupScopeProc(id);
    if (!*symbol)
      *symbol = SymbolLookup(id);
    if (*symbol)
    {
      cat = SymbolGetCategory(SymbolGetKind(*symbol));
      if (cat!=symbolCategoryNone && cat!=symbolCategoryType)
      {
        value->value = SymbolGetValue(*symbol);
        value->kind  = SymbolGetKind(*symbol);
        value->type  = SymbolGetType(*symbol);
      }
    }
  }
  
  while (*s=='.')
  {
    s = ParseId(s+1,id);
    if (*id && *symbol)
    {
      lastSymbol = *symbol; // save it for later (if we have a bitmap member)
      *symbol = SymbolLookupScopeSymbol(lastSymbol,id); // try to find this symbol
      
      // if we haven't gotten the symbol kind yet try to get it from this symbol
      if (*symbol)
      {
        cat = SymbolGetCategory(SymbolGetKind(*symbol));
        if (value->kind==symbolKindUndefined || value->kind==symbolKindProcEntry)
        {
          // symbolKindProcEntry will be overwritten if we are actually
          // accessing a symbol local to the procedure
          if (cat!=symbolCategoryNone && cat!=symbolCategoryType)
          {
            value->value = SymbolGetValue(*symbol);
            value->kind  = SymbolGetKind(*symbol);
            value->type  = SymbolGetType(*symbol);
          }
        }
        else if (cat!=symbolCategoryNone && cat!=symbolCategoryType)
        {
          if (SymbolGetKind(SymbolGetType(lastSymbol))==symbolKindTypeBitmapMember)
          {
            value->value = SymbolGetValue(*symbol);
            value->kind  = symbolKindConst;
            value->type  = NULL;
          }
          else
          {
            value->value += SymbolGetValue(*symbol);
            value->type  = SymbolGetType(*symbol);
          }
        }
      }
    }
    else
      *symbol = NULL; // no symbol id found behind period!
  }

  return s;
}

/*************************************************************************************
* Parsing an argument. It will consist of all characters until the end of the string,
* until a semicolon outside of quotes or until any termination character outside of
* quotes and outside of any pair of brackets or parenthesis, whatever occures first.
*************************************************************************************/
char *ParseArg(char *s, char *d, char *szTerm)
{
  int cParen   = 0;

  int cBracket = 0;

  int fQuoted  = 0;

  int fDblQuot = 0;

  s = skipSpace(s);
  
  /* Loop until the end of the string or until a semicolon outside of quotes or until     */
  /* any termination character outside of quotes and outside any parenthesis or brackets. */
  while (*s && ((*s!=';' && (!strchr(szTerm,*s) || cParen || cBracket)) || fQuoted || fDblQuot))
  {
    switch (*d++=*s++)
    {
      case '(':
        if (!fQuoted && !fDblQuot)
          cParen++;
        break;
        
      case ')':
        if (!fQuoted && !fDblQuot)
        {
          cParen--;
          if (cParen<0)
          {
            Error(UNMATCHED_RIGHT_PAREN,s-1);
            cParen = 0;
          }
        }
        break;
        
      case '[':
        if (!fQuoted && !fDblQuot)
          cBracket++;
        break;
        
      case ']':
        if (!fQuoted && !fDblQuot)
        {
          cBracket--;
          if (cBracket<0)
          {
            Error(UNMATCHED_RIGHT_BRACKET,s-1);
            cBracket = 0;
          }
        }
        break;
        
      case '\'':
        if (!fDblQuot)
          fQuoted = !fQuoted;
        break;
        
      case '"':
        if (!fQuoted)
          fDblQuot = !fDblQuot;
        break;
        
      default:
        break;
    }
    s = skipSpace(s);
  }
  *d = '\0';    
  
  if (cParen || cBracket || fQuoted || fDblQuot)
    Error(INCOMPLETE_PARAMETER_SPEC,NULL);
  
  return s;
}


/***********************************************************************************
* Parse a type specification. Not only the type name will be parsed but also pointer
* markers and brackets for array specifications.
***********************************************************************************/
char *ParseTypeSpec(char *s,SymbolDef **varType,boolean dotFirst)
{
  Value dimSize;
  SymbolDef *baseType;
  char typeId[255+SIGCHARS];
  
  *varType = NULL;

  if (dotFirst)
  {
    if (*s!='.')
    {
      Error(EXPECTED_PERIOD_BEFORE_TYPE,s);
      return NULL;
    }
    else
      s++;
  }

  s = ParseId(s,typeId);
  if (!(*typeId))
  {
    if (*s>='0' && *s<='9')
    {
      long  typeSize = 0;
      char *dest = typeId;
      while (*s>='0' && *s<='9')
      {
        typeSize = typeSize*10+*s-'0';
        *(dest++) = *(s++);
      }
      *dest = '\0';
      baseType = SymbolCreate(typeId,symbolKindTypeSimple,NULL,typeSize);
    }
    else
    {
      Error(EXPECTED_TYPE_NAME,s);
      return NULL;
    }
  }
  else
    baseType = SymbolLookup(typeId);
    
  if (!baseType || SymbolGetCategory(SymbolGetKind(baseType))!=symbolCategoryType)
  {
    Error(UNDEFINED_TYPE,typeId);
    return NULL;
  }
  
  while (*s=='*')
  {
    baseType = SymbolCreateDerivedType(baseType,symbolKindTypePointer,0);
    s++;
  }
    
  while (*s=='[')
  {
    do
    {
      s = skipSpace(s+1);
      if (*s==']' || *s==',')
        baseType = SymbolCreateDerivedType(baseType,symbolKindTypeArray,0);
      else
      {
        s = evaluate(s,&dimSize);
        if (!s || ErrorStatusIsSevere())
          return NULL;
        else if (SymbolGetCategory(dimSize.kind)!=symbolCategoryConst)
          Error(INV_VALUE_CATEGORY,NULL);
        baseType = SymbolCreateDerivedType(baseType,symbolKindTypeArray,dimSize.value);
      }
    } while (*s==',');
    if (*s==']')
      s++;
  }

  *varType = baseType;
  return skipSpace(s);
}


/***********************************************************************************
* Parse a parameter list specification.
***********************************************************************************/
char *ParseParameters(char *s,SymbolDef *parmList,SymbolKind kind)
{
  SymbolDef *type;
  char       symbolId[SIGCHARS+1];
  int        parmCount = 0;
  char      *aux;

  if (*s!='(')
  {
    Error(EXPECTED_LEFT_PAREN,s);
    return NULL;
  }
  
  aux = skipSpace(s+1);
  if (*aux==')')
    s = aux;
    
  while (*s!=')')
  {
    parmCount++;
    type = NULL;
    s = ParseId(skipSpace(s+1),symbolId);
    if (!*symbolId)
    {
      if (strncmp("...",s,3)==0)
      {
        s = skipSpace(s+3);
        if (*s!=')')
        {
          Error(EXPECTED_RIGHT_PAREN,s);
          return NULL;
        }
        type = SymbolLookup("void");
        strcpy(symbolId,"...");
      }
      else
        sprintf(symbolId,"_parm%d",parmCount);
    }
    
    if (!type)
    {
      s = ParseTypeSpec(s,&type,true); // looks for '.' first
      if (type==NULL)
        return NULL;
    }
    
    SymbolCreateParameter(parmList,kind,parmCount,symbolId,type);

    if (*s!=',' && *s!=')')
    {
      Error(EXPECTED_PAREN_OR_COMMA,s);
      return NULL;
    }
  }

  // skip ')' and check for period in case a return type is given 
  if (*(++s)=='.')
  {
    s = ParseTypeSpec(s,&type,true); // skips the '.' first
    if (type==NULL)
      return NULL;
    SymbolSetType(parmList,type);
  }
  else // if no period then skip all spaces
    s = skipSpace(s);

  return s;
}
