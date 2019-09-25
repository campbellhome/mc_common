// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct buildCommands_s buildCommands_t;

AUTOSTRUCT AUTOFROMLOC typedef struct sourceTimestampEntry {
	sb_t key;
	u64 timestamp;
} sourceTimestampEntry;

AUTOSTRUCT AUTOFROMLOC typedef struct sourceTimestampChain {
	u32 count;
	u32 allocated;
	sourceTimestampEntry *data;
} sourceTimestampChain;

AUTOSTRUCT AUTOFROMLOC AUTOSTRINGHASH typedef struct sourceTimestampTable {
	u32 count;
	u32 allocated;
	sourceTimestampChain *data;
} sourceTimestampTable;

AUTOSTRUCT AUTOFROMLOC typedef struct buildDependencyEntry {
	sb_t key;
	sbs_t deps;
} buildDependencyEntry;

AUTOSTRUCT AUTOFROMLOC typedef struct buildDependencyChain {
	u32 count;
	u32 allocated;
	buildDependencyEntry *data;
} buildDependencyChain;

AUTOSTRUCT AUTOFROMLOC AUTOSTRINGHASH typedef struct buildDependencyTable {
	u32 count;
	u32 allocated;
	buildDependencyChain *data;
} buildDependencyTable;

buildDependencyTable buildDependencyTable_init(u32 buckets);
sourceTimestampTable sourceTimestampTable_init(u32 buckets);

void buildDependencyTable_insertFromDir(buildDependencyTable *depTable, sourceTimestampTable *timeTable, sbs_t *sourcePaths, const char *sourceDir, const char *objectDir, b32 bRecursive);
b32 buildDependencyTable_checkDeps(buildDependencyTable *deps, sourceTimestampTable *times, const char *path, b32 bDebug);
u32 buildDependencyTable_queueCommands(buildCommands_t *commands, buildDependencyTable *deps, sourceTimestampTable *times, sbs_t *sourcePaths, const char *objectDir, b32 bDebug, b32 bRebuild, const char *dir, const char *parameterizedCommand);

void buildDependencyTable_dump(buildDependencyTable *table);
void sourceTimestampTable_dump(sourceTimestampTable *table);

#if defined(__cplusplus)
} // extern "C"
#endif
