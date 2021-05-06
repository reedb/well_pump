
#include <stdio.h>
#include "options.h"

int SetArgFlags(int cpszArgs, char *apszArgs[])
{
    int i;

	// set default for database_type
	strcpy(OPTION(database_type),"appl");
	
    for (i = 1; i < cpszArgs && apszArgs[i][0] == '-'; i++) {
        char ch;
        char *pszArg = apszArgs[i] + 1, *pch;

        while ((ch = *pszArg++) != 0) {
            switch (ch) {
            case 'd':
                OPTION(verbose) = true;
                break;
            case 'c':
                OPTION(const_expanded) = true;
                break;
            case 'l':
                OPTION(listing) = true;
                break;
            case 'r':
                OPTION(resources_only) = true;
                break;
            case 'h':
            case '?':
                help();
                break;
            case 's':
                OPTION(emit_proc_symbols) = true;
                break;
            case 't':
                if (*pszArg != 0) {
                    fprintf(stdout, "-t must be followed by a space and a "
                            "four character type.\n");
                    return 0;
                }

                if (i + 1 >= cpszArgs) {
                    fprintf(stdout, "-t requires four character type.\n");
                    return 0;
                }

                pch = apszArgs[++i];
                if (strlen(pch) != 4) {
                    fprintf(stdout, "-t requires four character type.\n");
                    return 0;
                }

                strcpy(OPTION(database_type),pch);
                break;

            default:
                fprintf(stdout, "Unknown option -%c\n", ch);
                return 0;
            }
        }
    }

    return i;
}


/**********************************************************************
 *
 *  Function help() prints out a usage explanation if a bad
 *  option is specified or no filename is given.
 *
 *********************************************************************/

void help()
{
    puts("Usage: pila [-cldrs] [-t TYPE] infile.ext\n");
    puts("Options: -c  Show full constant expansions for DC directives");
    puts("         -l  Produce listing file (infile.lis)");
    puts("         -d  Debugging output");
    puts("         -r  Resources only, don't generate code or data");
    puts("         -s  Include debugging symbols in output");
    puts("    -t TYPE  Specify the PRC type. Default is appl");
    exit(0);
}
