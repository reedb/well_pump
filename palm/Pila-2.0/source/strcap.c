// Uppercase everything that isn't inside of single or double quotes
// 1998-02-17 Michael Dreher: changed to handle also mixed single/double 
// quote char cases (e.g. "my sister's husband" or just "'")
#include "safe-ctype.h"

void strcap(char *d, char *s)
{
    char quoteChar;

    quoteChar = '\0';
    while (*s) {
        if (!quoteChar) {
            // start of a quoted string?
            if (*s == '\'' || *s == '\"') {
                quoteChar = *s; // remember the quote char
            }
            *d = TOUPPER(*s);
        } else {
            // end of quoted string?
            if (*s == quoteChar) {
                quoteChar = '\0';
            }
            *d = *s;
        }
        d++;
        s++;
    }
    *d = '\0';
}


