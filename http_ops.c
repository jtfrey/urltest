//
// http_ops.c
//

#include "http_ops.h"
#include "util_fns.h"

#include <curl/curl.h>

//

const char*
http_ops_method_get_string(
	http_ops_method		the_method
)
{
	static const char* __http_ops_method_strings[] = {
																"GET     ",
																"MKCOL   ",
																"PUT     ",
																"DELETE  ",
																"PROPFIND"
															};
	if ( the_method >= http_ops_method_get && the_method < http_ops_method_max ) return __http_ops_method_strings[the_method];
	return NULL;
}

//

size_t
__http_ops_null_write(
  char      *ptr,
  size_t    size,
  size_t    nmemb,
  void      *userdata
)
{
  return (size * nmemb);
}

//

size_t
__http_ops_null_read(
  void      *ptr,
  size_t    size,
  size_t    nmemb,
  void      *stream
)
{
  return 0;
}

//

static const char   *__http_ops_propfind_read_data =
        "<?xml version=\"1.0\"?>"
        "<a:propfind xmlns:a=\"DAV:\">"
        "<a:allprop/>"
        "</a:propfind>";
static size_t       __http_ops_propfind_read_data_offset = 0;
static size_t       __http_ops_propfind_read_data_length = -1;

void
__http_ops_propfind_read_data_reset(void)
{
  __http_ops_propfind_read_data_offset = 0;
  __http_ops_propfind_read_data_length = strlen(__http_ops_propfind_read_data);
}

//

size_t
__http_ops_propfind_read(
  void      *ptr,
  size_t    size,
  size_t    nmemb,
  void      *stream
)
{
  size_t    total_size = size * nmemb;
  size_t    remnant_size = __http_ops_propfind_read_data_length - __http_ops_propfind_read_data_offset;
  
  if ( remnant_size == 0 ) return 0;

  if ( total_size < remnant_size ) {
    memcpy(
        ptr,
        __http_ops_propfind_read_data + __http_ops_propfind_read_data_offset,
        total_size
      );
    __http_ops_propfind_read_data_offset += total_size;
    return total_size;
  }
  
  memcpy(
      ptr,
      __http_ops_propfind_read_data + __http_ops_propfind_read_data_offset,
      remnant_size
    );
  return remnant_size;

}

//

typedef enum {
  http_ops_curl_request_get   = 0,
  http_ops_curl_request_put,
  http_ops_curl_request_delete,
  http_ops_curl_request_mkcol,
  http_ops_curl_request_propfind,
  //
  http_ops_curl_request_max
} http_ops_curl_request;

//

typedef struct _http_ops {
  struct curl_slist   *resolve_list, *propfind_headers;
  const char          *username, *password;
  bool                should_verify_peer;
  char                curl_error_buffer[CURL_ERROR_SIZE];
} http_ops;

//

CURL*
__http_ops_get_curl_request(
  http_ops                *ops,
  http_ops_curl_request   request
)
{
  CURL        *new_request = curl_easy_init();
  
  if ( new_request ) {
    curl_easy_setopt(new_request, CURLOPT_ERRORBUFFER, &ops->curl_error_buffer[0]);
    if ( ops->resolve_list ) curl_easy_setopt(new_request, CURLOPT_RESOLVE, ops->resolve_list);
    if ( ops->username ) {
      curl_easy_setopt(new_request, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
      curl_easy_setopt(new_request, CURLOPT_USERNAME, ops->username);
      if ( ops->password ) {
        curl_easy_setopt(new_request, CURLOPT_PASSWORD, ops->password);
      }
      curl_easy_setopt(new_request, CURLOPT_SSL_VERIFYPEER, (ops->should_verify_peer ? 1L : 0L));
    }
    
    //
    // Per-method config:
    //
    switch ( request ) {
    
      case http_ops_curl_request_get:
        curl_easy_setopt(new_request, CURLOPT_READFUNCTION, __http_ops_null_read);
        curl_easy_setopt(new_request, CURLOPT_READDATA, NULL);
        break;
    
      case http_ops_curl_request_put:
        curl_easy_setopt(new_request, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(new_request, CURLOPT_READFUNCTION, __http_ops_null_read);
        curl_easy_setopt(new_request, CURLOPT_READDATA, NULL);
        curl_easy_setopt(new_request, CURLOPT_WRITEFUNCTION, __http_ops_null_write);
        curl_easy_setopt(new_request, CURLOPT_WRITEDATA, NULL);
        break;
        
      case http_ops_curl_request_delete:
        curl_easy_setopt(new_request, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(new_request, CURLOPT_READFUNCTION, __http_ops_null_read);
        curl_easy_setopt(new_request, CURLOPT_READDATA, NULL);
        curl_easy_setopt(new_request, CURLOPT_WRITEFUNCTION, __http_ops_null_write);
        curl_easy_setopt(new_request, CURLOPT_WRITEDATA, NULL);
        break;
    
      case http_ops_curl_request_mkcol:
        curl_easy_setopt(new_request, CURLOPT_CUSTOMREQUEST, "MKCOL");
        curl_easy_setopt(new_request, CURLOPT_READFUNCTION, __http_ops_null_read);
        curl_easy_setopt(new_request, CURLOPT_READDATA, NULL);
        curl_easy_setopt(new_request, CURLOPT_WRITEFUNCTION, __http_ops_null_write);
        curl_easy_setopt(new_request, CURLOPT_WRITEDATA, NULL);
        break;
      
      case http_ops_curl_request_propfind:
        curl_easy_setopt(new_request, CURLOPT_CUSTOMREQUEST, "PROPFIND");
        ops->propfind_headers = curl_slist_append(NULL, "Content-type: text/xml");
        ops->propfind_headers = curl_slist_append(ops->propfind_headers, "Depth: 0");
        ops->propfind_headers = curl_slist_append(ops->propfind_headers, "Translate: f");
        curl_easy_setopt(new_request, CURLOPT_HTTPHEADER, ops->propfind_headers);
        curl_easy_setopt(new_request, CURLOPT_READFUNCTION, __http_ops_propfind_read);
        curl_easy_setopt(new_request, CURLOPT_READDATA, NULL);
        curl_easy_setopt(new_request, CURLOPT_WRITEFUNCTION, __http_ops_null_write);
        curl_easy_setopt(new_request, CURLOPT_WRITEDATA, NULL);
        break;
      
      case http_ops_curl_request_max:
        break;
    
    }
  }
  return new_request;
}

//

http_ops_ref
http_ops_create()
{
  http_ops    *new_ops;
  
  new_ops = malloc(sizeof(http_ops));
  if ( new_ops ) {
    memset(new_ops, 0, sizeof(http_ops));
  }
  return new_ops;
}

//

void
http_ops_destroy(
  http_ops_ref  ops
)
{
  http_ops_curl_request   i_r;
  
  if ( ops->resolve_list ) curl_slist_free_all(ops->resolve_list);
  if ( ops->propfind_headers ) curl_slist_free_all(ops->propfind_headers);
  if ( ops->username ) free((void*)ops->username);
  if ( ops->password ) free((void*)ops->password);
  free((void*)ops);
}

//

const char*
http_ops_get_error_buffer(
  http_ops_ref  ops
)
{
  return (const char*)&ops->curl_error_buffer[0];
}

//

bool
http_ops_get_ssl_verify_peer(
  http_ops_ref  ops
)
{
  return ops->should_verify_peer;
}

//

void
http_ops_set_ssl_verify_peer(
  http_ops_ref  ops,
  bool          should_verify_peer
)
{
  ops->should_verify_peer = should_verify_peer;
}

//

const char*
http_ops_get_username(
  http_ops_ref  ops
)
{
  return ops->username;
}

//

bool
http_ops_set_username(
  http_ops_ref  ops,
  const char    *username
)
{
  if ( ops->username ) {
    free((void*)ops->username);
    ops->username = NULL;
  }
  if ( username && ! (ops->username = strdup(username)) ) return false;
  return true;
}

//

const char*
http_ops_get_password(
  http_ops_ref  ops
)
{
  return ops->password;
}

//

bool
http_ops_set_password(
  http_ops_ref  ops,
  const char    *password
)
{
  if ( ops->password ) {
    free((void*)ops->password);
    ops->password = NULL;
  }
  if ( password && ! (ops->password = strdup(password)) ) return false;
  return true;
}

//

bool
http_ops_add_host_mapping(
  http_ops_ref  ops,
  const char    *hostname,
  short         port,
  const char    *ipaddress
)
{
  char        *mapping = NULL;
  
  if ( asprintf(&mapping, "%s:%hd:%s", hostname, port, ipaddress) >= 0 ) {
    if ( mapping ) {
      http_ops_curl_request   i_r;
      
      ops->resolve_list = curl_slist_append(ops->resolve_list, mapping);
      free(mapping);
      return true;
    }
  }
  return false;
}

//

bool
http_ops_add_host_mapping_string(
  http_ops_ref  ops,
  const char    *host_map_string
)
{
  if ( host_map_string ) {
    http_ops_curl_request   i_r;
    
    ops->resolve_list = curl_slist_append(ops->resolve_list, host_map_string);
    return true;
  }
  return false;
}

//

bool
http_ops_mkdir(
  http_ops_ref    ops,
  const char      *url,
  http_stats_ref  stats,
  long            *http_status
)
{
  CURL            *curl_request = __http_ops_get_curl_request(ops, http_ops_curl_request_mkcol);
  bool            rc = false;
  
  if ( curl_request ) {
    CURLcode      ccode;
    
    curl_easy_setopt(curl_request, CURLOPT_URL, url);
    ccode = curl_easy_perform(curl_request);
    if ( ccode == CURLE_OK ) {
      curl_easy_getinfo(curl_request, CURLINFO_RESPONSE_CODE, http_status);
      http_stats_update(stats, curl_request);
      rc = true;
    }
    curl_easy_cleanup(curl_request);
  }
  return rc;
}

//

bool
http_ops_upload(
  http_ops_ref    ops,
  const char      *path,
  const char      *url,
  http_stats_ref  stats,
  long            *http_status
)
{
  CURL            *curl_request = __http_ops_get_curl_request(ops, http_ops_curl_request_put);
  bool            rc = false;
  
  if ( curl_request ) {
    CURLcode      ccode;
    FILE          *in_file = fopen(path, "r");
    
    if ( in_file ) {
      curl_easy_setopt(curl_request, CURLOPT_URL, url);
      curl_easy_setopt(curl_request, CURLOPT_READFUNCTION, NULL);
      curl_easy_setopt(curl_request, CURLOPT_READDATA, in_file);
      ccode = curl_easy_perform(curl_request);
      fclose(in_file);
      if ( ccode == CURLE_OK ) {
        curl_easy_getinfo(curl_request, CURLINFO_RESPONSE_CODE, http_status);
        http_stats_update(stats, curl_request);
        rc = true;
      }
    }
    curl_easy_cleanup(curl_request);
  }
  return rc;
}

//

bool
http_ops_download(
  http_ops_ref    ops,
  const char      *url,
  const char      *path,
  http_stats_ref  stats,
  long            *http_status
)
{
  CURL            *curl_request = __http_ops_get_curl_request(ops, http_ops_curl_request_get);
  bool            rc = false;
  
  if ( curl_request ) {
    CURLcode      ccode = -1;
    
    if ( path && *path ) {
      FILE          *out_file = fopen(path, "w");
      
      if ( out_file ) {
        curl_easy_setopt(curl_request, CURLOPT_URL, url);
        curl_easy_setopt(curl_request, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl_request, CURLOPT_WRITEDATA, out_file);
        ccode = curl_easy_perform(curl_request);
        fclose(out_file);
      }
    } else {
      curl_easy_setopt(curl_request, CURLOPT_URL, url);
      curl_easy_setopt(curl_request, CURLOPT_WRITEFUNCTION, __http_ops_null_write);
      curl_easy_setopt(curl_request, CURLOPT_WRITEDATA, NULL);
      ccode = curl_easy_perform(curl_request);
    }
    if ( ccode == CURLE_OK ) {
      curl_easy_getinfo(curl_request, CURLINFO_RESPONSE_CODE, http_status);
      http_stats_update(stats, curl_request);
      rc = true;
    }
    curl_easy_cleanup(curl_request);
  }
  return rc;
}

//

bool
http_ops_delete(
  http_ops_ref    ops,
  const char      *url,
  http_stats_ref  stats,
  long            *http_status
)
{
  CURL            *curl_request = __http_ops_get_curl_request(ops, http_ops_curl_request_delete);
  bool            rc = false;
  
  if ( curl_request ) {
    CURLcode      ccode;
    
    curl_easy_setopt(curl_request, CURLOPT_URL, url);
    ccode = curl_easy_perform(curl_request);
    if ( ccode == CURLE_OK ) {
      curl_easy_getinfo(curl_request, CURLINFO_RESPONSE_CODE, http_status);
      http_stats_update(stats, curl_request);
      rc = true;
    }
    curl_easy_cleanup(curl_request);
  }
  return rc;
}

//

bool
http_ops_getinfo(
  http_ops_ref    ops,
  const char      *url,
  http_stats_ref  stats,
  long            *http_status
)
{
  CURL            *curl_request = __http_ops_get_curl_request(ops, http_ops_curl_request_propfind);
  bool            rc = false;
  
  if ( curl_request ) {
    CURLcode      ccode;
    
    __http_ops_propfind_read_data_reset();
    curl_easy_setopt(curl_request, CURLOPT_URL, url);
    ccode = curl_easy_perform(curl_request);
    if ( ccode == CURLE_OK ) {
      curl_easy_getinfo(curl_request, CURLINFO_RESPONSE_CODE, http_status);
      http_stats_update(stats, curl_request);
      rc = true;
    }
    curl_easy_cleanup(curl_request);
  }
  return rc;
}
