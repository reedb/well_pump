/**********************************************************************************
 *
 *      EXPAND.H
 *
 *      Routines to programmatically 'inject' additional source lines into the
 *      the source read from the input files.
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Created for Pila Version 2.0
 *
 *********************************************************************************/


#ifndef _EXPAND_H_
#define _EXPAND_H_

int   ExpandGetLineNum();
char *ExpandGetLine();
void  ExpandString(char *string);
void  ExpandInstruction(char *szInst, char *szOp1, char *szOp2);
void  ExpandInteger(int value);

#endif
