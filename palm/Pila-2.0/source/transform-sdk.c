
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "ansidecl.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "obstack.h"
#include "hashtable.h"
#include "crc32.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

typedef struct
{  
	ht_identifier id;
	union
	{
		char *token;
		long number;
	} value;
} TableEntry;

hashnode allocEntry(hash_table *ht)
{
	TableEntry *entry = (TableEntry *)xmalloc(sizeof(TableEntry));
	entry->value.number = 0L;
	return (hashnode)entry;
}

char GetCharWithCRC(FILE *inFile)
{
	char chr = fgetc(inFile);
	if (chr!='\r' && chr!=EOF)
		CRC32Add((unsigned char)chr);
	return chr;
}

void PutCharWithCRC(char chr,FILE *outFile)
{
	fputc(chr,outFile);
	if (chr!='\r')
		CRC32Add(chr);
}

void PutStringWithCRC(char *str,FILE *outFile)
{
	fputs(str,outFile);
	for ( ; *str; str++)
	{
		if (*str!='\r')
			CRC32Add(*str);
	}
}

void processFile(char* inFileName)
{
	char *outFileName;
	char *hdrFileName;
	
	FILE *inFile;
	FILE *outFile;
	FILE *hdrFile;
	char chr;
	char *token;
	int  i;
	long wrdCnt = 0;
	TableEntry *entry;
	int  mode = 0; /* 0=unknown, 1=inc->sdk, 2=sdk->inc */
	unsigned long crc = 0l;
	
	hash_table *htab = NULL;
	struct obstack *stack;
	
	for (i=strlen(inFileName)-1; i>=0 && inFileName[i]!='.' ; i--)
		;
	
	if (strcasecmp(inFileName+i,".inc")==0)
		mode = 1; // inc->sdk
	else if (strcasecmp(inFileName+i,".sdk")==0)
		mode = 2; // sdk->inc
	else
		printf("*** Input file name must have extension .inc (%s)\n",inFileName);
	
	if (mode)
	{
		htab = ht_create(11);
		htab->alloc_node = &allocEntry;
		stack = &(htab->stack);
		
		obstack_init(stack);
		
		obstack_grow(stack,inFileName,i+1);
		if (mode==1)
			obstack_grow0(stack,"sdk",3);
		else
			obstack_grow0(stack,"inc",3);
		outFileName = obstack_finish(stack);
		
		obstack_grow(stack,inFileName,i+1);
		obstack_grow0(stack,"h",1);
		hdrFileName = obstack_finish(stack);
		
		hdrFile = fopen(hdrFileName,"r");
		if (hdrFile==NULL || hdrFile==(FILE *)-1)
			printf("*** No header file found to transform %s (skipped)\n",inFileName);
		else
		{
			// now fill up the hashtable with all the identifiers we can find in the file
			// that are made up from consecutive digits, letters and '_'.
			
			chr = fgetc(hdrFile);
			while (chr!=EOF)
			{
				if (ISALNUM(chr) || chr=='_')
				{
					do
					{
						obstack_1grow(stack,chr);
						chr = fgetc(hdrFile);
					} while (ISALNUM(chr) || chr=='_');
					
					obstack_1grow(stack,'\0');
					token = obstack_finish(stack);
					wrdCnt++;
					entry = (TableEntry *)ht_lookup(htab,token,strlen(token),HT_ALLOCED);
					
					if (entry->value.number==0)
					{
						entry->value.number = wrdCnt;
						if (mode==2) // sdk->inc?
						{
							// If we are in mode sdk->inc we need to find the word for a specific number. Therefore
							// we need to save the number in our hash table as well and it's value pointing to the word.
							//
							// Since there aren't gonna be more than 2^24-1 words in any of the header files
							// the identifier &wrdCnt with sizeof(wrdCnt) length will alway include a \0 and
							// will therefore not interfere with the valid strings inserted into the table
							// by the previous ht_lookup.
							entry = (TableEntry *)ht_lookup(htab,(unsigned char *)&wrdCnt,sizeof(wrdCnt),HT_ALLOC);
							entry->value.token = token;
						}
					}
				}
				chr = fgetc(hdrFile);
			}
			fclose(hdrFile);
			
			inFile = fopen(inFileName,"r");
			if (inFile==NULL || inFile==(FILE *)-1)
				printf("*** Could not open input file %s\n",inFileName);
			else
			{
				outFile = fopen(outFileName,"w");
				if (outFile==NULL || outFile==(FILE *)-1)
					printf("*** Could not open output file %s\n",outFileName);
				else
				{
					if (mode==1) // inc->sdk
					{
						CRC32Reset(); // reset CRC calculation

						chr = GetCharWithCRC(inFile);
						while (chr!=EOF)
						{
							if (ISALNUM(chr) || chr=='_')
							{
								do
								{
									obstack_1grow(stack,chr);
									chr = GetCharWithCRC(inFile);
								} while (ISALNUM(chr) || chr=='_');
								obstack_1grow(stack,'\0');
								token = obstack_finish(stack);
								entry = (TableEntry *)ht_lookup(htab,token,strlen(token),HT_NO_INSERT);
								if (entry==NULL)
									fputs(token,outFile);
								else
									fprintf(outFile,"@%ld@",entry->value.number);
								obstack_free(stack,token);
								// the character in chr is not alphanumeric or '_'
								// therefore it is save to write it out to the file
								// without further checks.
							}
							fputc(chr,outFile);
							if (chr=='@')
								fputc(chr,outFile);
							chr = GetCharWithCRC(inFile);
						}
						// the file is processed and we can now add the CRC32 value of the original .inc file
						fprintf(outFile,"@*%8.8lX",CRC32Get());
					}
					else // mode==2 sdk->inc
					{
						CRC32Reset(); // reset CRC calculation

						chr = fgetc(inFile);
						while (chr!=EOF)
						{
							if (chr=='@')
							{
								chr = fgetc(inFile);
								if (chr=='*')
								{	// we are done with the file content and the CRC32 value
									// of the original .inc file follows
									fscanf(inFile,"%8lX",&crc);
									if (crc!=CRC32Get())
										printf("*** CRC32 mismatch detected when processing %s (sdk=%8.8lX, inc=%8.8lX)\n",
										       inFileName,crc,CRC32Get());
									chr = EOF; // done with this file
								}
								else if (chr!='@') // not just an escaped at-symbol?
								{
									wrdCnt = 0;
									while (ISDIGIT(chr))
									{
										wrdCnt = wrdCnt*10+(long)(chr-'0');
										chr = fgetc(inFile);
									}
									entry = (TableEntry *)ht_lookup(htab,(unsigned char *)&wrdCnt,sizeof(wrdCnt),HT_NO_INSERT);
									if (entry==NULL || chr!='@' || wrdCnt==0)
									{
										printf("*** Aborted! Not a valid or unmatching transform file: %s\n",inFileName);
										chr = EOF;
									}
									else		  
										PutStringWithCRC(entry->value.token,outFile);	      
								}
								else // output the escaped at-symbol
									PutCharWithCRC(chr,outFile);
							}
							else // just copy the char to the output
								PutCharWithCRC(chr,outFile);
							
							if (chr!=EOF)
								chr = fgetc(inFile); // and off to the next one
						}

					}
					fclose(outFile);
				}
				fclose(inFile);
			}
		}
		ht_destroy(htab);
	}
}


int main(int argc, char *argv[])
{
	char *buffer = xmalloc(1024);
	long  bufsize = 1024;
	long  offset  = 0;
	FILE *listFile;
	char *readRtn;
	
	if (argc<2)
		printf("*** Missing parameter with file name of list file\n");
	else
	{
		if (argc>2)
			printf("*** Extranous parameter(s) ignored!\n");
		listFile = fopen(argv[1],"r");
		if (listFile==NULL || listFile==(FILE*)-1)
			printf("*** Could not open list file %s\n",argv[1]);
		else
		{
			do
			{
				*buffer = '\0';
				readRtn = fgets(buffer,bufsize,listFile);
				offset = strlen(buffer);
				while (readRtn!=NULL && buffer[offset-1]!='\n')
				{
					bufsize *= 2;
					buffer = xrealloc(buffer,bufsize);
					readRtn = fgets(buffer+offset,bufsize-offset,listFile);
					offset += strlen(buffer+offset);
				}
				while (offset>0 && ISSPACE(*(buffer+(--offset))))
					*(buffer+offset) = '\0'; // remove white-space at the end (incl. LF)
				if (offset>0)
					processFile(buffer);
			} while (readRtn!=NULL);
		}
	}
	return 0;
}

