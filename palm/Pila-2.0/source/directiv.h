/***************************************************************************
 *
 *      Header file with declarations of all the functions found
 *      in directiv.c
 *
 *      Originally those declarations could be found in pila.h.
 *
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Created for Pila Version 2.0
 *
 **************************************************************************/


#ifndef __DIRECTIV_H__
#define __DIRECTIV_H__

#ifdef ORG_DIRECTIVE
    int org(int, char *, char *);
#endif
int funct_end(int, char *, char *);
int equ(int, char *, char *);
int set(int, char *, char *);
int dc(int, char *, char *);
int dcb(int, char *, char *);
int ds(int, char *, char *);

boolean DirectiveContinuation(char *line);
int CodeDirective(int size, char *label, char *op);
int DataDirective(int size, char *label, char *op);
int ResDirective(int size, char *label, char *op);
int IncludeDirective(int size, char *label, char *op);
int ApplDirective(int size, char *label, char *op);
FILE *PushSourceFile(char *pszNewSource);
boolean PopSourceFile();
int AlignDirective(int size, char *label, char *op);
int ListDirective(int size, char *label, char *op);
int IncbinDirective(int size, char *label, char *op);
int BeginProcDirective(int size, char *label,char *op);	// start procedure code
int CallDirective(int size, char *label, char *op);	// call procedure or trap
int EndProcDirective(int size, char *label, char *op);	// end of procedure
int EndProxyDirective(int size, char *label, char *op);	// end of proxy
int EndMemberedTypeDirective(int size, char *label, char *op); // end of type
int EnumDirective(int size, char *label, char *op);	// start of enum type
int ExternDirective(int size, char *label, char *op);	// external variable
int GlobalDirective(int size, char *label, char *op);	// global variable
int LocalDirective(int size, char *label, char *op);	// local variable
int ProcDirective(int size, char *label, char *op);	// procedure definition
int ProcDefDirective(int size, char *label, char *op);	// procedure declaration
int ProxyDirective(int size, char *label, char *op);	// proxy definition
int StructDirective(int size, char *label, char *op);	// start of struct type
int TrapDefDirective(int size, char *label, char *op);	// trap definition
int TypeDefDirective(int size, char *label, char *op);	// type definition
int UnionDirective(int size, char *label, char *op);	// start of union type
int EndEnumDirective(int size, char *label, char *op);	// end of enum type
int EndStructDirective(int size, char *label, char *op);// end of struct type
int EndUnionDirective(int size, char *label, char *op);	// end of struct type
int IfDirective(int size, char *label, char *op);	// if directive
int IfDefDirective(int size, char *label, char *op);	// ifdef directive
int IfNDefDirective(int size, char *label, char *op);	// ifndef directive
int ElseDirective(int size, char *label, char *op);	// else directive
int EndIfDirective(int size, char *label, char *op);	// endif directive
int ErrorDirective(int size, char *label, char *op);	// programmer error message

#endif
