/***********************************************************************
 *
 *      ERROR.C
 *      Error message printer for 68000 Assembler
 *
 *    Function: printError()
 *      Prints an appropriate message to the specified output
 *      file according to the error code supplied. If the
 *      errorCode is OK, no message is printed; otherwise an
 *      WARNING or ERROR message is produced. The line number
 *      will be included in the message unless lineNum = -1.
 *
 *   Usage: printError(outFile, errorCode, lineNum)
 *      FILE *outFile;
 *      int errorCode, lineNum;
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 12/12/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *                 Created true module that neither defines nor
 *                 accesses global (external) variables.
 *                 Lots of functions added to query current state.
 *
 ************************************************************************/

#include "pila.h"
#include "asm.h"
#include "libiberty.h"

int           maxErrorCode   = OK;
int           errorCount     = 0;
int           warningCount   = 0;
boolean		  writeMessage   = false;

void ErrorStartReporting()
{
	writeMessage = true;
	maxErrorCode = OK;
	errorCount   = 0;
	warningCount = 0;
}


char *ErrorMessage(ErrorCode errCde)
{
  switch(errCde)
  {
#define ERRCODE(x,y) case x: return y;
	ERROR_CODE_LIST
#undef ERRCODE
  }
  return "no message";
}


char *ErrorLine(ErrorCode errorCode,char *details)
{
  char msg[1024];
  char *pszError = NULL;
  char *pszType;
  pszError = ErrorMessage(errorCode);
  pszType  = errorCode>=MINOR ? "error" : "warning";
  if (details)
    sprintf(msg,"%s(%d): %s: %s: %s\n",gpsseCur->szFile,gpsseCur->iLineNum,pszType,pszError,details);
  else
    sprintf(msg,"%s(%d): %s: %s\n",gpsseCur->szFile,gpsseCur->iLineNum,pszType,pszError);
  return xstrdup(msg);
}


void Error(ErrorCode code,char *details)
{
  char *msg;
 
  if (code>maxErrorCode)
    maxErrorCode = code;
  
  if (writeMessage)
  {
    if (code>=MINOR)
      errorCount++;
    else if (code>=WARNING)
      warningCount++;

    msg = ErrorLine(code,details);
    if (!msg)
    {
      printf("memory error: message %d for line no. %d could not be created",code,gpsseCur->iLineNum);
      exit(16);
    }
    else
    {
      fputs(msg,stdout);
	  if (ListWriteError(msg)) // returns true if message is not kept
	      free(msg);
    }
  }
}

int ErrorGetErrorCount()
{
  return errorCount;
}

int ErrorGetWarningCount()
{
  return warningCount;
}

int ErrorStatusIsSevere()
{
  return (maxErrorCode>=SEVERE);
}

int ErrorStatusIsError()
{
  return (maxErrorCode>=ERROR_RANGE);
}

int ErrorStatusIsMinor()
{
  return (maxErrorCode>=MINOR);
}

int ErrorStatusIsWarning()
{
  return (maxErrorCode>=WARNING);
}

int ErrorStatusIsOK()
{
  return (maxErrorCode==OK);
}

void ErrorStatusReset()
{
  maxErrorCode  = OK;
}

