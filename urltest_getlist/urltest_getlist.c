//
//  urltest_getlist.c
//  
//  Given a list of URLs read from stdin (or a file), fetch each
//  in sequence and keep aggregate statistics.
//
//

#include "config.h"

#include <getopt.h>

#include <curl/curl.h>

#include "util_fns.h"
#include "http_ops.h"
#include "http_stats.h"

//

static const struct option urltest_getlist_options[] = {
    { "help",             no_argument,          NULL,       'h' },
    //
    { "verbose",          no_argument,          NULL,       'v' },
    { "verbose-curl",     no_argument,          NULL,       'V' },
    { "dry-run",          no_argument,          NULL,       'd' },
    { "show-timings",     optional_argument,    NULL,       't' },
    //
    { "base-url",         required_argument,    NULL,       'U' },
    { "url-list",         required_argument,    NULL,       'l' },
    { "host-mapping",     required_argument,    NULL,       'm' },
    { "username",         required_argument,    NULL,       'u' },
    { "password",         required_argument,    NULL,       'p' },
    { "retries",          required_argument,    NULL,       'r' },
    { "no-cert-verify",   no_argument,          NULL,       'k' },
    { "follow-3xx",       no_argument,          NULL,       'f' },
    { NULL,               0,                    NULL,        0  }
  };

static const char *urltest_getlist_optstring = "h" "vVdt" "U:l:m:u:p:r:kf";

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
      "  %s {options} <entity> {<entity> ..}\n\n"
      " options:\n\n"
      "  --help/-h                    show this information\n"
      "\n"
      "  --verbose/-v                 display additional information to stdout as the\n"
      "                               program progresses\n"
      "  --verbose-curl/-V            ask cURL to display verbose request progress to\n"
      "                               stderr\n"
      "  --dry-run/-d                 do not perform any HTTP requests, just show an\n"
      "                               activity trace\n"
      "  -t                           show HTTP timing statistics as a table to stdout\n"
      "  --show-timings=<out>         show HTTP timing statistics at the end of the run\n"
      "\n"
      "                                 <out> = <format>{:<path>}\n"
      "                                 <format> = table | csv | tsv\n"
      "\n"
      "  --base-url/-U <remote URL>   prepend the given <remote URL> to each URL read from the\n"
      "                               url list; implies that URLs on the url list will be path\n"
      "                               components that should be appended to a base URL\n"
      "  --url-list/-l <path>         read URLs from the given <path>, one per line; use a dash\n"
      "                               to indicate the default (stdin)\n"
      "  --host-mapping/-m <hostmap>  provide a static DNS mapping for a hostname and TCP/IP\n"
      "                               port\n"
      "\n"
      "                                 <hostmap> = <hostname>:<port>:<ip address>\n"
      "\n"
      "  --username/-u <string>       use HTTP basic authentication with the given string as\n"
      "                               the username\n"
      "  --password/-p <string>       use HTTP basic authentication with the given string as\n"
      "                               the password\n"
      "  --retries/-r <#>             for failed cURL requests retry this number of times before\n"
      "                               hard failing the URL in question (default: 1)\n"
      "  --no-cert-verify/-k          do not require SSL certificate verfication for connections\n"
      "                               to succeed\n"
      "  --follow-3xx/-f              attempt to follow all HTTP 3XX responses to the eventual\n"
      "                               non-3XX target\n"
      "\n"
      " environment:\n"
      "\n"
      "   URLTEST_GETLIST_USER        default user name for HTTP requests; is overridden by\n"
      "                               the --username/-u option\n"
      "   URLTEST_GETLIST_PASSWORD    password to use for authenticated HTTP requests; is\n"
      "                               overridden by the --password/-p option\n"
      "\n"
      ,
      urltest_version_string,
      exe
    );
}

//

void
http_error_exit(
  const char  *url,
  long        http_status
)
{
  bool      do_not_exit = false;
  int       rc = EPERM;
  
  switch ( http_status / 100 ) {
  
    case 4: {
      fprintf(stderr, "REQUEST ERROR(%ld) for '%s' : ", http_status, url);
      switch ( http_status ) {
        case 400:
          fprintf(stderr, "request was not properly constructed\n");
          rc = EINVAL;
          break;
        
        case 401:
          fprintf(stderr, "authentication was required and credentials did not work\n");
          rc = EACCES;
          break;
        
        case 403:
          fprintf(stderr, "access forbidden by server, unable to proceed further\n");
          rc = EACCES;
          break;
        
        case 408:
          fprintf(stderr, "request timed out, will try again\n");
          do_not_exit = true;
          break;
        
        default:
          fprintf(stderr, "unable to proceed further\n");
          break;
      }
      break;
    }
    
    case 5: {
      fprintf(stderr, "SERVER ERROR(%ld): ", http_status);
      switch ( http_status ) {
        case 507:
          fprintf(stderr, "no room left on device\n");
          rc = ENOSPC;
          break;
          
        case 506:
        case 508:
          fprintf(stderr, "referential loop detected\n");
          rc = ELOOP;
          break;
        
        default:
          fprintf(stderr, "unable to proceed further\n");
          break;
      }
      break;
    }
  
  }
  if ( ! do_not_exit ) exit(rc);
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
  bool                      should_show_timings = false;
  bool                      should_follow_3xx = false;
  int                       retries = 1;
  http_stats_format					stats_format = http_stats_format_table;
  http_stats_ref            aggr_stats = http_stats_create();
  http_ops_ref              http_ops = http_ops_create();
  const char								*timing_output = NULL;
  const char                *base_url = NULL;
  size_t                    base_url_len;
  bool                      does_base_url_have_terminal_slash;
  const char                *url_list = NULL;
  FILE                      *url_stream = stdin;
  size_t                    next_url_len;
  const char                *next_url;
  
  if ( getenv("URLTEST_GETLIST_USER") ) {
    http_ops_set_username(http_ops, getenv("URLTEST_GETLIST_USER"));
  }
  
  if ( getenv("URLTEST_GETLIST_PASSWORD") ) {
    http_ops_set_password(http_ops, getenv("URLTEST_GETLIST_PASSWORD"));
  }

  optind = 0;
  while ( (opt = getopt_long(argc, argv, urltest_getlist_optstring, urltest_getlist_options, NULL)) != -1 ) {
    switch ( opt ) {
      
      case 'h':
        usage(argv[0]);
        exit(0);
      
      case 'd':
        is_dry_run = true;
        break;
      
      case 'v':
        is_verbose = true;
        break;
      
      case 'V':
        http_ops_set_is_verbose(http_ops, true);
        break;
      
      case 't': {
        should_show_timings = true;
        if ( optarg && *optarg ) {
        	char			*colon = strchr(optarg, ':');
        	size_t		format_len;
        	
        	if ( colon == NULL ) {
        		format_len = strlen(colon);
        	} else {
        		format_len = colon - optarg;
        	}
        	if ( strncasecmp(optarg, "table", format_len) == 0 ) {
        		stats_format = http_stats_format_table;
        	} else if ( strncasecmp(optarg, "csv", format_len) == 0 ) {
        		stats_format = http_stats_format_csv;
        	} else if ( strncasecmp(optarg, "tsv", format_len) == 0 ) {
        		stats_format = http_stats_format_tsv;
        	} else {
        		fprintf(stderr, "ERROR:  invalid timing output specification: %s\n", optarg);
        		exit(EINVAL);
        	}
        	if ( colon ) timing_output = colon + 1;
        	break;
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
      
      case 'l': {
        if ( optarg && *optarg ) {
          if ( *optarg == '-' && *(optarg+1) == '\0' ) {
            // Sure, go ahead and use stdin!
          } else {
            url_list = optarg;
          }
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --url-list/-l option\n");
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
      
      case 'r': {
        if ( optarg && *optarg ) {
          char          *endp;
          long          value = strtol(optarg, &endp, 10);
          
          if ( (value >= 0) && (endp > optarg) ) {
            retries = value;
          } else {
            fprintf(stderr, "ERROR:  invalid argument to --retries/-r:  %s\n", optarg);
            exit(EINVAL);
          }
        } else {
          fprintf(stderr, "ERROR:  no argument provided with --retries/-r option\n");
          exit(EINVAL);
        }
        break;
      }
      
      case 'k':
        http_ops_set_ssl_verify_peer(http_ops, false);
        break;
        
      case 'f':
        should_follow_3xx = true;
        break;
      
    }
  }
  
  if ( base_url ) {
    base_url_len = strlen(base_url);
    does_base_url_have_terminal_slash = (base_url[base_url_len - 1] == '/') ? true : false;
    if ( is_verbose ) printf("Will prepend '%s' to all URLs\n", base_url);
  }
  
  //
  // Should 3xx responses be followed to an eventual target?
  //
  http_ops_set_should_follow_redirects(http_ops, should_follow_3xx);
  
  //
  // All set, get the url_list open:
  //
  if ( url_list ) {
    url_stream = fopen(url_list, "r");
    if ( ! url_stream ) {
      int   rc = errno;
      
      fprintf(stderr, "ERROR:  unable to open url list `%s` for reading (errno = %d)\n", url_list, errno);
      exit(rc);
    }
  }
  
  while ( (next_url = fgetln(url_stream, &next_url_len)) ) {
    char            *target_url = NULL;
    
    //
    // Drop any trailing whitespace:
    //
    while ( next_url_len && isspace(next_url[next_url_len - 1]) ) next_url_len--;
      
    //
    // If there's a base url provided, append next_url to that:
    //
    if ( base_url ) {
      int             need_slash = 0;
      char            *target_url_ptr;
      size_t          target_url_len;
      
      if ( does_base_url_have_terminal_slash ) {
        //
        // Remove leading slashes from next_url:
        //
        while ( next_url_len && (*next_url == '/') ) next_url++, next_url_len--;
      } else {
        //
        // Do we need to prepend a slash?
        //
        if ( next_url_len && (*next_url != '/') ) need_slash = 1;
      }
      //
      // Allocate the target_url buffer:
      //
      target_url_len = base_url_len + need_slash + next_url_len + 1;
      target_url = malloc(target_url_len);
      if ( target_url ) {
        char        *target_url_ptr = target_url;
        
        //
        // Compile the base_url, possible slash, and next_url into
        // the buffer:
        //
        target_url_ptr = stpncpy(target_url_ptr, base_url, base_url_len + 1);
        if ( need_slash ) target_url_ptr = stpncpy(target_url_ptr, "/", 2);
        stpncpy(target_url_ptr, next_url, next_url_len);
        target_url_ptr[next_url_len] = '\0';
      }
    } else if ( next_url_len > 0 ) {
      //
      // Just duplicate next_url to target_url:
      //
      target_url = strndup(next_url, next_url_len);
    }
    if ( target_url ) {
      if ( is_dry_run ) {
        printf("<- %s\n", target_url);
      } else {
        http_stats_record   req_stats;
        http_stats_field    i_f;
        long                http_status = -1;
        int                 retry_count = 0;

retry:
        if ( http_ops_download(http_ops, target_url, NULL, aggr_stats, &req_stats, &http_status) ) {
          if ( is_verbose ) {
            bool            line_done = false;
            
            printf("T,%ld,\"%s\"", http_status, target_url);
            for ( i_f = http_stats_field_dns; i_f < http_stats_field_max; i_f++ ) printf(",%lg", req_stats[i_f]);
            
            if ( (http_status / 100) == 3 ) {
              CURL    *curl_handle = http_ops_curl_handle_for_request(http_ops, http_ops_curl_request_get);
              
              if ( curl_handle ) {
                char    *url = NULL;
                
                curl_easy_getinfo(curl_handle, CURLINFO_REDIRECT_URL, &url);
                if ( url ) {
                  printf(",\"%s\"\n", url);
                  line_done = true;
                }
              }
            }
            else if ( http_status == 200 ) {
              CURL    *curl_handle = http_ops_curl_handle_for_request(http_ops, http_ops_curl_request_get);
              
              if ( curl_handle ) {
                char    *url = NULL;
                
                curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &url);
                if ( url ) {
                  printf(",\"%s\"\n", url);
                  line_done = true;
                }
              }
            }
            if ( ! line_done ) printf(",\n");
          }
        } else {
          if ( retry_count++ < retries ) goto retry;
          printf("F,%ld,\"%s\"", 0L, target_url);
          for ( i_f = http_stats_field_dns; i_f < http_stats_field_max; i_f++ ) printf(",0");
          printf(",\"%s\"\n", http_ops_get_error_buffer(http_ops));
        }
      }
      free(target_url);
    }
  }
  
  if ( ! is_dry_run && should_show_timings ) {
    if ( ! timing_output ) {
      printf("Timing information:\n\n");
      http_stats_print(stats_format, http_stats_print_flags_none, aggr_stats);
    } else {
      FILE			*timing_fptr = fopen(timing_output, "w");
      
      if ( timing_fptr ) {
        http_stats_fprint(timing_fptr, stats_format, http_stats_print_flags_none, aggr_stats);
        fclose(timing_fptr);
      } else {
        fprintf(stderr, "ERROR:  unable to open timing file for writing: %s\n", timing_output);
        rc = errno;
      }
    }
  }
  
  //
  // Close the url_list we opened:
  //
  if ( url_list && (url_stream != stdin) ) fclose(url_stream);
  
  return rc;
}
