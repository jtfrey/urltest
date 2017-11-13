//
// http_ops.h
//

#ifndef __HTTP_OPS_H__
#define __HTTP_OPS_H__

#include "http_stats.h"

typedef enum {
	http_ops_method_get			= 0,
	http_ops_method_mkcol,
	http_ops_method_put,
	http_ops_method_delete,
	http_ops_method_propfind,
  http_ops_method_options,
	//
	http_ops_method_max
} http_ops_method;

const char* http_ops_method_get_string(http_ops_method the_method);

typedef struct _http_ops * http_ops_ref;

http_ops_ref http_ops_create(void);
void http_ops_destroy(http_ops_ref ops);

const char* http_ops_get_error_buffer(http_ops_ref ops);

bool http_ops_get_is_verbose(http_ops_ref ops);
void http_ops_set_is_verbose(http_ops_ref ops, bool is_verbose);

bool http_ops_get_ssl_verify_peer(http_ops_ref ops);
void http_ops_set_ssl_verify_peer(http_ops_ref ops, bool should_verify_peer);

const char* http_ops_get_username(http_ops_ref ops);
bool http_ops_set_username(http_ops_ref ops, const char *username);

const char* http_ops_get_password(http_ops_ref ops);
bool http_ops_set_password(http_ops_ref ops, const char *password);

bool http_ops_add_host_mapping(http_ops_ref ops, const char *hostname, short port, const char *ipaddress);
bool http_ops_add_host_mapping_string(http_ops_ref ops, const char *host_map_string);

bool http_ops_mkdir(http_ops_ref ops, const char *url, http_stats_ref stats, long *http_status);
bool http_ops_upload(http_ops_ref ops, const char *path, const char *url, http_stats_ref stats, long *http_status);
bool http_ops_download(http_ops_ref ops, const char *url, const char *path, http_stats_ref stats, long *http_status);
bool http_ops_download_range(http_ops_ref ops, const char *url, const char *path, http_stats_ref stats, long *http_status, long expected_length);
bool http_ops_delete(http_ops_ref ops, const char *url, http_stats_ref stats, long *http_status);
bool http_ops_getinfo(http_ops_ref ops, const char *url, http_stats_ref stats, long *http_status);
bool http_ops_options(http_ops_ref ops, const char *url, http_stats_ref stats, long *http_status, bool *has_propfind, bool *has_delete);

#endif /* __HTTP_OPS_H__ */
