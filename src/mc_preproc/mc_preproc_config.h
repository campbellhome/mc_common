// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct preprocInputDir {
	sb_t dir;
	sb_t base;
} preprocInputDir;

typedef struct preprocInputDirs {
	u32 count;
	u32 allocated;
	preprocInputDir* data;
} preprocInputDirs;

typedef struct preprocInputConfig {
	b32 checkFonts;
	preprocInputDirs sourceDirs;
	preprocInputDirs includeDirs;
} preprocInputConfig;

typedef struct preprocOutputConfig {
	sb_t prefix;
	sb_t sourceDir;
	sb_t includeDir;
	sb_t baseDir;
} preprocOutputConfig;

typedef struct preprocConfig {
	b32 bb;
	preprocInputConfig input;
	preprocOutputConfig output;
} preprocConfig;

void reset_preprocConfig(preprocConfig *obj);
preprocConfig read_preprocConfig(const char *path);

#if defined(__cplusplus)
} // extern "C"
#endif
