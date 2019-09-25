// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#pragma once

#include "bb_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sb_s sb_t;
typedef struct sbs_s sbs_t;
typedef struct sdictEntry_s sdictEntry_t;
typedef struct sdict_s sdict_t;
typedef struct buildCommand_s buildCommand_t;
typedef struct buildCommands_s buildCommands_t;
typedef struct sourceTimestampEntry sourceTimestampEntry;
typedef struct sourceTimestampChain sourceTimestampChain;
typedef struct sourceTimestampTable sourceTimestampTable;
typedef struct buildDependencyEntry buildDependencyEntry;
typedef struct buildDependencyChain buildDependencyChain;
typedef struct buildDependencyTable buildDependencyTable;

void sb_reset_from_loc(const char *file, int line, sb_t *val);
void sbs_reset_from_loc(const char *file, int line, sbs_t *val);
void sdictEntry_reset(sdictEntry_t *val);
void sdict_reset(sdict_t *val);
void buildCommand_reset_from_loc(const char *file, int line, buildCommand_t *val);
void buildCommands_reset_from_loc(const char *file, int line, buildCommands_t *val);
void sourceTimestampEntry_reset_from_loc(const char *file, int line, sourceTimestampEntry *val);
void sourceTimestampChain_reset_from_loc(const char *file, int line, sourceTimestampChain *val);
void sourceTimestampTable_reset_from_loc(const char *file, int line, sourceTimestampTable *val);
void buildDependencyEntry_reset_from_loc(const char *file, int line, buildDependencyEntry *val);
void buildDependencyChain_reset_from_loc(const char *file, int line, buildDependencyChain *val);
void buildDependencyTable_reset_from_loc(const char *file, int line, buildDependencyTable *val);

sb_t sb_clone_from_loc(const char *file, int line, const sb_t *src);
sbs_t sbs_clone_from_loc(const char *file, int line, const sbs_t *src);
sdictEntry_t sdictEntry_clone(const sdictEntry_t *src);
sdict_t sdict_clone(const sdict_t *src);
buildCommand_t buildCommand_clone_from_loc(const char *file, int line, const buildCommand_t *src);
buildCommands_t buildCommands_clone_from_loc(const char *file, int line, const buildCommands_t *src);
sourceTimestampEntry sourceTimestampEntry_clone_from_loc(const char *file, int line, const sourceTimestampEntry *src);
sourceTimestampChain sourceTimestampChain_clone_from_loc(const char *file, int line, const sourceTimestampChain *src);
sourceTimestampTable sourceTimestampTable_clone_from_loc(const char *file, int line, const sourceTimestampTable *src);
buildDependencyEntry buildDependencyEntry_clone_from_loc(const char *file, int line, const buildDependencyEntry *src);
buildDependencyChain buildDependencyChain_clone_from_loc(const char *file, int line, const buildDependencyChain *src);
buildDependencyTable buildDependencyTable_clone_from_loc(const char *file, int line, const buildDependencyTable *src);

#if defined(__cplusplus)
} // extern "C"
#endif

#if !defined(sb_reset)
#define sb_reset(var) sb_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sbs_reset)
#define sbs_reset(var) sbs_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildCommand_reset)
#define buildCommand_reset(var) buildCommand_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildCommands_reset)
#define buildCommands_reset(var) buildCommands_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sourceTimestampEntry_reset)
#define sourceTimestampEntry_reset(var) sourceTimestampEntry_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sourceTimestampChain_reset)
#define sourceTimestampChain_reset(var) sourceTimestampChain_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sourceTimestampTable_reset)
#define sourceTimestampTable_reset(var) sourceTimestampTable_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildDependencyEntry_reset)
#define buildDependencyEntry_reset(var) buildDependencyEntry_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildDependencyChain_reset)
#define buildDependencyChain_reset(var) buildDependencyChain_reset_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildDependencyTable_reset)
#define buildDependencyTable_reset(var) buildDependencyTable_reset_from_loc(__FILE__, __LINE__, var);
#endif

#if !defined(sb_clone)
#define sb_clone(var) sb_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sbs_clone)
#define sbs_clone(var) sbs_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildCommand_clone)
#define buildCommand_clone(var) buildCommand_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildCommands_clone)
#define buildCommands_clone(var) buildCommands_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sourceTimestampEntry_clone)
#define sourceTimestampEntry_clone(var) sourceTimestampEntry_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sourceTimestampChain_clone)
#define sourceTimestampChain_clone(var) sourceTimestampChain_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(sourceTimestampTable_clone)
#define sourceTimestampTable_clone(var) sourceTimestampTable_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildDependencyEntry_clone)
#define buildDependencyEntry_clone(var) buildDependencyEntry_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildDependencyChain_clone)
#define buildDependencyChain_clone(var) buildDependencyChain_clone_from_loc(__FILE__, __LINE__, var);
#endif
#if !defined(buildDependencyTable_clone)
#define buildDependencyTable_clone(var) buildDependencyTable_clone_from_loc(__FILE__, __LINE__, var);
#endif

u64 sourceTimestampEntry_hash(const sourceTimestampEntry *entry);
int sourceTimestampEntry_compare(const sourceTimestampEntry *a, const sourceTimestampEntry *b);
sourceTimestampEntry *sourceTimestampTable_find(sourceTimestampTable *table, const char *name);
sourceTimestampEntry *sourceTimestampTable_insert(sourceTimestampTable *table, const sourceTimestampEntry *entry);
sourceTimestampEntry *sourceTimestampTable_insertmulti(sourceTimestampTable *table, const sourceTimestampEntry *entry);
sourceTimestampEntry *sourceTimestampTable_find_internal(sourceTimestampTable *table, const sourceTimestampEntry *entry, u64 hashValue);
sourceTimestampEntry *sourceTimestampTable_insert_internal(sourceTimestampTable *table, const sourceTimestampEntry *entry, u64 hashValue);
sourceTimestampEntry *sourceTimestampTable_insertmulti_internal(sourceTimestampTable *table, const sourceTimestampEntry *entry, u64 hashValue);
void sourceTimestampTable_remove(sourceTimestampTable *table, const char *name);
void sourceTimestampTable_removemulti(sourceTimestampTable *table, const char *name);

u64 buildDependencyEntry_hash(const buildDependencyEntry *entry);
int buildDependencyEntry_compare(const buildDependencyEntry *a, const buildDependencyEntry *b);
buildDependencyEntry *buildDependencyTable_find(buildDependencyTable *table, const char *name);
buildDependencyEntry *buildDependencyTable_insert(buildDependencyTable *table, const buildDependencyEntry *entry);
buildDependencyEntry *buildDependencyTable_insertmulti(buildDependencyTable *table, const buildDependencyEntry *entry);
buildDependencyEntry *buildDependencyTable_find_internal(buildDependencyTable *table, const buildDependencyEntry *entry, u64 hashValue);
buildDependencyEntry *buildDependencyTable_insert_internal(buildDependencyTable *table, const buildDependencyEntry *entry, u64 hashValue);
buildDependencyEntry *buildDependencyTable_insertmulti_internal(buildDependencyTable *table, const buildDependencyEntry *entry, u64 hashValue);
void buildDependencyTable_remove(buildDependencyTable *table, const char *name);
void buildDependencyTable_removemulti(buildDependencyTable *table, const char *name);