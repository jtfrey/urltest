//
// fs_entity.h
//

#ifndef __FS_ENTITY_H__
#define __FS_ENTITY_H__

#include <stdio.h>
#include <stdlib.h>

#include "http_stats.h"

//

typedef enum {
  fs_entity_kind_directory  = 0,
  fs_entity_kind_file,
  //
  fs_entity_kind_max
} fs_entity_kind;

typedef enum {
  fs_entity_state_upload = 0,
  fs_entity_state_upload_sub,
  fs_entity_state_getinfo,
  fs_entity_state_download_sub,
  fs_entity_state_download,
  fs_entity_state_delete_sub,
  fs_entity_state_delete,
  //
  fs_entity_state_max
} fs_entity_state;

typedef struct _fs_entity {
  fs_entity_kind      kind;
  const char          *path;
  const char          *name;
  
  unsigned int        generation;
  size_t              size;
  fs_entity_state     state;
  http_stats_ref      http_stats;
  
  struct _fs_entity   *sibling, *child;
} fs_entity;

typedef struct _fs_entity_list {
  unsigned int        count;
  unsigned int        generation;
  const char          *base_path;
  fs_entity           *root_entity;
} fs_entity_list;

fs_entity_list* fs_entity_list_create_with_path(const char *path);

void fs_entity_list_destroy(fs_entity_list *the_list);

unsigned int fs_entity_list_hits_average(fs_entity_list *the_list);

typedef enum {
  fs_entity_print_format_kind       = 1 << 0,
  fs_entity_print_format_generation = 1 << 1,
  fs_entity_print_format_state      = 1 << 2,
  fs_entity_print_format_name       = 1 << 3,
  fs_entity_print_format_path       = 1 << 4,
  fs_entity_print_format_size       = 1 << 5,
  //
  fs_entity_print_format_summary    = 1 << 16,
  fs_entity_print_format_ascii      = 1 << 17,
  //
  fs_entity_print_format_default    = fs_entity_print_format_kind | fs_entity_print_format_generation | \
                                      fs_entity_print_format_state | fs_entity_print_format_name | \
                                      fs_entity_print_format_size | fs_entity_print_format_summary,
  //
  fs_entity_print_format_short      = fs_entity_print_format_kind | fs_entity_print_format_name
} fs_entity_print_format;

void fs_entity_print(fs_entity_print_format format, fs_entity *entity);
void fs_entity_fprint(FILE *fptr, fs_entity_print_format format, fs_entity *entity);

void fs_entity_list_print(fs_entity_print_format format, fs_entity_list *the_list);
void fs_entity_list_fprint(FILE *fptr, fs_entity_print_format format, fs_entity_list *the_list);

fs_entity* fs_entity_list_random_node(fs_entity_list *the_list, unsigned int max_generation);

void fs_entity_advance_state(fs_entity *root_entity);

const char* fs_entity_list_url_for_entity(fs_entity_list *the_list, const char *base_url, fs_entity *the_entity);

void fs_entity_list_stats_print(fs_entity_print_format format, fs_entity_list *the_list);
void fs_entity_list_stats_fprint(FILE *fptr, fs_entity_print_format format, fs_entity_list *the_list);

#endif /* __FS_ENTITY_H__ */
