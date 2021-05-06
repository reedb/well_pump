/***********************************************************************
 *
 *      MAIN.C
 *      Main Module for 68000 Assembler
 *
 *    Function: main()
 *      Parses the command line, opens the input file and
 *      output files, and calls processFile() to perform the
 *      assembly, then closes all files.
 *
 *   Usage: main(argc, argv);
 *      int argc;
 *      char *argv[];
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

/* F. Schaeckermann --- moved ifndef unix block up before "#include asm.h" due to      */
/* 2003-07-23       --- conflict with #define WORD in asm.h and it's usage in windef.h */
// for htonl and ntohl
#ifndef unix
    #include <winsock.h>
#else
    #include <asm/byteorder.h>
#endif

#include "pila.h"
#include "asm.h"
#include "prc.h"
#include "safe-ctype.h"
#include "options.h"

char   gszAppName[dmDBNameLength] = "";		/* application name from APPL directive */
FourCC gfcPrcType = MAKE4CC('a','p','p','l');	/* database type, default is 'appl' */
char  *pilafilename = NULL;			/* input file name */

#define _NO_EXTERN_GLOBAL_OPTIONS
options globalOptions;				/* command line options */

/* General */
const char *progname; /* the program's name under which it was called */

long gulOutLoc;     /* The assembler's location counter */
int giPass;         /* Counter telling what pass we are on (0, 1 or 2) */
boolean endFlag;       /* Flag set when the END directive is encountered */

//
//
//

BlockType gbt;
unsigned char *gpbCode;
long gulCodeLoc;

unsigned char *gpbData;
long gulDataLoc;

unsigned char *gpbResource;
long gulResLoc;
unsigned long gfcResType;
long gidRes;
long gcbResTotal;

unsigned char *gpbOutput;



int main(int argc, char *argv[])
{
    extern long gcbDataCompressed;
    char pszFile[_MAX_PATH], outName[_MAX_PATH], *p;
    int i;
    long cbRes, cbPrc;
    char szErrors[80];

	pilafilename = argv[0];

    puts("Pila 2.0 Beta ("__DATE__" "__TIME__")\n");

    i = SetArgFlags(argc, argv);
    if (!i) {
        help();
    }

    gszAppName[0] = 0;

    /* Check whether a name was specified */

    if (i >= argc) {
        fputs("No input file specified\n\n", stdout);
        help();
    }

    if (!strcmp("?", argv[i])) {
        help();
    }

    strcpy(pszFile, argv[i]);

    /* Process output file names in their own buffer */
    strcpy(outName, pszFile);

    /* Change extension to .lis */
    p = strchr(outName, '.');
    if (!p) {
        p = outName + strlen(outName);
    }
    if (OPTION(listing))
    {
        strcpy(p, ".lis");
        ListInitialize(outName);
    }

    strcpy(p, ".prc");

    /* Assemble the file */
    SymbolInitialize();
    processFile(pszFile);

    /* Close files and print error and warning counts */
    //PopSourceFile();

    // Get the resource total before WritePrc adds in the code and data
    // resources.
    cbRes = gcbResTotal;

    // If no errors, write the PRC file.
    cbPrc = 0;
    if (ErrorGetErrorCount()==0) {
        cbPrc = WritePrc(outName, gszAppName, gpbCode, gulCodeLoc, gpbData, gulDataLoc);
        fprintf(stdout, "Code: %ld bytes\nData: %ld bytes (%ld compressed)\n"
                "Res:  %ld bytes\nPRC:  %ld bytes\n",
                gulCodeLoc, gulDataLoc, gcbDataCompressed, cbRes, cbPrc);
    }

    sprintf(szErrors, "%d error%s, %d warning%s\n",
            ErrorGetErrorCount(),   (ErrorGetErrorCount()!= 1)  ? "s" : "",
            ErrorGetWarningCount(), (ErrorGetWarningCount()!=1) ? "s" : "");

    fprintf(stdout, szErrors);

    ListClose(szErrors);

    return ErrorGetErrorCount();
}


char *skipSpace(char *p)
{
  while (ISSPACE(*p)) p++;
  return p;
}


