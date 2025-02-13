/************************************************************

  A super simple boolean datatype 'bool' implementation
  for C compilers that don't understand boolean datatypes.

  Just a header 

 ************************************************************/
#ifndef __C_BOOL_H__
#define __C_BOOL_H__

#if __STDC_VERSION__ >= 19990L

/* C99 or above has stdbool.h */
#include <stdbool.h>

#else

typedef enum { false, true } bool;

#ifndef TRUE
#define TRUE           true
#endif

#ifndef FALSE
#define FALSE          false
#endif

#endif /* __STDC_VERSION__ >= 19990L */

#endif /* Include Guard */