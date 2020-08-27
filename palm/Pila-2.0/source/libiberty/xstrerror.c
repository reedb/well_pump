/* xstrerror.c -- jacket routine for more robust strerror() usage.
   Fri Jun 16 18:30:00 1995  Pat Rankin  <rankin@eql.caltech.edu>
   This code is in the public domain.  */

/*

@deftypefn Replacement char* xstrerror (int @var{errnum})

Behaves exactly like the standard @code{strerror} function, but
will never return a @code{NULL} pointer.

@end deftypefn

*/

#include <stdio.h>
#include "libiberty.h"

extern char *strerror PARAMS ((int));

/* If strerror returns NULL, we'll format the number into a static buffer.  */

#define ERRSTR_FMT "undocumented error #%d"
static char xstrerror_buf[sizeof ERRSTR_FMT + 20];

/* Like strerror, but result is never a null pointer.  */

char *
xstrerror (errnum)
     int errnum;
{
  char *errstr;
  errstr = strerror (errnum);
  /* If `errnum' is out of range, result might be NULL.  We'll fix that.  */
  if (!errstr)
    {
      sprintf (xstrerror_buf, ERRSTR_FMT, errnum);
      errstr = xstrerror_buf;
    }
  return errstr;
}
