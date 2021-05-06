#ifndef __PILA_H__
#define __PILA_H__

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

typedef enum
{
  false = 0,
  true  = 1
} boolean;

#include "error.h"
#include "listing.h"

#ifndef unix /* DPN */
    #include <Windows.h>
#endif

#ifndef _MAX_PATH
    #define _MAX_PATH PATH_MAX
#endif

#ifdef unix
    #define stricmp strcasecmp
    #define strcmpi strcasecmp
#endif

#define Assert(f) assert(f)

#endif /* __PILA_H__ */

