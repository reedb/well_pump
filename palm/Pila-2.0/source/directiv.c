/***********************************************************************
 *
 *		DIRECTIV.C
 *		Directive Routines for 68000 Assembler
 *
 * Description: The functions in this file carry out the functions of
 *		assembler directives. All the functions share the same
 *		calling sequence:
 *
 *			general_name(size, label, op)
 *			int size;
 *			char *label, *op;
 *
 *		The size argument contains the size code that was
 *		specified with the instruction (using the definitions
 *		in ASM.H) or 0 if no size code was specified. The label
 *		argument is a pointer to a string (which may be empty)
 *		containing the label from the line containing the
 *		directive. The op argument is a pointer to the first
 *		non-blank character after the name of the directive,
 *		i.e., the operand(s) of the directive. Errors are reported
 *		through a call to function Error(<code>).
 *
 *		Author: Paul McKee
 *		ECE492	  North Carolina State University
 *
 *		  Date: 12/13/86
 *
 *		Change Log:
 *
 *		07/23/2003 Frank Schaeckermann (frmipg602@sneakemail.com)
 *				   Major changes for Pila Version 2.0
 *
 ***********************************************************************/

#include "pila.h"
#include "asm.h"
#include "prc.h"
#include "directiv.h"
#include "expand.h"
#include "parse.h"
#include "guard.h"
#include "options.h"

extern long gulOutLoc;
extern int	giPass;
extern boolean endFlag;

extern char *listPtr;	/* Pointer to buffer where listing line is assembled
						   (Used to put =XXXXXXXX in the listing for EQU's and SET's */
#define kcsseMax	50

int gcsse = 0;
struct SourceStackEntry gasse[kcsseMax];
struct SourceStackEntry *gpsseCur;

SymbolDef *insideType		= NULL;		// used for struct/union/enum directives
Value	   nextEnumValue	= {0,symbolKindConst,NULL}; // holds value for next enum member
int		   bitmapShiftValue = 0;		// used for bitmaps inside structs (shift value of last bitmap member)
int		   bitmapTypeSize	= 0;		// number of bytes into which last bitmap member was assigned
SymbolDef *lastLocalSymbol	= NULL;		// last local symbol defined (used to calculate link operand in beginproc)
boolean	   procedureBegun	= false;		// true between beginproc and endproc

#define MAX_IF_LEVEL 32
int		   ifLevel			= 0;		// used for if/else/endif to control code generation
int		   ifNoGenLevel		= 0;		// used for if/else/endif to control code generation
boolean	   ifElseFlag[MAX_IF_LEVEL];		// used to control that only one ELSE is specified for each IF

#ifdef ORG_DIRECTIVE
/***********************************************************************
 *
 *	Function org implements the ORG directive.
 *
 ***********************************************************************/

int org(int size, char *label, char *op)
{
	Value newLoc;

	if (size) {
		Error(INV_SIZE_CODE,NULL);
	}
	if (!*op) {
		Error(SYNTAX,op);
		return NORMAL;
	}
	op = evaluate(op, &newLoc);
	if (op)
	{
	  if (!ErrorStatusIsSevere() && giPass>0 && Guard(newLoc.value,0))
	  {
		Error(GUARD_ERROR,NULL);
	  } else if (!ErrorStatusIsError())
	  {
		if (SymbolGetCategory(newLoc.kind)!=symbolCategoryConst)
		{
		  Error(INV_VALUE_CATEGORY,NULL);
		}
		else if (!*op || *op==';')
		{
			/* Check for an odd value, adjust to one higher */
			if (newLoc.value & 1) {
				Error(ODD_ADDRESS,NULL);
				newLoc.value++;
			}
			gulOutLoc = newLoc.value;
			/* Define the label attached to this directive, if any */
			if (*label) {
				SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);
			}
			/* Show new location counter on listing */
			ListPutLocation(gulOutLoc);
		} else
		{
			Error(SYNTAX,op);
		}
	  }
	}

	return NORMAL;
}
#endif

// A "code", "data", or "res" block ends when the next block begins or
// when a "end" statement is reached.  Each of handlers for those
// directives call this function to make sure the block is properly
// closed.

void EndBlock()
{
	switch (gbt) {
	case kbtData:
		gulDataLoc = gulOutLoc;
		break;

	case kbtCode:
		gulCodeLoc = gulOutLoc;
		break;

	case kbtResource:
		// Write the entire resource out
		AddResource(gfcResType, (unsigned short)gidRes, gpbResource, 
					gulOutLoc, false);
		break;
	}

	// Just in case no new block is specified, we set the default
	// to code.
	gbt = kbtCode;
	gulOutLoc = gulCodeLoc;
}


/***********************************************************************
 *
 *	Function end implements the END directive.
 *
 ***********************************************************************/

int funct_end(int size, char *label, char *op)
{
	if (size) {
		Error(INV_SIZE_CODE,NULL);
	}
	endFlag = true;

	EndBlock();

	return NORMAL;
}


/***********************************************************************
 *
 *	Function equ implements the EQU directive.
 *
 ***********************************************************************/

int equ(int size, char *label, char *op)
{
  Value value;

  if (size)
	Error(INV_SIZE_CODE,NULL);

  if (!*label)
	Error(LABEL_REQUIRED,NULL);
	
  if (!*op)
  {
	Error(EXPECTED_EXPRESSION,NULL);
	return NORMAL;
  }
  
  op = evaluate(op,&value);
  if (!ErrorStatusIsError())
  {
	SymbolCreate(label,value.kind,NULL,value.value);
	ListPutSymbol(value.value);
  }

  if (op && *op && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}


/***********************************************************************
 *
 *	Function set implements the SET directive.
 *
 ***********************************************************************/

int set(int size, char *label, char *op)
{
  Value value;

  if (size)
	Error(INV_SIZE_CODE,NULL);

  if (!*label)
	Error(LABEL_REQUIRED,NULL);
	
  if (!*op)
  {
	Error(EXPECTED_EXPRESSION,NULL);
	return NORMAL;
  }
  
  op = evaluate(op,&value);
  if (!ErrorStatusIsError())
  {
	SymbolSetRedefineable(SymbolCreate(label,value.kind,NULL,value.value));
	ListPutSymbol(value.value);
  }

  if (op && *op && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}


/***********************************************************************
 *
 *	Function dc implements the DC directive.
 *
 ***********************************************************************/

int dc(int size, char *label, char *op)
{
	Value outVal;
	char string[260], *p;

	// No such thing as 'dc.s'.
	if (size==SHORT)
	{
		Error(INV_SIZE_CODE,NULL);
		size = WORD;
	}
	// If not specified, default size is WORD.
	else if (!size)
		size = WORD;

	// Move location counter to a word boundary and fix the listing if doing
	// DC.W or DC.L (but not if doing DC.B, so DC.B's can be contiguous)
	if ((size & (WORD | LONG)) && (gulOutLoc & 1))
	{
		Error(ALIGNMENT_WARNING,NULL);
		ListPutLocation(++gulOutLoc);
	}

	// Pass 1: Define the label, if any, attached to this directive
	// Pass 2 and 3: Verify the label, if any, already defined
	if (*label)
		SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);

	// Check for the presence of the operand list
	if (!*op)
	{
		Error(SYNTAX,op);
		return NORMAL;
	}

	do {
		if (*op=='"')
		{
			op = ParseQuotedString(op,string);
			if ((*op) && *op!=',' && *op!=';')
			{
				Error(SYNTAX,op);
				return NORMAL;
			}

			p = string;
			while (*p)
			{
				outVal.value = *p++;
				if (size > BYTE) {
					outVal.value = (outVal.value << 8) + *p++;
				}
				if (size > WORD) {
					outVal.value = (outVal.value << 16) + (*p++ << 8);
					outVal.value += *p++;
				}
				if (giPass==2) {
					output(outVal.value, size);
				}
				gulOutLoc += size;
			}
		}
		else
		{
			op = evaluate(op, &outVal);
			if (!op || ErrorStatusIsSevere())
				return NORMAL;
			
			op = skipSpace(op);
			if ((*op) && *op!=',' && *op!=';')
			{
				Error(SYNTAX,op);
				return NORMAL;
			}
			
			if (giPass==2)
				output(outVal.value, size);
			gulOutLoc += size;
			
			if (size==BYTE && (outVal.value<-128 || outVal.value>255))
				Error(INV_8_BIT_DATA,NULL);
			else if (size==WORD && (outVal.value<-32768 || outVal.value>65535))
				Error(INV_16_BIT_DATA,NULL);
		}
	} while (*op++==',');
	--op;

	if ((*op) && *op!=';')
		Error(SYNTAX,op);

	return NORMAL;
}

/***********************************************************************
 *
 *	Function dcb implements the DCB directive.
 *
 ***********************************************************************/

int dcb(int size, char *label, char *op)
{
	Value blockSize, blockVal;
	long i;

	if (size == SHORT) {
		Error(INV_SIZE_CODE,NULL);
		size = WORD;
	} else if (!size) {
		size = WORD;
	}
	/* Move location counter to a word boundary and fix the listing if doing
	   DCB.W or DCB.L (but not if doing DCB.B, so DCB.B's can be contiguous) */
	if ((size & (WORD | LONG)) && (gulOutLoc & 1))
	{
		Error(ALIGNMENT_WARNING,NULL);
		ListPutLocation(++gulOutLoc);
	}

	/* Define the label attached to this directive, if any */
	if (*label) {
		SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);
	}
	/* Evaluate the size of the block (in bytes, words, or longwords) */
	op = evaluate(op, &blockSize);
	if (op && !ErrorStatusIsSevere() && giPass>0 && Guard(blockSize.value,0))
	{
		Error(GUARD_ERROR,NULL);
		return NORMAL;
	}
	if (!op || ErrorStatusIsSevere()) {
		return NORMAL;
	}
	if (*op != ',') {
		Error(SYNTAX,NULL);
		return NORMAL;
	}
	if (blockSize.value < 0) {
		Error(INV_LENGTH,NULL);
		return NORMAL;
	}
	/* Evaluate the data to put in block */
	op = evaluate(op+1, &blockVal);
	if (op && !ErrorStatusIsSevere()) {
		if (*op && *op!=';') {
			Error(SYNTAX,NULL);
			return NORMAL;
		}
		/* On pass 2, output the block of values directly
		   to the object file (without putting them in the listing) */
		if (giPass==2) {
			for (i = 0; i < blockSize.value; i++) {
				outputObj(gulOutLoc, blockVal.value, size);
				gulOutLoc += size;
			}
		} else {
			gulOutLoc += blockSize.value * size;
		}
	}

	return NORMAL;
}


/***********************************************************************
 *
 *	Function ds implements the DS directive.
 *
 ***********************************************************************/

int ds(int size, char *label, char *op)
{
	Value blockSize;

	if (size == SHORT) {
		Error(INV_SIZE_CODE,NULL);
		size = WORD;
	} else if (!size) {
		size = WORD;
	}

	/* Move location counter to a word boundary and fix the listing if doing
	   DS.W or DS.L (but not if doing DS.B, so DS.B's can be contiguous) */
	if ((size & (WORD | LONG)) && (gulOutLoc & 1))
	{
		Error(ALIGNMENT_WARNING,NULL);
		ListPutLocation(++gulOutLoc);
	}

	/* Define the label attached to this directive, if any */
	if (*label) {
		SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);
	}

	/* Evaluate the size of the block (in bytes, words, or longwords) */
	op = evaluate(op, &blockSize);
	if (!ErrorStatusIsSevere() && giPass>0 && Guard(blockSize.value,0)) {
		Error(GUARD_ERROR,NULL);
		return NORMAL;
	}
	if (!op || ErrorStatusIsSevere()) {
		return NORMAL;
	}

	if (*op && *op!=';') {
		Error(SYNTAX,op);
		return NORMAL;
	}
	if (blockSize.value<0) {
		Error(INV_LENGTH,NULL);
		return NORMAL;
	}

	// Set uninitialized data to zeros.
	// OK, this seems strange but for now we're pooling 'uninitialized'
	// data in with the initialized data.
	if (giPass==2) {
		int iblock;

		for (iblock = 0; iblock < blockSize.value; iblock++) {
			output(0, size);
			gulOutLoc += size;
		}
	} else {
		gulOutLoc += blockSize.value * size;
	}

	return NORMAL;
}


//
//
//

int CodeDirective(int size, char *label, char *op)
{
	if (size != 0) {
		Error(INV_SIZE_CODE,NULL);
	}

	// Already in a code block? just return.
	if (gbt == kbtCode) {
		return NORMAL;
	}

	// Save away important state pertaining to the previous block
	EndBlock();

	gpbOutput = gpbCode;
	gulOutLoc = gulCodeLoc;
	gbt = kbtCode;

	return NORMAL;
}

//
//
//

int DataDirective(int size, char *label, char *op)
{
	if (size != 0) {
		Error(INV_SIZE_CODE,NULL);
	}

	// Already in a data block? just return.
	if (gbt == kbtData) {
		return NORMAL;
	}

	// Save away important state pertaining to the previous block
	EndBlock();

	gpbOutput = gpbData;
	gulOutLoc = gulDataLoc;
	gbt = kbtData;

	return NORMAL;
}

//
//
//

int ResDirective(int size, char *label, char *op)
{
	char szT[80];
	char *pch;
	FILE *pfil;
	Value val;

	if (size != 0) {
		Error(INV_SIZE_CODE,NULL);
	}

	// Save away important state pertaining to the previous block
	EndBlock();

	// Parse the res directive's arguments
	op = skipSpace(op);
	op = evaluate(op, &val);
	if (!op || ErrorStatusIsSevere()) {
		return NORMAL;
	}
	if (*op && *op!=',') {
		Error(SYNTAX,op);
		return NORMAL;
	}
	gfcResType = val.value;

	op = skipSpace(op+1);
	op = evaluate(op, &val);
	if (!op ||ErrorStatusIsSevere()) {
		return NORMAL;
	}
	if (*op && *op!=',' && *op!=';') {
		Error(SYNTAX,op);
		return NORMAL;
	}
	gidRes = val.value;
	
	// Check for resource data filename.
	szT[0] = 0;
	if (*op==',') {
		op = skipSpace(op+1);

		if (*op == '"') {
			pch = szT;
			while (*++op != '"') {
				*pch++ = *op;
			}
			*pch++ = 0;
			op = skipSpace(op+1);
		}

		pfil = fopen(szT, "rb");
		if (pfil == NULL || pfil == (FILE *)-1) {
			Error(RESOURCE_OPEN_FAILED,szT);
			return NORMAL;
		}

		gulResLoc = fread(gpbResource, 1, kcbResMax, pfil);
		if (gulResLoc == kcbResMax) {
			Error(RESOURCE_TOO_BIG,szT);
			return NORMAL;
		}
		fclose(pfil);

		AddResource(gfcResType, (unsigned short)gidRes, gpbResource,
					gulResLoc, false);

		gbt = kbtCode;
		gulOutLoc = gulCodeLoc;
		gpbOutput = gpbCode;
	} else {
		gbt = kbtResource;
		gulOutLoc = 0;
		gpbOutput = gpbResource;
	}

	if (OPTION(verbose)) {
		printf("0x%lx, %ld, %s\n", gfcResType, gidRes, szT);
	}

	gulResLoc = 0;

	if (*op && *op!=';')
		Error(SYNTAX,op);

	return NORMAL;
}


boolean IsExisting(const char *pszDir, const char *pszFile)
{
	char szT[_MAX_PATH];
	FILE *pfilT;

	if (pszDir != NULL) {
#ifndef unix
		sprintf(szT, "%s\\%s", pszDir, pszFile);
#else
		sprintf(szT, "%s/%s", pszDir, pszFile);
#endif
	} else {
		strcpy(szT, pszFile);
	}

	pfilT = fopen(szT, "rb");
	if (pfilT == NULL || pfilT == (FILE *)-1) {
		return false;
	}
	fclose(pfilT);

	return true;
}

void TruncateDir(char *pszPath)
{
	char ch;
	char *pchDirEnd = pszPath;

	while ((ch = *pszPath) != 0) {
#ifndef unix
		if (ch == '\\') {
#else
		if (ch == '/') {
#endif
			pchDirEnd = pszPath;
		}
		pszPath++;
	}

	*pchDirEnd = 0;
}

//
//
//

int IncludeDirective(int size, char *label, char *op)
{
	char szFile[_MAX_PATH], szT[_MAX_PATH], szSearchPath[300];
	char *pszT, *pszPilaInc;
	int cchT;
	char tmpbuf[_MAX_PATH];

#ifndef unix
	int i,i2;
#endif

	if (size) {
		Error(INV_SIZE_CODE,NULL);
	}

	// Get the include filename.
	szFile[0] = 0;
	op = skipSpace(op);

	if (*op == '"') {
		char *pch = szFile;
		while (*++op != '"') {
			*pch++ = *op;
		}
		*pch++ = 0;
		op++;
	} else {
		Error(SYNTAX,op);
		return NORMAL;
	}

	//
	// This is the sequence followed for locating an include file:
	//
	// 1. If the path is fully qualified, make sure it exists.
	//	  If not found, fail.
	// 2. Relative to same directory as the including file
	// 3. Relative to current directory
	// 4. Relative to each directory on the PILAINC include path, in order
	// 5. Relative to the dir which contains Pila.exe
	// 6. Relative to <PilaDir>\..\inc

	// 1. If the path is fully qualified, make sure it exists.
	//	  If not found, fail.

#ifndef unix
	cchT = strlen(szFile);
	if (cchT >= 2) {
		if (szFile[1] == ':' || (szFile[0] == '\\' && szFile[1] == '\\')) {
			// Path is fully qualified
			if (!IsExisting(NULL, szFile)) {
				Error(INCLUDE_OPEN_FAILED,szFile);
				return NORMAL;
			}

			// Found the file
			goto lbPushIt;
		}
	}
#else
	cchT = strlen(szFile);
	if (szFile[0] == '/') {
		// Path is fully qualified
		if (!IsExisting(NULL, szFile)) {
			Error(INCLUDE_OPEN_FAILED,szFile);
			return NORMAL;
		}

		// Found the file
		goto lbPushIt;
	}
#endif

	// 2. Relative to same directory as the including file
	strcpy(szT, gasse[gcsse - 1].szFile);
	TruncateDir(szT);
#ifndef unix
	sprintf(szSearchPath, "%s;", szT);
#else
	sprintf(szSearchPath, "%s:", szT);
#endif

	// 3. Relative to current directory
#ifndef unix
	strcat(szSearchPath, ".;");
#else
	strcat(szSearchPath, ".:");
#endif

	// 4. Relative to each directory on the PILAINC include path, in order
	pszPilaInc = getenv("PILAINC");
	if (pszPilaInc != NULL) {
#ifndef unix
		strcat(szSearchPath, pszPilaInc);
		strcat(szSearchPath, ";");
#else
		strcat(szSearchPath, pszPilaInc);
		strcat(szSearchPath, ":");
#endif
	}

#ifndef unix
	// 5. Relative to the dir which contains Pila.exe
	sprintf( szSearchPath, "%s%s\\..\\..\\inc;", szSearchPath, pilafilename );

	// 6. Relative to <PilaDir>\..\inc
	GetModuleFileName(NULL, szT, sizeof(szT));
	TruncateDir(szT);
//	  sprintf(szSearchPath, "%s%s;%s\\..\\inc", szSearchPath, szT, szT);
#endif

#ifndef unix
	for( i = 0, i2 = 0; i < (int)strlen( szSearchPath ); i++ ) {
		if( szSearchPath[i] != ';' ) {
			tmpbuf[i2++] = szSearchPath[i];
		} else {
			tmpbuf[i2] = '\0';
			if (SearchPath(tmpbuf, szFile, NULL, sizeof(szFile), szFile,
				&pszT) != 0) {
				break;
			}
			i2 = 0;
		}
	}
	if( i > (int)strlen( szSearchPath ) ) {
		Error(INCLUDE_OPEN_FAILED,szFile);
		return NORMAL;
	}

#else
	pszT = szSearchPath;
	pszT = strtok(pszT, ":");
	while (pszT != NULL) {
		if (IsExisting(pszT, szFile)) {
			break;
		}
		pszT = strtok(NULL, ":");
	}

	if (pszT == NULL) {
		Error(INCLUDE_OPEN_FAILED,szFile);
		return NORMAL;
	} else {
		strcpy(tmpbuf, pszT);
		strcat(tmpbuf, "/");
		strcat(tmpbuf, szFile);
	}
#endif

lbPushIt:
#ifndef unix
	if (PushSourceFile(szFile) == NULL) {
#else
	if (PushSourceFile(tmpbuf) == NULL) {
#endif
		// PushSourceFile will set the ERROR if there is one
		return NORMAL;
	}

	if (OPTION(verbose)) {
#ifndef unix	
		printf("include \"%s\"\n", szFile);
#else
		printf("include \"%s\"\n", tmpbuf);
#endif
	}

	return NORMAL;
}


#define FILE_SYM_PREFIX		":include:"
#define FILE_SYM_PREFIX_LEN 9

FILE *PushSourceFile(char *pszNextSource)
{
	struct SourceStackEntry *psse;
	char symbolName[_MAX_PATH+1+FILE_SYM_PREFIX_LEN];
	char *fileName;
	SymbolDef *sym;

	strcpy(symbolName,FILE_SYM_PREFIX);
	fileName = symbolName+FILE_SYM_PREFIX_LEN;
	fileName[_MAX_PATH] = '\0';
	
#ifndef unix
	GetFullPathName(pszNextSource, _MAX_PATH, fileName, NULL);
#else
	strncpy(fileName, pszNextSource, _MAX_PATH);
#endif

	sym = SymbolLookup(symbolName);

	if (sym!=NULL && (SymbolGetValue(sym)==giPass)) // || stricmp(symbolName+strlen(symbolName)-4,".inc")==0))
	{
	  return NULL;
	}
	else if (sym==NULL)
	{
	  SymbolCreate(symbolName,symbolKindInclude,NULL,giPass);
	}
	else
	{
	  Value value;
	  value.value = giPass;
	  value.kind  = symbolKindInclude;
	  SymbolSetValue(sym,&value);
	}
	
	if (gcsse == kcsseMax) {
		Error(INCLUDE_NESTED_TOO_DEEP,fileName);
		return NULL;
	}

	psse = &gasse[gcsse];

	psse->pfil = fopen(fileName, "r");
	if (psse->pfil == NULL || psse->pfil == (FILE *)-1) {
		Error(INCLUDE_OPEN_FAILED,fileName);
		return NULL;
	}

	strcpy(psse->szFile, fileName);
	psse->iLineNum = 0;
	gpsseCur = psse;
	gcsse++;

	return gpsseCur->pfil;
}

boolean PopSourceFile()
{
	if (--gcsse <= 0) {
		return false;
	}

	// Close the include file.
	fclose(gpsseCur->pfil);

	// NOTE: This will underflow when the last file (the main source file) is
	// popped but that's OK because it isn't used after that (better not be).
	gpsseCur = &gasse[gcsse - 1];

	return true;
}

//
// Handler for the "appl" directive
//

int ApplDirective(int size, char *label, char *op)
{
	extern char gszAppName[dmDBNameLength];
	extern FourCC gfcCreatorId;
   
	Value val;

	if (size != 0) {
		Error(INV_SIZE_CODE,NULL);
	}

	// Get the application name.
	if (*op == '"') {
		char *pch = gszAppName;
		while (*++op != '"') {
			if (pch-gszAppName<sizeof(gszAppName)-1)
			  *pch++ = *op;
		}
		*pch++ = 0;
		op = skipSpace(op+1);
	} else {
		Error(MISSING_APPL_NAME,op);
		return NORMAL;
	}
	
	// Get the creator id -- NOT OPTIONAL
	if (*op!=',')
	{
		Error(MISSING_CREATOR_ID,op);
		return NORMAL;
	}
	op = skipSpace(op+1);
	op = evaluate(op, &val);
	gfcCreatorId = val.value;
	
	if (op && *op && *op!=';')
	  Error(SYNTAX,op);
	  
	return NORMAL;
}

//
// Handler for the "align" directive
//

int AlignDirective(int size, char *label, char *op)
{
	Value nAlign;

	if (size) {
		Error(INV_SIZE_CODE,NULL);
	}
	if (!*op) {
		Error(SYNTAX,op);
		return NORMAL;
	}
	op = evaluate(op, &nAlign);
	if (op && !ErrorStatusIsError()) {
		if (!*op || *op==';') {

			// Align the output location

			gulOutLoc += nAlign.value - 1;
			gulOutLoc -= gulOutLoc % nAlign.value;

			// Define the label attached to this directive, if any

			if (*label != 0) {
				SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);
			}

			// Show new location counter on listing

			ListPutLocation(gulOutLoc);
		} else {
			Error(SYNTAX,op);
		}
	}

	return NORMAL;
}

//
// Handler for the "list" directive
//

int ListDirective(int size, char *label, char *op)
{
	Value fOn;

	if (size) {
		Error(INV_SIZE_CODE,NULL);
	}
	if (!*op) {
		Error(SYNTAX,op);
		return NORMAL;
	}
	op = evaluate(op, &fOn);
	if (op && !ErrorStatusIsError())
	{
	  if (!*op || *op==';')
	  {
	if (fOn.value==0)
	  ListDisable();
	else
	  ListEnable();
	  }
	  else
	Error(SYNTAX,op);
	}
	return NORMAL;
}

/***********************************************************************
 *
 *	Function IncbinDirective implements the INCBIN directive.
 *
 *	/ Mikael Klasson (fluff@geocities.com)
 *	13 Jan 1998
 *
 ***********************************************************************/

int IncbinDirective(int size, char *label, char *op)
{
	char szFile[_MAX_PATH];
	char buf[4096];
	FILE *in;
	int bytesread;

	if (size != 0) {
		Error(INV_SIZE_CODE,NULL);
	}

	/* Get the incbin filename */
	szFile[0] = 0;
	op = skipSpace(op);

	if (*op == '"') {
		char *pch = szFile;
		while (*++op != '"') {
			*pch++ = *op;
		}
		*pch++ = 0;
		op++;
	} else {
		Error(SYNTAX,op);
		return NORMAL;
	}

	/* Define the label attached to this directive, if any */
	if (*label) {
		SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);
	}

	/* Try to open the file */
	in = fopen(szFile, "rb");
	if (in == NULL || in == (FILE *)-1) {
		Error(INCLUDE_OPEN_FAILED,szFile);
		return NORMAL;
	}

	/* Read 4K at a time */
	while ((bytesread = fread(buf, 1, 4096, in)) != 0) {
		if (giPass==2) {
			memcpy(gpbOutput + gulOutLoc, buf, bytesread);
		}
		gulOutLoc += bytesread;
	}
	fclose(in);

	return NORMAL;
}


int BeginProcDirective(int size, char *label, char *op) // start procedure code
{
  char		 szT[20];
  long		 offset;

  if (!SymbolHasCurrentProc() || procedureBegun)
  {
	Error(UNEXPECTED_BEGINPROC,NULL);
	return NORMAL;
  }
  
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
  
  if (*label)
	Error(LABEL_IGNORED,label);
	
  if (lastLocalSymbol==NULL)
	offset = 0;
  else
	offset = SymbolGetValue(lastLocalSymbol);
	
  sprintf(szT,"#%ld",offset);
  ExpandInstruction("link", "a6", szT);

  procedureBegun = true;
  
  if ((*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}


int EndProcDirective(int size, char *label, char *op) // end of procedure
{
  // get pointer to symbol of current procedure
  SymbolDef *currentProc;

  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
  
  if (*label)
	SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);

  currentProc = SymbolSetCurrentProc(NULL);
  if (!currentProc || !procedureBegun || SymbolGetKind(currentProc)!=symbolKindProcEntry)
  {
	Error(UNEXPECTED_ENDPROC,NULL);
	return NORMAL;
  }
  
  lastLocalSymbol = NULL;
  procedureBegun  = false;
  
  ExpandInstruction("unlk", "a6", NULL);
  ExpandInstruction("rts", NULL, NULL);

  if (OPTION(emit_proc_symbols))
  {
	char szT[SIGCHARS+16];
	int len = strlen(SymbolGetId(currentProc));
	if (len<=31)
	  sprintf(szT,"$%x,\"%s\",0,0", len|0x80, SymbolGetId(currentProc));
	else
	  sprintf(szT,"$80,$%x,\"%s\",0,0", len++, SymbolGetId(currentProc));
	if ((len&1)==0)
	  strcat(szT,",0");
	ExpandInstruction("dc.b", szT, NULL);
  }

  if ((*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}

int EndProxyDirective(int size, char *label, char *op) // end of proxy
{
  char szT[SIGCHARS+16];

  // get pointer to symbol of current procedure
  SymbolDef *currentProc;

  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
  
  if (*label)
	SymbolCreate(label,symbolKindLabel,NULL,gulOutLoc);

  currentProc = SymbolSetCurrentProc(NULL);
  if (!currentProc || !procedureBegun || SymbolGetKind(currentProc)!=symbolKindProxyEntry)
  {
	Error(UNEXPECTED_ENDPROXY,NULL);
	return NORMAL;
  }
  
  procedureBegun = false;
  
  sprintf(szT,"%ld(a7)",SymbolGetValue(SymbolGetType(currentProc)));
  ExpandInstruction("move.l",szT,"-(a7)"); // recover return address from space holder
  ExpandInstruction("rts", NULL, NULL);	   // and return

  if (OPTION(emit_proc_symbols))
  {
	int len = strlen(SymbolGetId(currentProc));
	if (len<=31)
	  sprintf(szT,"$%x,\"%s\",0,0", len|0x80, SymbolGetId(currentProc));
	else
	  sprintf(szT,"$80,$%x,\"%s\",0,0", len++, SymbolGetId(currentProc));
	if ((len&1)==0)
	  strcat(szT,",0");
	ExpandInstruction("dc.b", szT, NULL);
  }

  if ((*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}

int ExternDirective(int size, char *label, char *op) // external variable
{
  boolean lookForDot = (*label=='\0'); /* look for a dot after the name if the name wasn't specified as a label */
  SymbolDef *varType;

  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);

  if (lookForDot)		/* name wasn't specified as label? */
	op = ParseId(op,label); /* then parse it now */
	
  if (!(*label))		/* still no valid name? */
  {				/* then issue an error message */
	Error(EXPECTED_EXTERN_VAR_ID,op);
	return NORMAL;
  }

  op = ParseTypeSpec(op,&varType,lookForDot); // looks for '.' first if name wasn't specified as a label
  if (!varType)		/* no valid type found? */
	return NORMAL;	/* then exit - error message was already issued */

  // create external symbol
  SymbolCreate(label,symbolKindExtern,varType,0);
  
  if ((*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}

int GlobalDirective(int size, char *label, char *op) // global variable
{
	boolean lookForDot = (*label=='\0'); /* if no label than name must follow with a dot and the type */
	SymbolDef *varType;
	
	// if there was a size spec report error
	if (size!=0)
		Error(INV_SIZE_CODE,NULL);
		
	if (gbt!=kbtData)
		Error(GLOBAL_NOT_IN_DATA,NULL);

	if (lookForDot)			/* if there was not label */
		op = ParseId(op,label); /* then the name of the variable must follow the directive */
	
	if (!(*label)) /* still no name? */
	{
		Error(EXPECTED_GLOBAL_VAR_ID,op);
		return NORMAL;
	}
	
	op = ParseTypeSpec(op,&varType,lookForDot); // looks for '.' first if there was no label
	if (!varType)	   /* no valid type found? */
		return NORMAL; /* then return - the error message was issued already */
	
	size = SymbolGetSize(varType);
	if (size>1 && gulOutLoc&1) /* anything bigger than 1 byte must be aligned! */
	{
		Error(ALIGNMENT_WARNING,NULL);
		ListPutLocation(++gulOutLoc);
	}
	
	// create global-symbol
	SymbolCreate(label,symbolKindData,varType,gulOutLoc);

	// Set uninitialized data to zeros.  
	// OK, this seems strange but for now we're pooling 'uninitialized'
	// data in with the initialized data.
	if (giPass==2)
	{
		for (; size>0; size--)
		{
			output(0,1);
			gulOutLoc++;
		}
	}
	else
		gulOutLoc += size;

	if ((*op) && *op!=';')
		Error(SYNTAX,op);

	return NORMAL;
}


int LocalDirective(int size, char *label, char *op) // local variable
{
  boolean lookForDot = (*label=='\0'); /* if no label than name must follow with a dot and the type */
  SymbolDef *varType;
  SymbolDef *symbol;

  if (!SymbolHasCurrentProc())
  {
	Error(UNEXPECTED_LOCAL,NULL);
	return NORMAL;
  }
  
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);

  if (lookForDot)			/* there was not label */
	op = ParseId(op,label); /* thus the name of the variable must follow the directive */
   
  if (!(*label))
  {
	Error(EXPECTED_LOCAL_VAR_ID,op);
	return NORMAL;
  }

  op = ParseTypeSpec(op,&varType,lookForDot); // looks for '.' first if there was no label
  if (!varType)	   /* not a valid type found? */
	return NORMAL; /* then return - error message was issued already */
	
  // create local-symbol (it's value will be calculated
  // automatically based on the already existing local symbols)
  symbol = SymbolCreateScopeProc(label,symbolKindProcLocal,varType,0);
  if (symbol)
	ListPutSymbol(SymbolGetValue(symbol)); 

  lastLocalSymbol = symbol;

  if ((*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}

// used for proc, procdef and trapdef
int EntryPointDirective(SymbolKind kind, int size, char *label, char *op)
{
  long		 symValue	  = 0;
  long		 trapSelector = 0;
  SymbolDef *parms		  = NULL;
  SymbolDef *symbol		  = NULL;
  SymbolKind typeKind	  = symbolKindTypeProc; /* default is for proc and procdef */

  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (!*label)
  {
	// if there was no label specified there
	// MUST be a name behind the keyword
	op = ParseId(op,label);
	if (!*label)
	{
	  Error(EXPECTED_PROC_NAME,op);
	  return NORMAL;
	}
  }
  
  if (SymbolHasCurrentProc() || procedureBegun)
  {
	Error(UNEXPECTED_ENTRY_DEFINITION,label);
	return NORMAL;
  }

  if (kind==symbolKindProcEntry || kind==symbolKindProxyEntry)
  {
	if (gulOutLoc&1)
	{
		Error(ALIGNMENT_WARNING,NULL);
		ListPutLocation(++gulOutLoc);
	}

	symValue = gulOutLoc;
	if (kind==symbolKindProxyEntry)
	  typeKind = symbolKindTypeProxy;
  }
  else if (kind==symbolKindTrapDef)
  {
	char *p;
	char  arg[strlen(op)+1];
	Value value;
	
	if (*op!='[')
	{
	  Error(EXPECTED_LEFT_BRACKET,op);
	  return NORMAL;
	}
	 
	op = ParseArg(op+1,arg,"]:");
	if (!(*arg))
	{
	  Error(MISSING_TRAP_DEF,op);
	  return NORMAL;
	}
	
	p = evaluate(arg, &value);

	if (p && *p)
	  Error(SYNTAX,p);
	if (!p || ErrorStatusIsSevere())
	  return NORMAL;
	if (SymbolGetCategory(value.kind)!=symbolCategoryConst && value.kind!=symbolKindUndefined)
	  Error(INV_VALUE_CATEGORY,NULL);
	  
	symValue = value.value;
	typeKind = symbolKindTypeTrapSimple;

	if (*op==':')	   
	{
	  op = ParseArg(op+1,arg,"]");
	  if (!(*arg))
	  {
		p = evaluate(arg, &value);
		if (p)
		{
		  if (*p && (*p!='.' || (*(p+1)!='w' && *(p+1)!='W') || *(p+2)))
			Error(SYNTAX,p);
		  else if (SymbolGetCategory(value.kind)!=symbolCategoryConst)
			Error(INV_VALUE_CATEGORY,NULL);
		}
		  
		if (!p || ErrorStatusIsSevere())
		  return NORMAL;

		trapSelector = value.value;

		if (!(*p))
		  typeKind = symbolKindTypeTrapSelector;
		else
		  typeKind = symbolKindTypeTrap16BitSel;
	  }
	}
	
	if (*(op)!=']')
	{
	  Error(EXPECTED_RIGHT_BRACKET,op);
	  return NORMAL;
	}
	op = skipSpace(op+1);
  }
  
  symbol = SymbolCreate(label,kind,NULL,symValue);
  parms = SymbolGetType(symbol);
  if (!parms)
  {
	parms = SymbolFactory(NULL,typeKind,NULL,trapSelector);
	SymbolSetType(symbol,parms);
  }
  else if (SymbolGetKind(parms)!=typeKind || 
		   (SymbolGetValue(parms)!=trapSelector && kind==symbolKindTrapDef))
	Error(PHASE_ERROR,SymbolGetId(symbol));
  
  SymbolSetCurrentProc(symbol);

  op = ParseParameters(op,parms,kind);
  
  if (kind==symbolKindProxyEntry)
  {
	// there is no beginproxy since proxies don't have local variables
	// therefore 'beginproxy' funtion must be realized right here
	char szT[20];
	procedureBegun = true; 
	sprintf(szT,"%ld(a7)",SymbolGetValue(parms));
	ExpandInstruction("move.l","(a7)+",szT); // move return address to space holder
  }
  
  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}


int ProxyDirective(int size, char *label, char *op) // proxy definition
{
  return EntryPointDirective(symbolKindProxyEntry,size,label,op);
}

int ProcDirective(int size, char *label, char *op) // procedure definition
{
  return EntryPointDirective(symbolKindProcEntry,size,label,op);
}

int ProcDefDirective(int size, char *label, char *op) // procedure declaration
{
  EntryPointDirective(symbolKindProcDef,size,label,op);
  SymbolSetCurrentProc(NULL);
  return NORMAL;
}

int TrapDefDirective(int size, char *label, char *op) // trap definition
{
  EntryPointDirective(symbolKindTrapDef,size,label,op);
  SymbolSetCurrentProc(NULL);
  return NORMAL;
}

// used for ENUM, STRUCT and UNION
void MemberedTypeDirective(SymbolKind kind, int size, char *label, char *op)
{
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (!*label)
  {
	// if there was no label specified there
	// MUST be a name behind the directive
	op = ParseId(op,label);
  }

  op = skipSpace(op);
  if (*label=='\0')
  {
	switch (kind)
	{
	  case symbolKindTypeEnum:	 Error(EXPECTED_ENUM_NAME,NULL);   break;
	  case symbolKindTypeStruct: Error(EXPECTED_STRUCT_NAME,NULL); break;
	  case symbolKindTypeUnion:	 Error(EXPECTED_UNION_NAME,NULL);  break;
	  default:	Error(INTERNAL_ERROR_INVALID_MEMBERED_TYPE,NULL); break;
	}
	return;
  }
  
  bitmapShiftValue	  = 0;
  bitmapTypeSize	  = 0;
  nextEnumValue.value = 0;
  nextEnumValue.kind  = symbolKindConst;
  
  if (giPass==0)
	insideType = SymbolCreate(label,kind,SymbolFactory(NULL,symbolKindTypeMemberList,NULL,0),0);
  else
	insideType = SymbolLookup(label);
  
  if ((*op) && *op!=';')
	Error(SYNTAX,op);

  return;
}

int EnumDirective(int size, char *label, char *op) // start of enum type
{
  MemberedTypeDirective(symbolKindTypeEnum, size, label, op);
  return NORMAL;
}

int StructDirective(int size, char *label, char *op) // start of struct type
{
  MemberedTypeDirective(symbolKindTypeStruct, size, label, op);
  return NORMAL;
}

int UnionDirective(int size, char *label, char *op)	  // start of union type
{
  MemberedTypeDirective(symbolKindTypeUnion, size, label, op);
  return NORMAL;
}

int EndEnumDirective(int size, char *label, char *op) // start of enum type
{
  Error(UNEXPECTED_ENUM,NULL);
  return NORMAL;
}

int EndStructDirective(int size, char *label, char *op) // start of struct type
{
  Error(UNEXPECTED_STRUCT,NULL);
  return NORMAL;
}

int EndUnionDirective(int size, char *label, char *op)	 // start of union type
{
  Error(UNEXPECTED_UNION,NULL);
  return NORMAL;
}

int ParseEnumMember(char *symbolId, char *op)
{
  SymbolDef *symbol;
  
  op = skipSpace(op);
  if (*op=='=')
  {
	op = evaluate(op+1,&nextEnumValue);
	if (!op || ErrorStatusIsSevere())
	  return NORMAL;
	else if (SymbolGetCategory(nextEnumValue.kind)!=symbolCategoryConst)
	  Error(INV_VALUE_CATEGORY,NULL);
	nextEnumValue.kind = symbolKindConst;
  }
  
  symbol = SymbolCreateTypeMember(insideType,symbolId,insideType,nextEnumValue.value++);
  if (symbol)
	ListPutSymbol(SymbolGetValue(symbol));
	
  if ((*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}

int ParseStructOrUnionMember(char *symbolId, char *op)
{
  SymbolDef *memberType;

  op = ParseTypeSpec(op,&memberType,true); // looks for '.' first
  if (!memberType)
	return NORMAL;
  
  if (SymbolGetKind(insideType)==symbolKindTypeStruct && (*op==':' || bitmapShiftValue!=0))
  {
	Value bitmapBitCount;
	if (*op==':')
	{
	  op = evaluate(op+1,&bitmapBitCount);
	  if (!op || ErrorStatusIsSevere())
		return NORMAL;

	  if (bitmapShiftValue==0)
	  {
		bitmapTypeSize	 = SymbolGetSize(memberType);
		bitmapShiftValue = bitmapTypeSize*8;
	  }

	  if (bitmapTypeSize!=SymbolGetSize(memberType))
	  {
		Error(INVALID_BITMAP_MEMBER_TYPE,SymbolGetId(memberType));
		return NORMAL;
	  }
	  else if (bitmapShiftValue<bitmapBitCount.value)
	  {
		Error(BIT_COUNT_TOO_BIG,NULL);
		return NORMAL;
	  }
	  else
	  {
		bitmapShiftValue = bitmapShiftValue-bitmapBitCount.value;
		memberType = SymbolFactory(NULL,
								   symbolKindTypeBitmapMember,
								   memberType,
								   bitmapShiftValue);
		SymbolCreateTypeMember(memberType,"mask",NULL,0xFFFFFFFFUL>>(32-bitmapBitCount.value));
		SymbolCreateTypeMember(memberType,"size",NULL,bitmapBitCount.value);
		SymbolCreateTypeMember(memberType,"shift",NULL,bitmapShiftValue);
	  }
	}
	else 
	{
	  // a few bits have been left dangling in the last bitmap,
	  // so make sure the next bitmap starts fresh
	  bitmapShiftValue = 0;
	  bitmapTypeSize   = 0;
	}
  }
  
  memberType = SymbolCreateTypeMember(insideType,symbolId,memberType,0);  
  if (memberType)
	ListPutSymbol(SymbolGetValue(memberType));

  if ((*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}

void EndMemberedType(char *op)
{
  if (insideType)
	ListPutSymbol((long)SymbolGetSize(insideType));

  insideType		  = NULL;
  nextEnumValue.value = 0;
  nextEnumValue.kind  = symbolKindConst;
  
  op = skipSpace(op);
  if (*op && *op!=';')
	Error(SYNTAX,op);
}


char *ProcessCallArg(char *op,int *stackByteCount,SymbolDef *prevType)
{
  char *p;
  char	arg[strlen(op)+1];
  int		 currSize = 0;
  SymbolDef *parmType = NULL;

  if (prevType)
  {
	parmType = SymbolGetNext(prevType);
	if (parmType && SymbolGetKind(parmType)!=symbolKindProcParm)
	  parmType = NULL;
	if (parmType)
	  currSize = SymbolGetSize(parmType);
  }
  
  op = ParseArg(op,arg,"),");
  
  if (*arg && prevType && !parmType && (SymbolGetId(prevType)==NULL || strcmp("...",SymbolGetId(prevType))!=0))
	Error(TOO_MANY_PARAMETERS,arg);
  else if (!*arg && parmType && strcmp("...",SymbolGetId(parmType))!=0)
  {
	Error(MISSING_PARAMETERS,SymbolGetId(parmType));
	strcpy(arg,"#0");
  }
  else if (!*arg && !parmType)
	return op;

  if (*op==',')
	op++;
  
  op = ProcessCallArg(op,stackByteCount,parmType);

  // first check if with the parameter there is also a type specification
  // to specify a type for a parameter the actual parameter must be enclosed
  // in parenthesis and the right one followed by a period and the type spec
  p = arg;
  if (arg[0]=='(')
  {
	int i;
	for (i=strlen(arg)-1; i>=0; i--)
	{
	  if (arg[i]==')')
	  {
		if (arg[i+1]=='.')
		{
		  p = ParseTypeSpec(&(arg[i+1]),&prevType,true);
		  if (prevType!=NULL)
		  {
			int typeSize = SymbolGetSize(prevType);
			if (currSize!=0 && currSize!=typeSize)
			  Error(UNMATCHING_TYPE_SIZES,NULL);
			currSize = typeSize;
			if (*p)
			  Error(SYNTAX,p);
			p = &(arg[1]); // the actual parameter starts at arg[1]
			arg[i] = '\0'; // and ends at arg[i-1]
		  }
		}
		i = 0; // exit the loop
	  }
	}
  }

  if (*p=='&') 
  {
	if (currSize!=4 && currSize!=0)
	  Error(INV_PARM_SIZE,NULL);
	ExpandInstruction("pea",p+1,NULL);
	*stackByteCount += 4;
  }
  else if (strcmp(p,"#0")==0)
  {
	switch (currSize)
	{
	  case 1:
		  ExpandInstruction("clr.b","-(a7)",NULL);
		  *stackByteCount += 2;
		  break;
	  case 2:
		  ExpandInstruction("clr.w","-(a7)",NULL);
		  *stackByteCount += 2;
		  break;
	  case 0: // to ensure max. code size for (still) undefined types
	  case 4:
		  ExpandInstruction("clr.l","-(a7)",NULL);
		  *stackByteCount += 4;
		  break;
	  default:
		  Error(INV_PARM_SIZE,NULL);
		  break;
	}
  }
  else
  {
	switch (currSize)
	{
	  case 1:
		  ExpandInstruction("move.b",p,"-(a7)");
		  *stackByteCount += 2;
		  break;
	  case 2:
		  ExpandInstruction("move.w",p,"-(a7)");
		  *stackByteCount += 2;
		  break;
	  case 0: // to ensure max. code size for (still) undefined types
	  case 4:
		  ExpandInstruction("move.l",p,"-(a7)");
		  *stackByteCount += 4;
		  break;
	  default:
		  Error(INV_PARM_SIZE,NULL);
		  break;
	}
  }
  return op;
}

int CallDirective(int size, char *label, char *op) // call procedure or trap
{
  int		 stackByteCount;
  char		 targetId[SIGCHARS];
  SymbolDef *jmpTarget = NULL;
  
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);

  if (*label)
  {
	ExpandString(label);
	ExpandString(":\n");
  }
  
  op = ParseId(op,targetId);
  if (!*targetId)
  {
	Error(EXPECTED_PROC_NAME,NULL);
	return NORMAL;
  }
  
  jmpTarget = SymbolLookup(targetId);
  if (!jmpTarget)
  {
	// create implicit declaration so the existence of the symbol
	// can be used as an indication, that it was referenced
	jmpTarget = SymbolCreate(targetId,symbolKindProcDef,NULL,0);
  }
	
  if (SymbolGetKind(jmpTarget)==symbolKindProcDef)
  {
	if (SymbolGetType(jmpTarget)==NULL)
	{
	  // if the type isn't set the symbol is the result of an implicit
	  // declaration therefore the symbol must be considered undefined
	  Error(UNDEFINED_SYMBOL,targetId);
	}
	else
	  Error(DECLARED_BUT_UNDEFINED_PROC,targetId);
  }
  else if (SymbolGetKind(jmpTarget)!=symbolKindProcEntry && 
		   SymbolGetKind(jmpTarget)!=symbolKindTrapDef	 &&
		   SymbolGetKind(jmpTarget)!=symbolKindProxyEntry)
  {
	if (giPass)
	  Error(NOT_A_PROCEDURE_NOR_TRAP,targetId);
	return NORMAL;
  }

  op = skipSpace(op);
  if (*op!='(')
  {
	Error(EXPECTED_LEFT_PAREN,op);
	return NORMAL;
  }

  if (jmpTarget && SymbolGetKind(jmpTarget)==symbolKindProxyEntry)
	ExpandInstruction("subq.l","#4","a7"); // create space holder for return address
	
  // ProcessCallArg will call itself recursively to process the last parameter first
  // therefore stackByteCount must be initialized prior to entry.
  stackByteCount = 0;
  op = ProcessCallArg(op+1,&stackByteCount,SymbolGetType(jmpTarget));
  if (*op!=')')
	Error(EXPECTED_RIGHT_PAREN,op);
  else
	op++;
  op = skipSpace(op);

  if (jmpTarget && SymbolGetKind(jmpTarget)==symbolKindProxyEntry)
	stackByteCount += 4; // account for the space holder for return address	   
  
  // if jmpTarget is NULL (undefined) a trap with 16-bit-selector is generated
  // that is the maximum sized call there is
  if (jmpTarget && (SymbolGetKind(jmpTarget)==symbolKindProcEntry || // is it a JSR we need?
					SymbolGetKind(jmpTarget)==symbolKindProxyEntry))
  {
	ExpandString("\tjsr\t");
	ExpandString(SymbolGetId(jmpTarget));
	ExpandString("(pc)\n");
  }
  else // we need to generate a trap call
  {
	int kind = SymbolGetKind(SymbolGetType(jmpTarget));
	char sz[24];
	if (kind!=symbolKindTypeTrapSimple)
	{
	  sprintf(sz,"#$%lX",jmpTarget ? SymbolGetValue(SymbolGetType(jmpTarget)) : 0);
	  if (kind==symbolKindTypeTrapSelector)
	  {
		ExpandInstruction("moveq",sz,"d2");
	  }
	  else
	  {
		ExpandInstruction("move.w",sz,"-(a7)");
		stackByteCount += 2;
	  }
	}
	
	sprintf(sz,"$%lX",jmpTarget ? SymbolGetValue(jmpTarget) : 0);
	ExpandInstruction("trap", "#15", NULL);
	ExpandInstruction("dc.w", sz, NULL);
  }

  if (stackByteCount>0)
  {
	char sz[24];
	if (stackByteCount<=8)
	{
	  sprintf(sz, "#%d", stackByteCount);
	  ExpandInstruction("addq.l", sz, "a7");
	}
	else
	{
	  sprintf(sz, "%d(a7)", stackByteCount);
	  ExpandInstruction("lea", sz, "a7");
	}
  }

  jmpTarget = SymbolGetType(jmpTarget);
  if (jmpTarget)
  {
	jmpTarget = SymbolGetType(jmpTarget);
	if (jmpTarget)
	  ListPutTypeName(SymbolGetId(jmpTarget));
  }
  if ((*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}


int TypeDefDirective(int size, char *label, char *op) // type definition
{
  boolean lookForDot = (*label=='\0'); /* if type name wasn't specified as label it must follow with a dot */ 
  SymbolDef *type;

  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (lookForDot)		/* if no label was specified */
	op = ParseId(op,label); /* then the type name must follow the directive */

  if (!(*label)) // still no type name?
  {		 // then issue an error message
	Error(EXPECTED_TYPE_NAME,NULL);
	return NORMAL;
  }
  
  if ((lookForDot && *op=='.' && *(op+1)=='(') ||
	  (!lookForDot && *op=='('))
  {
	SymbolDef *parmList;
	if (giPass==0)
	{
	  parmList = SymbolFactory(NULL,symbolKindTypeProc,NULL,0);
	  SymbolCreate(label,symbolKindTypeAlias,parmList,0);
	}
	else
	  parmList = SymbolGetType(SymbolLookup(label));

	if (lookForDot)
	  op++;

	op = ParseParameters(op,parmList,symbolKindProcDef);
  }
  else
  {
	op = ParseTypeSpec(op,&type,lookForDot); // looks for '.' first if type name wasn't specified as label
	// ignore if error or a typedef with the same id for both
	if (type && strcmp(label,SymbolGetId(type))!=0)
	  SymbolCreate(label,symbolKindTypeAlias,type,0);
  }

  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}


int IfDirective(int size, char *label, char *op)
{
  Value value;
  char *opSave = op;
  
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (*label)
	Error(LABEL_IGNORED,label);
  
  ifLevel++;
  if (ifLevel<MAX_IF_LEVEL)
	ifElseFlag[ifLevel] = false;
  
  if (ifNoGenLevel==0)
  {
	op = evaluate(op,&value);
	if (!op || ErrorStatusIsSevere())
	  value.value = 0;
	else if (value.value!=0 && value.value!=1)
	{
	  value.value = 0;
	  Error(INVALID_BOOLEAN_VALUE,opSave);
	}
  
	if (value.value==0)
	  ifNoGenLevel++;
  }
  else
	ifNoGenLevel++;
	
  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}


int IfDefDirective(int size, char *label, char *op)
{
  char symbolId[SIGCHARS+1];
  char *opSave = op;
  
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (*label)
	Error(LABEL_IGNORED,label);
  
  ifLevel++;
  if (ifLevel<MAX_IF_LEVEL)
	ifElseFlag[ifLevel] = false;

  if (ifNoGenLevel==0)
  {
	op = skipSpace(ParseId(op,symbolId));
	if (!*symbolId)
	{
	  Error(EXPECTED_SYMBOL,opSave);
	  ifNoGenLevel++;
	}
	else if (SymbolLookup(symbolId)==NULL)
	  ifNoGenLevel++;
  }
  else
	ifNoGenLevel++;
	
  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}


int IfNDefDirective(int size, char *label, char *op)
{
  char symbolId[SIGCHARS+1];
  char *opSave = op;
  
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (*label)
	Error(LABEL_IGNORED,label);
  
  ifLevel++;
  if (ifLevel<MAX_IF_LEVEL)
	ifElseFlag[ifLevel] = false;

  if (ifNoGenLevel==0)
  {
	op = skipSpace(ParseId(op,symbolId));
	if (!*symbolId)
	{
	  Error(EXPECTED_SYMBOL,opSave);
	  ifNoGenLevel++;
	}
	else if (SymbolLookup(symbolId)!=NULL)
	  ifNoGenLevel++;
  }
  else
	ifNoGenLevel++;
  
  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);
	
  return NORMAL;
}


int ElseDirective(int size, char *label, char *op)
{
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (*label)
	Error(LABEL_IGNORED,label);

  if (ifLevel==0)
	Error(UNEXPECTED_ELSE_MISSING_IF,NULL);
  else	if (ifLevel<MAX_IF_LEVEL && ifElseFlag[ifLevel]==true)
	Error(UNEXPECTED_ELSE_MULTIPLE,NULL);
  else
  {
	if (ifLevel<MAX_IF_LEVEL)
	  ifElseFlag[ifLevel] = true;
	  
	if (ifNoGenLevel==0)
	  ifNoGenLevel = 1;
	else if (ifNoGenLevel==1)
	  ifNoGenLevel = 0;
  }
  
  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}


int EndIfDirective(int size, char *label, char *op)
{
  // if there was a size spec report error
  if (size!=0)
	Error(INV_SIZE_CODE,NULL);
	
  if (*label)
	Error(LABEL_IGNORED,label);

  if (ifLevel==0)
  {
	Error(UNMATCHED_ENDIF,NULL);
	ifNoGenLevel = 0;
  }
  else
  {
	ifLevel--;
	if (ifNoGenLevel>0)
	  ifNoGenLevel--;
  }

  if (op && (*op) && *op!=';')
	Error(SYNTAX,op);

  return NORMAL;
}


boolean DirectiveContinuation(char *line)
{
  char		  symbolId[SIGCHARS+1];
  char		 *op;
  
  if (ifNoGenLevel>0)
  {
	op = ParseId(skipSpace(line),symbolId);
	if (stricmp(symbolId,"IF")==0 ||
		stricmp(symbolId,"IFDEF")==0 ||
		stricmp(symbolId,"IFNDEF")==0 ||
		stricmp(symbolId,"ELSE")==0 ||
		stricmp(symbolId,"ENDIF")==0)
	  return false;
	else
	  return true;
  }
  else if (insideType!=NULL)
  {
	SymbolKind	kind = SymbolGetKind(insideType);
	op = ParseId(skipSpace(line),symbolId);
	if (!(*symbolId))
	{
	  switch (kind)
	  {
		case symbolKindTypeEnum:   Error(EXPECTED_ENUM_MEMBER,NULL);   break;
		case symbolKindTypeStruct: Error(EXPECTED_STRUCT_MEMBER,NULL); break;
		case symbolKindTypeUnion:  Error(EXPECTED_UNION_MEMBER,NULL);  break;
		default:	Error(INTERNAL_ERROR_INVALID_MEMBERED_TYPE,NULL);  break;
	  }
	  return true;
	}

	if (stricmp(symbolId,"ENDENUM")==0)
	{
	  if (kind==symbolKindTypeEnum)
		EndMemberedType(op);
	  else
		Error(UNEXPECTED_ENDENUM,NULL);
	}
	else if (stricmp(symbolId,"ENDSTRUCT")==0)
	{
	  if (kind==symbolKindTypeStruct)
		EndMemberedType(op);
	  else
		Error(UNEXPECTED_ENDSTRUCT,NULL);
	}
	else if (stricmp(symbolId,"ENDUNION")==0)
	{
	  if (kind==symbolKindTypeUnion)
		EndMemberedType(op);
	  else
		Error(UNEXPECTED_ENDUNION,NULL);
	}
	else if (stricmp(symbolId,"ENUM")==0)
	  Error(UNEXPECTED_ENUM,NULL);
	else if (stricmp(symbolId,"STRUCT")==0)
	  Error(UNEXPECTED_STRUCT,NULL);
	else if (stricmp(symbolId,"UNION")==0)
	  Error(UNEXPECTED_UNION,NULL);
	else
	{
	  if (kind==symbolKindTypeEnum)
		ParseEnumMember(symbolId,op);
	  else
		ParseStructOrUnionMember(symbolId,op);
	}
	return true;
  }
  return false;
}


int ErrorDirective(int size, char *label, char *op)
{
  Error(USER_ERROR,skipSpace(op));
  return NORMAL;
}


