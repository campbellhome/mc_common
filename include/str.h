// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

u32 strtou32(const char *s);
s32 strtos32(const char *s);
u64 strsimplehash(const char* s);

#if defined(__cplusplus)
}
#endif
