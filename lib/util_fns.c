//
// util_fns.h
//

#include <stdarg.h>

#include "util_fns.h"

char*
strmcat(
  const char  *s1,
  ...
)
{
  va_list     argv;
  const char  *s = s1;
  size_t      total_len = 0;
  
  va_start(argv, s1);
  do {
    total_len += strlen(s);
  } while ( (s = va_arg(argv, const char*)) );
  va_end(argv);
  
  if ( total_len > 0 ) {
    size_t    rem_len = total_len + 1;
    char      *out_str = malloc(rem_len);
    char      *dst = out_str;
    
    s = s1;
    va_start(argv, s1);
    do {
      while ( rem_len && *s ) {
        *dst++ = *s++;
        rem_len--;
      }
      *dst = '\0';
    } while ( rem_len && (s = va_arg(argv, const char*)) );
    va_end(argv);
    
    return out_str;
  }
  return NULL;
}

//

char*
strmcatd(
  const char  *delim,
  const char  *s1,
  ...
)
{
  va_list     argv;
  const char  *s = s1;
  size_t      total_len = 0;
  size_t      delim_len = strlen(delim);
  
  va_start(argv, s1);
  do {
    total_len += ((s == s1) ? 0 : delim_len) + strlen(s);
  } while ( (s = va_arg(argv, const char*)) );
  va_end(argv);
  
  if ( total_len > 0 ) {
    size_t    rem_len = total_len + 1;
    char      *out_str = malloc(rem_len);
    char      *dst = out_str;
    
    s = s1;
    va_start(argv, s1);
    do {
      // Add delimiter?
      if ( s != s1 ) {
        const char  *d = delim;
        
        while ( rem_len && *d ) {
          *dst++ = *d++;
          rem_len--;
        }
      }
      while ( rem_len && *s ) {
        *dst++ = *s++;
        rem_len--;
      }
      *dst = '\0';
    } while ( (s = va_arg(argv, const char*)) );
    va_end(argv);
    
    return out_str;
  }
  return NULL;

}

//

#ifndef HAVE_ASPRINTF

int
asprintf(
  char*       *ret,
  const char  *format,
  ...
)
{
  int         s_len;
  char        *s;
  va_list     argv;
  
  va_start(argv, format);
  s_len = vsnprintf(NULL, 0, format, argv);
  va_end(argv);
  
  if ( s_len < 0 ) return s_len;
  
  if ( (s = malloc(s_len + 1)) ) {
    va_start(argv, format);
    s_len = vsnprintf(s, s_len + 1, format, argv);
    va_end(argv);
    
    *ret = s;
    return s_len;
  } 
  return -1;
}

#endif /* HAVE_ASPRINTF */

//

void
init_random_long()
{
#ifdef HAVE_SRANDOMDEV
  srandomdev();
#else
# ifdef HAVE_SRANDOM
  srandom(time(NULL));
# else
  srand(time(NULL));
# endif /* HAVE_SRANDOM */
#endif /* HAVE_SRANDOMDEV */
}

//

long int
random_long_int()
{
#ifdef HAVE_RANDOM
  return random();
#else
  if ( sizeof(long int) < sizeof(int) ) {
    long int  n;
    int       i = 0, iMax = (sizeof(long int) / sizeof(int));
    int       *p = (int*)&n;
    
    while ( i++ < iMax ) {
      *p++ = rand();
    }
    return n;
  }
  return rand();
#endif
}

//

long int
random_long_int_in_range(
  long int    low,
  long int    high
)
{
  long        n;
  
  if ( low == high ) return low;

#ifdef HAVE_RANDOM
  n = random();
#else
  if ( sizeof(long int) == sizeof(int) ) {
    n = rand();
  } else {
    int       i = 0, iMax = (sizeof(long int) / sizeof(int));
    int       *p = (int*)&n;
    
    while ( i++ < iMax ) {
      *p++ = rand();
    }
  }
#endif

  return low + (n % (high - low + 1));
}

//
#ifdef UTIL_FNS_TEST

#include <stdio.h>

int
main()
{
  char    *s = strmcatd("/", "", "Users", "frey", "Desktop", NULL);
  
  printf("%s\n", s);
  return 0;
}

#endif /* UTIL_FNS_TEST */
