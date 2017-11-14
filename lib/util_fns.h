//
// util_fns.h
//

#ifndef __UTIL_FNS_H__
#define __UTIL_FNS_H__

#include "config.h"

char* strmcat(const char *s1, ...);
char* strmcatd(const char *delim, const char *s1, ...);

#ifndef HAVE_ASPRINTF
int asprintf(char* *ret, const char *format, ...);
#endif /* HAVE_ASPRINTF */

void init_random_long();
long int random_long_int();
long int random_long_int_in_range(long int low, long int high);

#endif /* __UTIL_FNS_H__ */
