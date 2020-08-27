/**********************************************************************
 *
 *      PARSE.H
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

#ifndef _PARSE_H_
#define _PARSE_H_ 

char *ParseQuotedString(char *s, char *d);
char *ParseId(char *s,char *d);
char *ParseSymbol(char *s,SymbolDef **symbol,Value *value);
char *ParseArg(char *s, char *d, char *szTerm);
char *ParseTypeSpec(char *s,SymbolDef **varType,boolean lookForDot);
char *ParseParameters(char *s,SymbolDef *parmList,SymbolKind kind);

#endif
