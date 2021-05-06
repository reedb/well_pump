/**********************************************************************************
 *
 *      EXPAND.C
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

#define _GNU_SOURCE 1
#include <stdio.h>

#include "pila.h"
#include "asm.h"
#include "libiberty.h"
#include "expand.h"

// Expanded Line
typedef struct _ExpandLine
{
    struct _ExpandLine *next;
    char *line;
	int  capacity;
} ExpandLine;

typedef struct _Expand
{
  struct _Expand *next;
  ExpandLine     *firstLine;
  ExpandLine     *lastLine;
} Expand;

char  *sourceLine = NULL;		/* source line buffer */
int    sourceLineCapacity = 0;	/* source line buffer length */

Expand *pExpandStack = NULL;
int    ExpandLineNum = 0;


void ConcatString(char **target,int *targetCapacity,char *source)
{
	char *aux = *target;
	int  len = strlen(source);

	if (!aux)
	{
		*targetCapacity = len<81 ? 81 : len+1;
		*target = xmalloc(*targetCapacity);
		strcpy(*target,source);
	}
	else
	{
		len += strlen(aux);
		if (len<*targetCapacity)
			strcat(aux,source);
		else
		{
			*targetCapacity += len-*targetCapacity+1;
			*target = xrealloc(aux,*targetCapacity);
			strcat(*target,source);
		}
	}
}

int ExpandGetLineNum()
{
  return ExpandLineNum;
}

char *ExpandGetLine() // returns pointer to next sourceline
{
  if (pExpandStack!=NULL)
  {
    ExpandLine *expLine;

    // Disable any further additions to THIS Expand. Any newly
    // created Expand lines will now create a new Expand.
    pExpandStack->lastLine = NULL;
    
    expLine = pExpandStack->firstLine;
    pExpandStack->firstLine = pExpandStack->firstLine->next;
	if (sourceLine)
		*sourceLine = '\0';
	ConcatString(&sourceLine,&sourceLineCapacity,expLine->line);
    free(expLine->line);
	free(expLine);
    
    if (pExpandStack->firstLine==NULL)
    {
      Expand *exp = pExpandStack;
      pExpandStack = pExpandStack->next;
      free(exp);
    }
    ExpandLineNum++;
  }
  else
  {
	  ExpandLineNum = 0;
	  if (getline(&sourceLine,&sourceLineCapacity,gpsseCur->pfil)<0)
		  return NULL;
	  else
		  gpsseCur->iLineNum++;
  }
  return sourceLine;
}

void ExpandString(char *string)
{
	Expand    *exp = pExpandStack;
	ExpandLine *expLine;
	
	if (exp==NULL || exp->lastLine==NULL)
	{
		exp = xmalloc((size_t)sizeof(Expand));
		exp->next = pExpandStack;
		pExpandStack = exp;

		exp->firstLine = xmalloc((size_t)sizeof(ExpandLine));
		exp->firstLine->next = NULL;
		exp->firstLine->line = NULL;
		exp->firstLine->capacity = 0;
		exp->lastLine  = exp->firstLine;
	}
	
	// now we have a pointer to an Expand (in exp)
	// with a lastLine pointer that is not NULL and it
	// is pointing to the line that we have filled so far.
	expLine = exp->lastLine;
	
	// does that line exist and has already been completed?
	if (expLine->line && strchr(expLine->line,'\n')!=NULL)
	{
		// then we have to allocate a new line that will
		// absorb the string that was passed in.
		expLine = xmalloc((size_t)sizeof(ExpandLine));
		expLine->next = NULL;
		expLine->line = NULL;
		expLine->capacity = 0;
		exp->lastLine->next = expLine;
		exp->lastLine = expLine;
	}

	// now copy the string into the ExpandLine we have in expLine
	ConcatString(&(expLine->line),&(expLine->capacity),string);
}


void ExpandInstruction(char *szInst, char *szOp1, char *szOp2)
{
  ExpandString("\t");
  ExpandString(szInst);
  ExpandString("\t");
  if (szOp1!=NULL)
      ExpandString(szOp1);
  if (szOp2!=NULL)
  {
    ExpandString(",");
    ExpandString(szOp2);
  }
  ExpandString("\n");
}


void ExpandInteger(int value)
{
  char szT[16];
  sprintf(szT,"%d",value);
  ExpandString(szT);
}

