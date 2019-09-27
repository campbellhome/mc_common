// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

const char *path_get_filename(const char *path);
sb_t path_resolve(sb_t src);
void path_resolve_inplace(sb_t *path);
void path_remove_filename(sb_t *path);
void path_add_component(sb_t *path, const char *component);
b32 path_mkdir(const char *path);
b32 path_rmdir(const char *path); // non-recursive, must be empty
char path_get_separator(void);
b32 path_test_resolve(void);

#if defined(__cplusplus)
}
#endif
