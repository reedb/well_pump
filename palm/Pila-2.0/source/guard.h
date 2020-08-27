/**********************************************************
 *
 **********************************************************/

#ifndef __GUARD_H__
#define __GUARD_H__

#define GUARD_LONG_BRANCH    1L
#define GUARD_SHORT_BRANCH   2L
#define GUARD_REGLIST_LEFT   3L
#define GUARD_REGLIST_RIGHT  4L
#define GUARD_USE_MOVEQ      5L
#define GUARD_USE_MOVE       6L
#define GUARD_USE_QUICKMATH  7L
#define GUARD_NO_QUICKMATH   8L

boolean Guard(long value,int subId);
long    GuardGet(int subId);

#endif // __GUARD_H__
