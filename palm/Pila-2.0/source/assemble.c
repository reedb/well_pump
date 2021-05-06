/***********************************************************************
 *
 *      ASSEMBLE.C
 *      Assembly Routines for 68000 Assembler
 *
 *    Function: processFile()
 *      Assembles the input file. For each pass, the function
 *      passes each line of the input file to assemble() to be
 *      assembled. The routine also makes sure that errors are
 *      printed on the screen and listed in the listing file
 *      and keeps track of the error counts and the line
 *      number.
 *
 *      assemble()
 *      Assembles one line of assembly code. The line argument
 *      points to the line to be assembled. The errors are
 *      reported through the function Error(<code>).
 *      The routine first determines if the line contains
 *      a label and saves the label for later use.
 *      It then calls instLookup() to look up the
 *      instruction (or directive) in the instruction table. If
 *      this search is successful and the parseFlag for that
 *      instruction is true, it defines the label and parses
 *      the source and destination operands of the instruction
 *      (if appropriate) and searches the flavor list for the
 *      instruction, calling the proper routine if a match is
 *      found. If parseFlag is FALSE, it passes pointers to the
 *      label and operands to the specified routine for
 *      processing.
 *
 *   Usage: processFile()
 *
 *      assemble(line)
 *      char *line;
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 12/13/86
 *
 *      Change Log:
 *
 *      07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *                 Changes for Pila Version 2.0
 *
 ************************************************************************/

#include "pila.h"
#include "asm.h"

#include "directiv.h"
#include "expand.h"
#include "symbol.h"
#include "parse.h"
#include "strcap.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "insttabl.h"

extern long gulOutLoc;      /* The assembler's location counter */
extern int giPass;          /* Flag set during second pass */
extern boolean endFlag;     /* Flag set when the END directive is encountered */
extern char gszAppName[];   /* application name set by APPL directive */

int processFile(char *szFile)
{
    FILE *pfilInput;
	char *line;

    // Allocate temporary buffers used for code, data, and resources.
    gpbCode = (unsigned char *)xmalloc(kcbCodeMax);
    gpbData = (unsigned char *)xmalloc(kcbDataMax);
    gpbResource = (unsigned char *)xmalloc(kcbResMax);

    if (gpbCode == NULL || gpbData == NULL || gpbResource == NULL)
	{
        printf("Failed to allocate necessary assembly buffers!\n");
        exit(0);
    }

    for (giPass = 0; giPass<=2; giPass++)
	{
		gulOutLoc = gulCodeLoc = gulDataLoc = gulResLoc = 0;

		gbt = kbtCode;      // block is code unless otherwise specified
		gpbOutput = gpbCode;

		pfilInput = PushSourceFile(szFile);
		if (pfilInput == NULL || pfilInput == (FILE *)-1)
		{
			fputs("Input file not found\n", stdout);
			exit(0);
		}
		
		endFlag = false;
        
		if (giPass==2)
		{
			ListStartListing();
			ErrorStartReporting();
		}
	
        do
		{
			while (!endFlag && (line=ExpandGetLine())!=NULL)
			{
                ErrorStatusReset();
                ListPutLocation(gulOutLoc);
				ListPutSourceLine(line,gpsseCur->iLineNum);
                assemble(line);
				ListWriteLine();
			}
		} while (PopSourceFile());

        if (gszAppName[0]=='\0')
			Error(MISSING_APPL,NULL);
    }

    return NORMAL;
}


int assemble(char *line)
{
    instruction *tablePtr;
    flavor *flavorPtr;
    opDescriptor source, dest;
    char *p, *start, label[SIGCHARS+1], size, f;
    boolean sourceParsed, destParsed;
    unsigned short mask;

    p = start = skipSpace(line);

    // Comments start with '*' or ';'
    if (*p && *p!='*' &&  *p!=';')
    {
      if (!DirectiveContinuation(p))
      {
        label[0] = '\0';
        
        if (*p=='.') // could be a temporary label!
        {
          if ((*(p+1)>='1' && *(p+1)<='9') &&
              (ISSPACE(*(p+2)) || *(p+2)==':') &&
              (gbt==kbtCode))
          {
            // it IS a temporary label
            SymbolCreateTempLabel(*(p+1));
            if ((*p+2)==':')
              p++;
            p = start = skipSpace(p+2);
            // check if we are at the end of this line already
            if (!*p || *p=='*' || *p==';')
                return NORMAL; // yep... no further processing needed for this line
          }
        }
        else // if there is no temporary label then look for a normal one
        {
          // Get a word (alphanum string including '_', '$', '?', '@')
          // May be a label or instruction
          p = ParseId(p,label);
        }
        
        // Is this a label? (must be at the start of a line and end in a colon)
        if (*label && ((ISSPACE(*p) && start==line) || *p==':'))
        {
          if (*p==':')
            p++;
          p = skipSpace(p);

          // Look for a comment again.
          if (*p=='*' || *p==';' || !*p)
          {
              SymbolCreate(label, symbolKindLabel, NULL, gulOutLoc);
              return NORMAL;
          }
        }
        else
        {
          // Not a label, reset to start of line to begin getting the
          // instruction.
          p = start;
          label[0] = '\0';
        }

        // Parse an instruction
        p = instLookup(p, &tablePtr, &size);
        if (ErrorStatusIsSevere())
            return NORMAL;

        // Parse the instruction's operands
        p = skipSpace(p);
        if (tablePtr->parseFlag)
        {
            /* Move location counter to a word boundary and fix
               the listing before assembling an instruction */
            if (gulOutLoc & 1)
			{
				Error(ALIGNMENT_WARNING,NULL);
                ListPutLocation(++gulOutLoc);
			}

            if (*label)
            {
              SymbolCreate(label, symbolKindLabel, NULL, gulOutLoc);
              if (ErrorStatusIsSevere())
                  return NORMAL;
            }
            
            sourceParsed = destParsed = false;
            flavorPtr = tablePtr->flavorPtr;
            for (f = 0; f < tablePtr->flavorCount; f++, flavorPtr++) {
                if (!sourceParsed && flavorPtr->source) {
                    p = opParse(p, &source, 1, (flavorPtr->exec==branch));
                    if (ErrorStatusIsSevere()) {
                        return NORMAL;
                    }
                    p = skipSpace(p);
                    sourceParsed = true;
                }

                if (!destParsed && flavorPtr->dest) {
                    if (*p != ',') {
                        Error(SYNTAX,p);
                        return NORMAL;
                    }

                    p = opParse(skipSpace(p+1), &dest, 2, (flavorPtr->exec==dbcc));
                    if (ErrorStatusIsSevere()) {
                        return NORMAL;
                    }
                    p = skipSpace(p);
                    if (*p && *p!=';') {
                        Error(SYNTAX,p);
                        return NORMAL;
                    }
                    destParsed = true;
                }

                if (!flavorPtr->source) {
                    mask = pickMask( (int) size, flavorPtr);
                    (*flavorPtr->exec)(mask, (int) size, &source, &dest);
                    return NORMAL;
                } else if ((source.mode & flavorPtr->source) && !flavorPtr->dest) {
                    if (*p && *p!=';') {
                        Error(SYNTAX,p);
                        return NORMAL;
                    }
                    mask = pickMask( (int) size, flavorPtr);
                    (*flavorPtr->exec)(mask, (int) size, &source, &dest);
                    return NORMAL;
                } else if (source.mode & flavorPtr->source 
                    && dest.mode & flavorPtr->dest) {
                    mask = pickMask( (int) size, flavorPtr);
                    (*flavorPtr->exec)(mask, (int) size, &source, &dest);
                    return NORMAL;
                }
            }
            Error(INV_ADDR_MODE,NULL);
        }
        else
        {
            (*tablePtr->exec)( (int) size, label, p);
            return NORMAL;
        }
      }
    }

    return NORMAL;
}


int pickMask(int size, flavor *flavorPtr)
{
    if (!size || size & flavorPtr->sizes) {
        if (size & (BYTE | SHORT)) {
            return flavorPtr->bytemask;
        } else if (!size || size == WORD) {
            return flavorPtr->wordmask;
        } else {
            return flavorPtr->longmask;
        }
    }
    Error(INV_SIZE_CODE,NULL);

    return flavorPtr->wordmask;
}

