//
// http_stats.h
//

#ifndef __HTTP_STATS_H__
#define __HTTP_STATS_H__

#include "config.h"

#include <curl/curl.h>

typedef enum {
  http_stats_field_dns = 0,
  http_stats_field_connect,
  http_stats_field_sslconnect,
  http_stats_field_pretransfer,
  http_stats_field_response,
  http_stats_field_total,
  http_stats_field_content_bytes,
  //
  http_stats_field_max
} http_stats_field;

typedef double http_stats_record[http_stats_field_max];

typedef enum {
  http_stats_bystatus_all = 0,
  http_stats_bystatus_2XX,
  http_stats_bystatus_3XX,
  http_stats_bystatus_4XX,
  http_stats_bystatus_5XX,
  //
  http_stats_bystatus_max
} http_stats_bystatus;

typedef struct {
  unsigned int    count;
  double          min, max;
  double          average;
  double          variance;
  double          stddev;
} http_stats_data;

typedef struct _http_stats * http_stats_ref;

http_stats_ref http_stats_create(void);
void http_stats_destroy(http_stats_ref the_stats);

bool http_stats_get(http_stats_ref the_stats, http_stats_bystatus bystatus, http_stats_field field, http_stats_data *out_data);

void http_stats_reset(http_stats_ref the_stats);

bool http_stats_update(http_stats_ref the_stats, CURL *curl_request);
bool http_stats_update_and_copy(http_stats_ref the_stats, CURL *curl_request, http_stats_record *copy);

bool http_stats_is_empty(http_stats_ref the_stats);

typedef enum {
	http_stats_print_flags_none = 0,
	http_stats_print_flags_show_all = 1 << 0,
	http_stats_print_flags_no_newline = 1 << 1,
	http_stats_print_flags_no_header = 1 << 2,
	http_stats_print_flags_header_only = 1 << 3
} http_stats_print_flags;

typedef enum {
	http_stats_format_table = 0,
	http_stats_format_csv,
	http_stats_format_tsv,
	//
	http_stats_format_max
} http_stats_format;

void http_stats_print(http_stats_format format, http_stats_print_flags flags, http_stats_ref the_stats);
void http_stats_fprint(FILE *fptr, http_stats_format format, http_stats_print_flags flags, http_stats_ref the_stats);

#endif /* __HTTP_STATS_H__ */
