//
// config.c
//

#include "config.h"

const char   *urltest_version_string = URLTEST_VERSION_STRING;


#ifndef HAVE_FGETLN

/* This implementation of fgetln(3) returns pointer to data, which may be
 * modified by later calls. Unlike original FreeBSD implementation, the
 * allocated (with help of malloc()/realloc()) memory isn't automatically
 * free()'d when the stream is fclose()'d. The returned pointer may be free()'d
 * by the application at any time, but in some cases this function itself frees
 * the memory.
 */

/* Emulation of FreeBSD's style of the implementation of streams */
char *__fgetln_int_buf = NULL;
/* Size of the allocated memory */
size_t __fgetln_int_len = 0;

char *
fgetln(FILE *fp, size_t *lenp)
{
    /* SKYNICK: Implementation note: "== -1" isn't equal to "< 0"... May be
     * getline(3) page will have more notes/examples in future... */
    if(((*lenp) = getline (&__fgetln_int_buf, &__fgetln_int_len, fp)) == -1) {
	if (__fgetln_int_buf != NULL) {
	    free(__fgetln_int_buf);
	}
	__fgetln_int_buf = NULL;
	__fgetln_int_len = 0;
	(*lenp) = 0;
    }

    return __fgetln_int_buf;
}

#endif /* HAVE_FGETLN */

#ifndef HAVE_STRNDUP

char*
strndup(
  const char *s1,
  size_t n
)
{
  size_t    s1_len = 0;
  char      *dst, *s = s1;
  
  while ( (s1_len < n) && (*s) ) s1_len++, s++;
  if ( (dst = malloc(s1_len + 1)) ) {
    s = dst;
    while ( s1_len-- ) *s++ = *s1++;
    *s = '\0';
  }
  return dst;
}

#endif
