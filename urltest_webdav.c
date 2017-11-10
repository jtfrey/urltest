//
//  urltest_webdav.c
//  
//  Orchestrate a sequence of WebDAV operations to synchronize
//  a directory to a remote URL.
//
//

#include "config.h"

#include <getopt.h>

#include <curl/curl.h>

#include "util_fns.h"
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
    { "verbose-curl",     no_argument,          NULL,       'V' },
    { "dry-run",          no_argument,          NULL,       'd' },
    { "show-timings",     optional_argument,    NULL,       't' },
    { "generations",      required_argument,    NULL,       'g' },
    //
    { "base-url",         required_argument,    NULL,       'U' },
    { "host-mapping",     required_argument,    NULL,       'm' },
    { "no-delete",        no_argument,          NULL,       'D' },
    { "username",         required_argument,    NULL,       'u' },
    { "password",         required_argument,    NULL,       'p' },
    { "no-cert-verify",   no_argument,          NULL,       'k' },
    { "no-random-walk",   no_argument,          NULL,       'W' },
    { "ranged-ops",       no_argument,          NULL,       'r' },
    { "no-options",       no_argument,          NULL,       'O' },
    { NULL,               0,                    NULL,        0  }
  };

static const char *urltest_webdav_optstring = "h" "lsna" "vVdtg:" "U:m:Du:p:kWrO";

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
      "  --generations/-g <#>         maximum number of generations to iterate\n"
      "\n"
      "  --base-url/-U <URL>          the base URL to which the content should be mirrored\n"
      "  --host-mapping/-m <hostmap>  provide a static DNS mapping for a hostname and TCP/IP\n"
      "                               port\n"
      "\n"
      "                                 <hostmap> = <hostname>:<port>:<ip address>\n"
      "\n"
      "  --no-delete/-D               do not delete anything on the remote side\n"
      "  --username/-u <string>       use HTTP basic authentication with the given string as\n"
      "                               the username\n"
      "  --password/-p <string>       use HTTP basic authentication with the given string as\n"
      "                               the password\n"
      "  --no-cert-verify/-k          do not require SSL certificate verfication for connections\n"
      "                               to succeed\n"
      "  --no-random-walk/-W          process the file list as a simple depth-first traversal\n"
      "  --ranged-ops/-r              enable ranged GET operations\n"
      "  --no-options/-O              disable OPTIONS operations\n"
      "\n",
      urltest_webdav_version_string,
      exe
    );
}

//

void
http_error_exit(
  long      http_status
)
{
  bool      do_not_exit = false;
  int       rc = EPERM;
  
  switch ( http_status / 100 ) {
  
    case 4: {
      fprintf(stderr, "REQUEST ERROR(%ld): ", http_status);
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

typedef fs_entity* (*fs_entity_list_node_selector_fn)(fs_entity_list *the_list, unsigned int max_generation);

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
  bool                      should_delete = true;
  bool                      should_do_random_walk = true;
  bool                      should_do_ranged_ops = false;
  bool                      should_do_options = true;
  unsigned int              generations = 1;
  fs_entity_print_format    print_format = fs_entity_print_format_default;
  fs_entity_print_format    print_charset = 0;
  http_stats_format					stats_format = http_stats_format_table;
  const char                *base_url = NULL;
  http_ops_ref              http_ops = http_ops_create();
  const char								*timing_output = NULL;

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
      
      case 'D':
        should_delete = false;
        break;
      
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
      
      case 'k':
        http_ops_set_ssl_verify_peer(http_ops, false);
        break;
        
      case 'W':
        should_do_random_walk = false;
        break;
      
      case 'r':
        should_do_ranged_ops = true;
        break;
      
      case 'O':
        should_do_options = false;
        break;
      
    }
  }
  
  if ( should_do_random_walk ) {
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
  
  if ( is_verbose && base_url ) {
    printf("\nMirroring content to '%s'\n", base_url);
  }
  
  while ( optind < argc ) {
    fs_entity_list    *fslist = fs_entity_list_create_with_path(argv[optind]);
    
    if ( fslist ) {
      fs_entity                           *e;
      unsigned int                        current_generation = 1;
      const char                          *real_base_url = base_url;
      fs_entity_list_node_selector_fn     node_selector = should_do_random_walk ? fs_entity_list_random_node : fs_entity_list_next_node;
      
      //
      // If the entity is a directory and was specified with a trailing slash, then
      // append that pathname to the base_url to get our real base URL:
      //
      if ( fslist->root_entity->kind == fs_entity_kind_directory ) {
        const char    *last_char = argv[optind] + strlen(argv[optind]) - 1;
        
        if ( *last_char == '/' ) {
          const char  *slash = last_char - 1;
          
          while ( slash >= argv[optind] && (*slash != '/') ) slash--;
          if ( slash < argv[optind] ) {
            real_base_url = strmcat(base_url, "/", argv[optind], NULL);
          } else {
            real_base_url = strmcat(base_url, slash, NULL);
          }
          if ( is_verbose ) {
            printf("Using modified base URL of %s\n", real_base_url);
          }
        }
      }
      
      //
      // Disable deletion?
      //
      fs_entity_list_set_state_is_enabled(fslist, fs_entity_state_delete, should_delete);
      fs_entity_list_set_state_is_enabled(fslist, fs_entity_state_delete_sub, should_delete);
      
      //
      // Enable ranged ops?
      //
      fs_entity_list_set_state_is_enabled(fslist, fs_entity_state_download_range, should_do_ranged_ops);
      
      //
      // Disable option method?
      //
      fs_entity_list_set_state_is_enabled(fslist, fs_entity_state_options, should_do_options);
      
      
      if ( should_show_file_list ) fs_entity_list_print(print_format | print_charset, fslist);
      
      if ( is_verbose ) {
        printf("\nCommencing %u iteration%s...\n", generations, (generations == 1) ? "" : "s");
      }
      
      while ( (e = node_selector(fslist, generations)) ) {
        if ( is_verbose && (fslist->generation == current_generation) ) {
          printf("Generation %u completed\n", current_generation++);
        }
        if ( ! is_dry_run ) {
          const char  *url = fs_entity_list_url_for_entity(fslist, real_base_url, e);
          
          if ( url ) {
            bool      ok = false;
            long      http_status = -1L;
            
            switch ( e->kind ) {
              
              case fs_entity_kind_directory: {
                switch ( e->state ) {
                  case fs_entity_state_upload: {
                    ok = http_ops_mkdir(http_ops, url, e->http_stats[http_ops_method_mkcol], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                          if ( http_status == 405 ) {
                            // Directory already exists, that's okay:
                            break;
                          }
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_options: {
                    ok = http_ops_options(http_ops, url, e->http_stats[http_ops_method_options], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_getinfo: {
                    ok = http_ops_getinfo(http_ops, url, e->http_stats[http_ops_method_propfind], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_download: {
                    ok = http_ops_download(http_ops, url, NULL, e->http_stats[http_ops_method_get], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_delete: {
                    ok = http_ops_delete(http_ops, url, e->http_stats[http_ops_method_delete], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_download_range:
                  case fs_entity_state_download_sub:
                  case fs_entity_state_upload_sub:
                  case fs_entity_state_delete_sub:
                  case fs_entity_state_max:
                    fprintf(stderr, "CATASTOPHIC ERROR:  directory state flow should not reach this state!!\n");
                    exit(EINVAL);
                    break;
                }
                break;
              }
              
              case fs_entity_kind_file: {
                switch ( e->state ) {
                  case fs_entity_state_upload: {
                    ok = http_ops_upload(http_ops, e->path, url, e->http_stats[http_ops_method_put], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_options: {
                    ok = http_ops_options(http_ops, url, e->http_stats[http_ops_method_options], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_getinfo: {
                    ok = http_ops_getinfo(http_ops, url, e->http_stats[http_ops_method_propfind], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_download: {
                    ok = http_ops_download(http_ops, url, NULL, e->http_stats[http_ops_method_get], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_download_range: {
                    ok = http_ops_download_range(http_ops, url, NULL, e->http_stats[http_ops_method_get], &http_status, (long int)e->size);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_delete: {
                    ok = http_ops_delete(http_ops, url, e->http_stats[http_ops_method_delete], &http_status);
                    if ( ok ) {
                      switch ( http_status / 100 ) {
                      
                        case 4:
                        case 5:
                          http_error_exit(http_status);
                          ok = false;
                          break;
                          
                      }
                    }
                    break;
                  }
                  
                  case fs_entity_state_upload_sub:
                  case fs_entity_state_download_sub:
                  case fs_entity_state_delete_sub:
                  case fs_entity_state_max:
                    fprintf(stderr, "CATASTOPHIC ERROR:  file state flow should not reach this state!!\n");
                    exit(EINVAL);
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
            if ( ok ) fs_entity_list_advance_entity_state(fslist, e);
          } else {
            fprintf(stderr, "CATASTROPHIC ERROR:  unable to generate URL for %s (errno = %d)\n", e->path, errno);
            exit(errno);
          }
        } else {
          if ( print_format ) fs_entity_print(print_format | print_charset, e);
          fs_entity_list_advance_entity_state(fslist, e);
        }
      }
      if ( is_verbose ) {
        printf("Generation %u completed\n", current_generation);
      }
      if ( ! is_dry_run && should_show_timings ) {
				if ( ! timing_output ) {
					printf("\nTiming information:\n\n");
        	fs_entity_list_stats_print(stats_format, http_stats_print_flags_none, fslist);
        } else {
        	FILE			*timing_fptr = fopen(timing_output, "w");
        	
        	if ( timing_fptr ) {
        		fs_entity_list_stats_fprint(timing_fptr, stats_format, http_stats_print_flags_none, fslist);
        		fclose(timing_fptr);
        	} else {
        		fprintf(stderr, "ERROR:  unable to open timing file for writing: %s\n", timing_output);
        		rc = errno;
        	}
        }
      }
      fs_entity_list_destroy(fslist);
      if ( real_base_url != base_url ) free((void*)real_base_url);
    }
    optind++;
  }
  return rc;
}
