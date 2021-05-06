/***********************************************************************
 *
 *      LISTING.C
 *      Listing File Routines for 68000 Assembler
 *
 *    ListInitialize(char *listFileName)
 *      Opens the specified listing file for writing. If the
 *      file cannot be opened, then the routine prints a
 *      message and exits.
 *
 *    ListClose(char *szErrors)
 *      Closing of the listing file after writing the error statistics
 *      passed in with szErrors.
 *
 *    ListStartListing()
 *      There will be no listing output (even after calls to
 *      ListInitialize and ListEnable) until this function has been
 *      called. That way any listing output can be deferred until
 *      the last pass is started.
 *
 *    ListEnable()
 *      Enables writing to the listing file.
 *
 *    ListDisable()
 *      Disables writing to the listing file.
 *
 *    ListWriteLine()
 *      Writes the current listing line to the listing file.
 *      If a current source line is available it is appended to
 *      the listing data that was set through the ListPut... routines.
 *      If the current source line is not an expansion-line the current
 *      source file line number is written as well.
 *      If an error occurs during the writing, the routine prints a 
 *      message and stops the program.
 *
 *    ListWriteError(char *errorMsg)
 *      Adds an error message to the listing file. The actual writing of
 *      the error message is deferred until after the offending source
 *      line has been written to the listing file.
 *      The string passed in as parameter must have been allocated via
 *      malloc. If there is no listing file written the function returns
 *      true to signal to the caller that the message has not been stored
 *      (and can be freed) otherwise false is returned and the message will
 *      be freed by ListWriteLine later on.
 *
 *    ListPutSourceLine(char *sourceLine, int sourceLineNo)
 *      This call stores the source line and the current line no for
 *      later inclusion in the line written to the listing file.
 *
 *    ListPutLocation(unsigned long outputLocation)
 *      Starts the process of assembling a listing line by
 *      printing the location counter value into listData and
 *      initializing listPtr.
 *
 *    ListPutSymbol(long data)
 *      This call prints the value of a symbol into the object field
 *      of the listing line. It is called by the directives EQU and SET.
 *
 *    ListPutData(long data,int size)
 *      Prints the data whose size and value are specified in
 *      the object field of the current listing line. Bytes are
 *      printed as two digits, words as four digits, and
 *      longwords as eight digits, and a space follows each
 *      group of digits.
 *           If the item to be printed will not fit in the
 *      object code field, one of two things will occur. If option
 *      const_expanded is TRUE, then the current listing line will
 *      be output to the listing file and the data will be printed
 *      on a continuation line. Otherwise, elipses ("...") will
 *      be printed to indicate the omission of values from the
 *      listing, and the data will not be added to the file.
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 12/13/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Major Changes for Pila Version 2.0
 *                 Changed to be a true Module without external
 *                 globals (neither defining nor accessing).
 *                 Removed all globals and access to globals.
 *                 Renamed all functions.
 *
 ************************************************************************/

#include "pila.h"
#include "asm.h"

#include <stdio.h>
#include "expand.h"
#include "options.h"
#include "listing.h"
#include "libiberty.h"

/************************************************************************
 * Declarations of module variables
 ************************************************************************/
static FILE *listFile = NULL;		/* listing file */

static char  listData[49];			/* Buffer in which listing lines are assembled */
static char *listPtr;				/* Pointer to above buffer */

static char *currentSourceLine = NULL;	/* buffer for current source line */
static int   currentSourceLineLen = 0;	/* size of source line buffer */
static int   currentSourceLineNo;		/* currently worked on source line no. */

static boolean enabled = false;		/* to keep track of list enable/disable */
static boolean started = false;		/* only in pass 2 will there be anything written */

/************************************************************************
 * Structure and variables for error message deferral
 ************************************************************************/
typedef struct _DeferredErrorMsg
{
  struct _DeferredErrorMsg *next;
  char                     *msg;
} DeferredErrorMsg;

DeferredErrorMsg *deferredErrorMsgList = NULL;
DeferredErrorMsg *lastErrorEntry       = (DeferredErrorMsg *)&deferredErrorMsgList;


void ListInitialize(char *name)
{
    listFile = fopen(name, "w");
    if (!listFile)
	{
        puts("Can't open listing file");
        exit(0);
    }
}

void ListClose(char *szErrors)
{
	if (listFile)
	{
		putc('\n', listFile);
		fputs(szErrors,listFile);
		fclose(listFile);
		listFile = NULL;
	}

	if (currentSourceLine)
	{
		free(currentSourceLine);
		currentSourceLine = NULL;
	}
}

void ListStartListing()
{
	started = true;
	ListEnable();
}


void ListEnable()
{
	enabled = (listFile!=NULL) && started;
}


void ListDisable()
{
	enabled = false;
}


boolean ListWriteError(char *errorMsg)
{
	if (listFile!=NULL) // keep message if a listing file is being written
	{					// (even if listing is off at this point)
		DeferredErrorMsg *errListEntry = xmalloc(sizeof(DeferredErrorMsg));
		errListEntry->next = NULL;
		errListEntry->msg  = errorMsg;
		lastErrorEntry->next = errListEntry;
		lastErrorEntry = errListEntry;
		return false;
	}
	else
		return true;
}


void ListWriteLine()
{
	if (enabled)
	{
	    fprintf(listFile, "%-41.41s", listData);
	    if (currentSourceLine && *currentSourceLine)
	    {
	      if (ExpandGetLineNum()==0)
	        fprintf(listFile, "%5d  %s", currentSourceLineNo, currentSourceLine);
	      else
	        fprintf(listFile, "       %s",currentSourceLine);
		  *currentSourceLine = '\0';
	    }
	    else
	    {
	        putc('\n', listFile);
	    }
	}

	if (listFile!=NULL) // write error messages even if listing is off
	{
		DeferredErrorMsg *deferredErrorMsg;

		while (deferredErrorMsgList!=NULL)
		{
			fputs(deferredErrorMsgList->msg,listFile);
			deferredErrorMsg = deferredErrorMsgList;
			deferredErrorMsgList = deferredErrorMsgList->next;
			free(deferredErrorMsg->msg);
			free(deferredErrorMsg);
		}
		
	    if (ferror(listFile)) {
	        fputs("Error writing to listing file\n", stdout);
	        exit(0);
	    }
	}
}


void ListPutSourceLine(char *sourceLine,int sourceLineNo) // gpsseCur->iLineNum
{
	if (enabled)
	{
		int len = strlen(sourceLine)+1;

		if (currentSourceLine==NULL)
		{
			currentSourceLine = xstrdup(sourceLine);
			currentSourceLineLen = len;
		}
		else
		{
			if (currentSourceLineLen<len)
			{
				currentSourceLine = realloc(currentSourceLine,len);
				currentSourceLineLen = len;
			}
			strcpy(currentSourceLine,sourceLine);
		}
		currentSourceLineNo = sourceLineNo;
	}
}

void ListPutLocation(unsigned long outputLocation)
{
	if (enabled)
	{
    	sprintf(listData, "%08lX  ", outputLocation);
    	listPtr = listData + 10;
	}
}


void ListPutSymbol(long data)
{
	if (enabled)
	{
		*(listPtr++) = '=';
		ListPutData(data,LONG);
	}
}


void ListPutTypeName(char *data)
{
	if (enabled)
	{
		*(listPtr++) = '=';
		*(listPtr+29) = '\0';
		strncpy(listPtr,data,29);
		listPtr += strlen(listPtr);
	}
}


void ListPutData(long data, int size)
{
	if (enabled)
	{
		if (listPtr-listData+size*2+1>40)
		{
			if (!OPTION(const_expanded))
			{
				strcpy(listData+((size==WORD) ? 35 : 37),"...");
				return;
			}
			else
			{
				ListWriteLine();
				strcpy(listData,"          ");
				listPtr = listData+10;
			}
		}
		
	    switch (size)
		{
			case BYTE:
				sprintf(listPtr, "%02lX ", data & 0xFF);
				listPtr += 3;
				break;
			case WORD:
				sprintf(listPtr, "%04lX ", data & 0xFFFF);
				listPtr += 5;
				break;
			case LONG:
				sprintf(listPtr, "%08lX ", data);
				listPtr += 9;
				break;
			default:
				printf("ListPutData: INVALID SIZE CODE!\n");
				exit(0);
		}
	}
}
