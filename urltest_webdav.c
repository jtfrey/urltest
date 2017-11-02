//
//  urltest_webdav.c
//  
//  Orchestrate a sequence of WebDAV operations to synchronize
//  a directory to a remote URL.
//
//

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <curl/curl.h>

#include "fs_entity.h"
#include "http_ops.h"

//

#ifndef URLTEST_WEBDAV_VERSION_STRING
# error URLTEST_WEBDAV_VERSION_STRING is not defined
#endif
const char   *urltest_webdav_version_string = URLTEST_WEBDAV_VERSION_STRING;

//

static const struct option urltest_webdav_options[] = {
    { "help",             no_argument,          NULL,       'h' },
    //
    { "long-listing",     no_argument,          NULL,       'l' },
    { "short-listing",    no_argument,          NULL,       's' },
    { "no-listing",       no_argument,          NULL,       'n' },
    { "ascii",            no_argument,          NULL,       'a' },
    //
    { "verbose",          no_argument,          NULL,       'v' },
    { "dry-run",          no_argument,          NULL,       'd' },
    { "show-timings",     no_argument,          NULL,       't' },
    { "generations",      required_argument,    NULL,       'g' },
    //
    { "base-url",         required_argument,    NULL,       'U' },
    { "host-mapping",     required_argument,    NULL,       'm' },
    { "username",         required_argument,    NULL,       'u' },
    { "password",         required_argument,    NULL,       'p' },
    { NULL,               0,                    NULL,        0  }
  };

static const char *urltest_webdav_optstring = "h" "lsna" "dvtg:" "U:m:u:p:";

//

void
usage(
  char      *exe
)
{
  printf(
      "version %s\n"
      "built " __DATE__ " " __TIME__ "\n"
      "usage:\n\n"
      "  %s {options} <directory> {<directory> ..}\n\n"
      " options:\n\n"
      "  --help/-h                    show this information\n"
      "\n"
      "  --long-listing/-l            list the discovered file hierarchy in an extended\n"
      "                               format\n"
      "  --short-listing/-s           list the discovered file hierarchy in a compact\n"
      "                               format\n"
      "  --no-listing/-n              do not list the discovered file hierarchy\n"
      "  --ascii/-a                   restrict to ASCII characters\n"
      "\n"
      "  --verbose/-v                 display additional information to stdout as the\n"
      "                               program progresses\n"
      "  --dry-run/-d                 do not perform any HTTP requests, just show an\n"
      "                               activity trace\n"
      "  --show-timings/-t            show HTTP timing statistics at the end of the run\n"
      "  --generations/-g <#>         maximum number of generations to iterate\n"
      "\n"
      "  --base-url/-U <URL>          the base URL to which the content should be mirrored\n"
      "  --host-mapping/-m <hostmap>  provide a static DNS mapping for a hostname and TCP/IP\n"
      "                               port\n"
      "\n"
      "                                 <hostmap> = <hostname>:<port>:<ip address>\n"
      "\n"
      "  --username/-u <string>       use HTTP basic authentication with the given string as\n"
      "                               the username\n"
      "  --password/-p <string>       use HTTP basic authentication with the given string as\n"
      "                               the password\n"
      "\n",
      urltest_webdav_version_string,
      exe
    );
}

//

int
main(
  int               argc,
  char* const       *argv
)
{
  int                       rc = 0, opt;
  bool                      is_verbose = false;
  bool                      is_dry_run = false;
  bool                      should_show_file_list = true;
  bool                      should_show_timings = false;
  unsigned int              generations = 1;
  fs_entity_print_format    print_format = fs_entity_print_format_default;
  fs_entity_print_format    print_charset = 0;
  const char                *base_url = NULL;
  http_ops_ref              http_ops = http_ops_create();
  
#ifdef HAVE_SRANDOMDEV
  srandomdev();
#elif HAVE_SRANDOM
  srandom(time(NULL));
#else
  srand(time(NULL));
#endif /* HAVE_SRANDOMDEV, HAVE_SRANDOM */

  optind = 0;
  while ( (opt = getopt_long(argc, argv, urltest_webdav_optstring, urltest_webdav_options, NULL)) != -1 ) {
    switch ( opt ) {
      
      case 'h':
        usage(argv[0]);
        exit(0);
      
      case 'l':
        print_format = (fs_entity_print_format_default & ~fs_entity_print_format_name) | fs_entity_print_format_path;
        break;
        
      case 's':
        print_format = fs_entity_print_format_short;
        break;
      
      case 'n':
        should_show_file_list = false;
        break;
      
      case 'a':
        print_charset = fs_entity_print_format_ascii;
        break;
      
      case 'v':
        is_verbose = true;
        break;
      
      case 'd':
        is_dry_run = true;
        break;
      
      case 't':
        should_show_timings = true;
        break;
      
      case 'g': {
        if ( optarg && *optarg ) {
          char          *endp;
          long          value = strtol(optarg, &endp, 10);
          
          if ( (value > 0) && (endp > optarg) ) {
            generations = value;
          } else {
            fprintf(stderr, "ERROR:  invalid argument to --generations/-g:  %s\n", optarg);
            exit(EINVAL);
          }
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --generations/-g option\n");
          exit(EINVAL);
        }
        break;
      }
      
      case 'U': {
        if ( optarg && *optarg ) {
          char      *endp = optarg + strlen(optarg) - 1;
          
          if ( base_url ) {
            free((void*)base_url);
            base_url = NULL;
          }
          while ( *endp == '/' ) endp--;
          base_url = strndup(optarg, endp - optarg + 1);
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --base-url/-U option\n");
          exit(EINVAL);
        }
        break;
      }
      
      case 'm': {
        if ( optarg && *optarg ) {
          if ( ! http_ops_add_host_mapping_string(http_ops, optarg) ) {
            fprintf(stderr, "ERROR:  unable to add host mapping (errno = %d)\n", errno);
            exit(errno);
          }
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --host-mapping/-m option\n");
          exit(EINVAL);
        }
        break;
      }
      
      case 'u': {
        if ( optarg && *optarg ) {
          if ( ! http_ops_set_username(http_ops, optarg) ) {
            fprintf(stderr, "ERROR:  unable to set HTTP authentication username (errno = %d)\n", errno);
            exit(errno);
          }
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --username/-u option\n");
          exit(EINVAL);
        }
        break;
      }
      
      case 'p': {
        if ( optarg && *optarg ) {
          if ( ! http_ops_set_password(http_ops, optarg) ) {
            fprintf(stderr, "ERROR:  unable to set HTTP authentication username (errno = %d)\n", errno);
            exit(errno);
          }
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --password/-p option\n");
          exit(EINVAL);
        }
        break;
      }
      
    }
  }
  
  if ( is_verbose && base_url ) {
    printf("\nMirroring content to '%s'\n", base_url);
  }
  
  while ( optind < argc ) {
    fs_entity_list    *fslist = fs_entity_list_create_with_path(argv[optind]);
    
    if ( fslist ) {
      fs_entity       *e;
      unsigned int    current_generation = 1;
      
      if ( should_show_file_list ) fs_entity_list_print(print_format | print_charset, fslist);
      
      if ( is_verbose ) {
        printf("\nCommencing %u iteration%s...\n", generations, (generations == 1) ? "" : "s");
      }
      
      while ( (e = fs_entity_list_random_node(fslist, generations)) ) {
        if ( is_verbose && (fslist->generation == current_generation) ) {
          printf("Generation %u completed\n", current_generation++);
        }
        if ( ! is_dry_run ) {
          const char  *url = fs_entity_list_url_for_entity(fslist, base_url, e);
          
          if ( url ) {
            bool      ok = false;
            long      http_status = -1L;
            
            switch ( e->kind ) {
              
              case fs_entity_kind_directory: {
                switch ( e->state ) {
                  case fs_entity_state_upload: {
                    ok = http_ops_mkdir(http_ops, url, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_getinfo: {
                    ok = http_ops_getinfo(http_ops, url, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_download: {
                    ok = http_ops_download(http_ops, url, NULL, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_delete: {
                    ok = http_ops_delete(http_ops, url, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_download_sub:
                  case fs_entity_state_upload_sub:
                  case fs_entity_state_delete_sub:
                  case fs_entity_state_max:
                    // Just to please the compilers that want complete enumeration coverage:
                    break;
                }
                break;
              }
              
              case fs_entity_kind_file: {
                switch ( e->state ) {
                  case fs_entity_state_upload: {
                    ok = http_ops_upload(http_ops, e->path, url, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_getinfo: {
                    ok = http_ops_getinfo(http_ops, url, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_download: {
                    ok = http_ops_download(http_ops, url, NULL, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_delete: {
                    ok = http_ops_delete(http_ops, url, e->http_stats, &http_status);
                    break;
                  }
                  
                  case fs_entity_state_upload_sub:
                  case fs_entity_state_download_sub:
                  case fs_entity_state_delete_sub:
                  case fs_entity_state_max:
                    // Just to please the compilers that want complete enumeration coverage:
                    break;
                }
                break;
              }
              
              case fs_entity_kind_max:
                break;
            
            }
            if ( is_verbose ) {
              printf("%-3ld ", http_status);
              if ( print_format ) {
                fs_entity_print(print_format | print_charset, e);
              } else {
                printf("%s\n", url);
              }
            }
            free((void*)url);
            if ( ok ) fs_entity_advance_state(e);
          }
        } else {
          if ( print_format ) fs_entity_print(print_format | print_charset, e);
          fs_entity_advance_state(e);
        }
      }
      if ( is_verbose ) {
        printf("Generation %u completed\n", current_generation);
      }
      if ( ! is_dry_run && should_show_timings && print_format ) {
        printf("\nTiming information:\n\n");
        fs_entity_list_stats_print(print_format | print_charset, fslist);
      }
      fs_entity_list_destroy(fslist);
    }
    optind++;
  }
  
  return rc;
}
