//
// http_stats.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "http_stats.h"

//

static inline http_stats_bystatus
http_stats_bystatus_from_http_status(
  long        http_status
)
{
  if ( http_status >= 200 && http_status < 600 ) return http_stats_bystatus_2XX + ((http_status / 100) - 2);
  return http_stats_bystatus_max;
}

//

typedef double http_stats_record[http_stats_field_max];

typedef struct _http_stats {
  unsigned int        count[http_stats_bystatus_max];
  http_stats_record   min[http_stats_bystatus_max];
  http_stats_record   max[http_stats_bystatus_max];
  http_stats_record   m_i[http_stats_bystatus_max];
  http_stats_record   s_i[http_stats_bystatus_max];
} http_stats;

//

http_stats_ref
http_stats_create()
{
  http_stats  *new_stats = malloc(sizeof(http_stats));
  
  if ( new_stats ) http_stats_reset(new_stats);
  return new_stats;
}

//

void
http_stats_destroy(
  http_stats_ref  the_stats
)
{
  free((void*)the_stats);
}

//

bool
http_stats_get(
  http_stats_ref      the_stats,
  http_stats_bystatus bystatus,
  http_stats_field    field,
  http_stats_data     *out_data
)
{
  // Validate indices:
  if ( bystatus < http_stats_bystatus_all || bystatus >= http_stats_bystatus_max ) return false;
  if ( field < http_stats_field_dns || field >= http_stats_field_max ) return false;
  
  if ( the_stats->count[bystatus] > 0 ) {
    out_data->count = the_stats->count[bystatus];
    out_data->min = the_stats->min[bystatus][field];
    out_data->max = the_stats->max[bystatus][field];
    out_data->average = the_stats->m_i[bystatus][field];
    out_data->variance = the_stats->s_i[bystatus][field] / (out_data->count - 1);
    out_data->stddev = sqrt(out_data->variance);
  } else {
    // No stats, all zero:
    memset(out_data, 0, sizeof(http_stats_data));
  }
  return true;
}

//

void
http_stats_reset(
  http_stats_ref  the_stats
)
{
  http_stats_bystatus   i_s;
  http_stats_field      i_f;
  
  memset(the_stats, 0, sizeof(http_stats));
  
  // Set all min's to an absurdly large value:
  for ( i_s = 0; i_s < http_stats_bystatus_max; i_s++ )
    for ( i_f = 0; i_f < http_stats_field_max; i_f++ )
      the_stats->min[i_s][i_f] = DBL_MAX;
}

//

bool
http_stats_update(
  http_stats_ref    the_stats,
  CURL              *curl_request
)
{
  long                  http_status = -1;
  http_stats_bystatus   i_s;
  http_stats_field      i_f;
  http_stats_record     timing;
  
  // HTTP response code?
  curl_easy_getinfo(curl_request, CURLINFO_RESPONSE_CODE, &http_status);
  i_s = http_stats_bystatus_from_http_status(http_status);
  if ( i_s == http_stats_bystatus_max ) return false;
  
  // Retrieve timing values:
  curl_easy_getinfo(curl_request, CURLINFO_NAMELOOKUP_TIME, &timing[http_stats_field_dns]);
  curl_easy_getinfo(curl_request, CURLINFO_CONNECT_TIME, &timing[http_stats_field_connect]);
  curl_easy_getinfo(curl_request, CURLINFO_APPCONNECT_TIME, &timing[http_stats_field_sslconnect]);
  curl_easy_getinfo(curl_request, CURLINFO_PRETRANSFER_TIME, &timing[http_stats_field_pretransfer]);
  curl_easy_getinfo(curl_request, CURLINFO_STARTTRANSFER_TIME, &timing[http_stats_field_response]);
  curl_easy_getinfo(curl_request, CURLINFO_TOTAL_TIME, &timing[http_stats_field_total]);
  curl_easy_getinfo(curl_request, CURLINFO_SIZE_DOWNLOAD, &timing[http_stats_field_content_bytes]);
  
  //
  // Update the min/max/avg fields:
  //
  for ( i_f = http_stats_field_dns; i_f < http_stats_field_max; i_f++ ) {
    // Convert all times from microseconds to milliseconds:
    if ( i_f <= http_stats_field_total ) timing[i_f] *= 1000;
    
    // All responses:
    if ( timing[i_f] < the_stats->min[http_stats_bystatus_all][i_f] ) the_stats->min[http_stats_bystatus_all][i_f] = timing[i_f];
    if ( timing[i_f] > the_stats->max[http_stats_bystatus_all][i_f] ) the_stats->max[http_stats_bystatus_all][i_f] = timing[i_f];
    the_stats->count[http_stats_bystatus_all]++;
    //
    // Update the running variance accumulators:
    //   ( http://www.johndcook.com/blog/standard_deviation/ )
    //
    if ( the_stats->count[http_stats_bystatus_all] == 1 ) {
      the_stats->m_i[http_stats_bystatus_all][i_f] = timing[i_f];
    } else {
      double    m_prev = the_stats->m_i[http_stats_bystatus_all][i_f];
    
      the_stats->m_i[http_stats_bystatus_all][i_f] += (timing[i_f] - m_prev) / (double)the_stats->count[http_stats_bystatus_all];
      the_stats->s_i[http_stats_bystatus_all][i_f] += (timing[i_f] - m_prev) * (timing[i_f] - the_stats->m_i[http_stats_bystatus_all][i_f] );
    }
    
    // Specific http status:
    if ( timing[i_f] < the_stats->min[i_s][i_f] ) the_stats->min[i_s][i_f] = timing[i_f];
    if ( timing[i_f] > the_stats->max[i_s][i_f] ) the_stats->max[i_s][i_f] = timing[i_f];
    the_stats->count[i_s]++;
    //
    // Update the running variance accumulators:
    //   ( http://www.johndcook.com/blog/standard_deviation/ )
    //
    if ( the_stats->count[i_s] == 1 ) {
      the_stats->m_i[i_s][i_f] = timing[i_f];
    } else {
      double    m_prev = the_stats->m_i[i_s][i_f];
    
      the_stats->m_i[i_s][i_f] += (timing[i_f] - m_prev) / (double)the_stats->count[i_s];
      the_stats->s_i[i_s][i_f] += (timing[i_f] - m_prev) * (timing[i_f] - the_stats->m_i[i_s][i_f] );
    }
  }
  return true;
}

//

void
http_stats_print(
  http_stats_ref  the_stats,
  bool            should_show_all_bystatus
)
{
  http_stats_fprint(stdout, the_stats, should_show_all_bystatus);
}

//

const char      *http_stats_field_labels[] = { "dns lookup/ms", "tcp connect/ms", "ssl handshake/ms", "request sent/ms", "response start/ms", "total time/ms", "content/bytes" };

const char      *http_stats_bystatus_labels[] = { "All requests", "2XX", "3XX", "4XX", "5XX" };

void
http_stats_fprint(
  FILE            *fptr,
  http_stats_ref  the_stats,
  bool            should_show_all_bystatus
)
{
  http_stats_bystatus   i_s;
  http_stats_field      i_f;
  int                   how_many = 0;
  
  if ( ! should_show_all_bystatus ) {
    // Check to be sure we have anything to show:
    for ( i_s = http_stats_bystatus_2XX; i_s < http_stats_bystatus_max; i_s++ ) {
      if ( the_stats->count[i_s] > 0 ) how_many++;
    }
  } else {
    how_many = http_stats_bystatus_max;
  }
  if ( how_many > 0 ) {
    fprintf(fptr,
        "~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~ ~~~~~~~~ ~~~~~~~~ ~~~~~~~~~~ ~~~~~~~~~~\n"
        "%-24s %8s %8s %8s %10s %10s\n"
        "~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~ ~~~~~~~~ ~~~~~~~~ ~~~~~~~~~~ ~~~~~~~~~~\n",
        "data point", "#req", "min", "max", "avg", "std dev"
      );
    for ( i_s = (how_many == 1) ? http_stats_bystatus_2XX : http_stats_bystatus_all; i_s < http_stats_bystatus_max; i_s++ ) {
      if ( the_stats->count[i_s] > 0 ) {
        fprintf(fptr, "%-24s %8s %8s %8s %10s %10s\n", http_stats_bystatus_labels[i_s], "", "min", "max", "avg", "std dev");
        for ( i_f = http_stats_field_dns; i_f < http_stats_field_max; i_f++ ) {
          if ( the_stats->count[i_s] > 1 ) {
            fprintf(fptr,
                "+ %-22s %8u %8.3lg %8.3lg %10.3lg %10.3lg\n",
                http_stats_field_labels[i_f],
                the_stats->count[i_s],
                the_stats->min[i_s][i_f],
                the_stats->max[i_s][i_f],
                the_stats->m_i[i_s][i_f],
                sqrt(the_stats->s_i[i_s][i_f] / (the_stats->count[i_s] - 1))
              );
          } else {
            fprintf(fptr,
                "+ %-22s %8u %8.3lg %8.3lg %10.3lg %10s\n",
                http_stats_field_labels[i_f],
                the_stats->count[i_s],
                the_stats->min[i_s][i_f],
                the_stats->max[i_s][i_f],
                the_stats->m_i[i_s][i_f],
                "n/a"
              );
          }
        }
        fprintf(fptr, "~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~ ~~~~~~~~ ~~~~~~~~ ~~~~~~~~~~ ~~~~~~~~~~\n");
      }
    }
  }
}
