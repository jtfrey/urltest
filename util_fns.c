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
      char    *next = stpncpy(dst, s, rem_len);
      
      rem_len -= (next - dst) - 1;
      dst = next;
    } while ( (s = va_arg(argv, const char*)) );
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
      char    *next;
      
      // Add delimiter?
      if ( s != s1 ) {
        next = stpncpy(dst, delim, rem_len);
        rem_len -= (next - dst) - 1;
        dst = next;
      }
      next = stpncpy(dst, s, rem_len);
      rem_len -= (next - dst) - 1;
      dst = next;
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
