// object.c
//
// Author: Darrin Massena (darrin@massena.com)
// Date: 6/24/96

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

int outputObj(long lOutLoc, long data, int size)
{
    unsigned char *pbOutput = gpbOutput + lOutLoc;

    switch (size) {
    case BYTE:
        *(unsigned char *)pbOutput = (unsigned char)data;
        break;
    case WORD:
        *(unsigned short *)pbOutput = htons((unsigned short)data);
        break;
    case LONG:
        *(unsigned long *)pbOutput = htonl((unsigned long)data);
        break;
    }

    return NORMAL;
}

