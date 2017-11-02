//
// util_fns.h
//

#ifndef __UTIL_FNS_H__
#define __UTIL_FNS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char* strmcat(const char *s1, ...);
char* strmcatd(const char *delim, const char *s1, ...);

#ifndef HAVE_ASPRINTF
int asprintf(char* *ret, const char *format, ...);
#endif /* HAVE_ASPRINTF */

#endif /* __UTIL_FNS_H__ */
