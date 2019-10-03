// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "mc_build/mc_build_dependencies.h"
#include "bb_array.h"
#include "bb_string.h"
#include "bb_wrap_dirent.h"
#include "file_utils.h"
#include "mc_build/mc_build_commands.h"
#include "mc_build/mc_build_structs_generated.h"
#include "mc_build/mc_build_utils.h"
#include "path_utils.h"
#include "tokenize.h"
#include "va.h"

static u64 FileTimeToMilliseconds(const FILETIME *filetime)
{
	ULARGE_INTEGER uli;
	const u64 epoch = 116444736000000000Ui64;
	uli.LowPart = filetime->dwLowDateTime;
	uli.HighPart = filetime->dwHighDateTime;
	return (uli.QuadPart - epoch) / 10000;
}

static u64 GetFileTimeMilliseconds(const char *path)
{
	FILETIME creation = { BB_EMPTY_INITIALIZER }, access = { BB_EMPTY_INITIALIZER }, write = { BB_EMPTY_INITIALIZER };
	file_getTimestamps(path, &creation, &access, &write);
	u64 creationMillis = FileTimeToMilliseconds(&creation);
	u64 writeMillis = FileTimeToMilliseconds(&write);
	return creationMillis > writeMillis ? creationMillis : writeMillis;
}

static sb_t BuildFilePath(const sb_t dir, const char *filename)
{
	sb_t path = { BB_EMPTY_INITIALIZER };
	sb_va(&path, "%s%c%s", sb_get(&dir), path_get_separator(), filename);
	return path;
}

buildDependencyTable buildDependencyTable_init(u32 buckets)
{
	buildDependencyTable table = { BB_EMPTY_INITIALIZER };
	bba_add(table, buckets);
	return table;
}

sourceTimestampTable sourceTimestampTable_init(u32 buckets)
{
	sourceTimestampTable table = { BB_EMPTY_INITIALIZER };
	bba_add(table, buckets);
	return table;
}

void span_trim_char_start(span_t *span, char c)
{
	if(span->end > span->start) {
		if(*(span->start) == c) {
			++span->start;
		}
	}
}

void span_trim_char_end(span_t *span, char c)
{
	if(span->end > span->start) {
		if(*(span->end - 1) == c) {
			--span->end;
		}
	}
}

void buildDependencyTable_trimToken(span_t *token)
{
	span_trim_char_start(token, ' ');
	span_trim_char_start(token, ' ');
	span_trim_char_end(token, '\n');
	span_trim_char_end(token, '\r');
	span_trim_char_end(token, '\\');
	span_trim_char_end(token, ' ');
	span_trim_char_end(token, ':');
}

void buildDependencyTable_insertFromFile(buildDependencyTable *table, const char *dependenciesPath, const char *sourcePath, b32 bDebug)
{
	if(bDebug) {
		BB_LOG("Dependencies", "DepPath: %s / %s", dependenciesPath, sourcePath);
	}

	fileData_t data = fileData_read(dependenciesPath);
	if(data.buffer) {
		b32 bFirst = true;
		span_t dataCursor = { BB_EMPTY_INITIALIZER };
		dataCursor.start = data.buffer;
		dataCursor.end = dataCursor.start + data.bufferSize;
		buildDependencyEntry entry = { BB_EMPTY_INITIALIZER };

		const char *cursor = dataCursor.start;
		do {
			span_t token = tokenize(&cursor, " ");
			if(!token.end)
				break;

			buildDependencyTable_trimToken(&token);
			if(token.end == token.start)
				continue;

			sb_t depPath = sb_from_span(token);
			path_resolve_inplace(&depPath);

			if(bFirst) {
				bFirst = false;
				entry.key = sb_clone(&depPath);
				path_resolve_inplace(&entry.key);
				if(bDebug) {
					BB_LOG("Dependencies", "Key: %s", sb_get(&entry.key));
				}
				sb_reset(&depPath);
			} else {
				if(bDebug) {
					("Dependencies", "Line: %s", sb_get(&depPath));
				}
				bba_push(entry.deps, depPath);
			}
		} while(1);

		fileData_reset(&data);

		if(entry.key.data) {
#if 0
			buildDependencyEntry objEntry = { BB_EMPTY_INITIALIZER };
			objEntry.key = sb_from_c_string(sourcePath);
			bba_push(objEntry.deps, sb_clone(&entry.key));
			buildDependencyTable_insert(table, &objEntry);
			buildDependencyEntry_reset(&objEntry);
#endif

			buildDependencyTable_insert(table, &entry);
			buildDependencyEntry_reset(&entry);
		}
	}
}

void buildDependencyTable_insertFromDir(buildDependencyTable *depTable, sourceTimestampTable *timeTable, sbs_t *sourcePaths, const char *sourceDir, const char *objectDir, b32 bRecursive, b32 bSourceOnly, b32 bDebug)
{
	sb_t dir = sb_from_c_string(sourceDir);
	path_resolve_inplace(&dir);

	DIR *d = opendir(sb_get(&dir));
	if(d) {
		struct dirent *entry;
		while((entry = readdir(d)) != NULL) {
			if(entry->d_type == DT_DIR) {
				if(entry->d_name[0] != '.') {
					if(bRecursive) {
						sb_t subDir = sb_from_va("%s\\%s", sourceDir, entry->d_name);
						buildDependencyTable_insertFromDir(depTable, timeTable, sourcePaths, sb_get(&subDir), objectDir, bRecursive, bSourceOnly, bDebug);
						sb_reset(&subDir);
					}
				}
			} else {
				const char *ext = strrchr(entry->d_name, '.');
				if(ext) {
					if(!bb_stricmp(ext, ".c") || !bb_stricmp(ext, ".cpp") || (!bSourceOnly && (!bb_stricmp(ext, ".h") || !bb_stricmp(ext, ".o")))) {
						sb_t path = BuildFilePath(dir, entry->d_name);

						if(timeTable) {
							sourceTimestampEntry timestampEntry = { BB_EMPTY_INITIALIZER };
							timestampEntry.key = path;
							timestampEntry.timestamp = GetFileTimeMilliseconds(sb_get(&path));
							sourceTimestampTable_insert(timeTable, &timestampEntry);
						}

						if(depTable) {
							sb_t depPath = sb_from_c_string(objectDir);
							sb_va(&depPath, "%c%.*s.d", path_get_separator(), ext - entry->d_name, entry->d_name);
							path_resolve_inplace(&depPath);
							buildDependencyTable_insertFromFile(depTable, sb_get(&depPath), sb_get(&path), bDebug);
							sb_reset(&depPath);
						}

						if(sourcePaths) {
							if(sourcePaths) {
								bba_push(*sourcePaths, path);
							} else {
								sb_reset(&path);
							}
						} else {
							sb_reset(&path);
						}
					}
				}
			}
		}
		closedir(d);
	}

	sb_reset(&dir);
}

b32 buildDependencyTable_checkDeps(buildDependencyTable *deps, sourceTimestampTable *times, const char *path, b32 bDebug)
{
	sourceTimestampEntry *ste = sourceTimestampTable_find(times, path);
	if(!ste) {
		if(bDebug) {
			BB_LOG("Timestamps", "  (no source timestamp)");
		}
		return false;
	}

	buildDependencyEntry *entry = buildDependencyTable_find(deps, path);
	if(entry) {
		b32 bUpToDate = true;
		for(u32 j = 0; j < entry->deps.count; ++j) {
			sb_t *dep = entry->deps.data + j;
			const char *depPath = sb_get(dep);
			sourceTimestampEntry *dte = sourceTimestampTable_find(times, depPath);
			if(dte) {
				if(ste->timestamp < dte->timestamp) {
					if(bDebug) {
						BB_LOG("Timestamps", "  %s (out of date)", depPath);
					}
					bUpToDate = false;
				} else {
					if(bDebug) {
						BB_LOG("Timestamps", "  %s", depPath);
					}
				}
			} else {
				if(bDebug) {
					BB_LOG("Timestamps", "%s (no timestamp)", depPath);
				}
				bUpToDate = false;
			}
		}
		return bUpToDate;
	} else {
		if(bDebug) {
			BB_LOG("Timestamps", "%s (not found)", path);
		}
		return false;
	}
}

u32 buildDependencyTable_queueCommands(buildCommands_t *commands, buildDependencyTable *deps, sourceTimestampTable *times, sbs_t *sourcePaths, const char *objectDir, b32 bDebug, b32 bRebuild, const char *dir, const char *parameterizedCommandIn)
{
	sb_t parameterizedCommand = sb_from_c_string(parameterizedCommandIn);
	u32 count = 0;
	for(u32 i = 0; i < sourcePaths->count; ++i) {
		sb_t *path = sourcePaths->data + i;
		if(bDebug) {
			BB_LOG("Timestamps", "[%s]", sb_get(path));
		}

		sb_t objectPath = buildUtils_objectPathFromSourcePath(objectDir, sb_get(path));
		b32 bUpToDate = buildDependencyTable_checkDeps(deps, times, sb_get(&objectPath), bDebug);
		if(!bUpToDate || bRebuild) {
			sb_t srcPath = sb_clone(path);
			sb_replace_all_inplace(&srcPath, "\\", "/");
			sb_replace_all_inplace(&objectPath, "\\", "/");
			const char *title = path_get_filename(sb_get(path));

			sb_t command = sb_clone(&parameterizedCommand);
			sb_replace_all_inplace(&command, "{SOURCE_PATH}", sb_get(&srcPath));
			sb_replace_all_inplace(&command, "{OBJECT_PATH}", sb_get(&objectPath));
			buildCommands_push(commands, title, dir, sb_get(&command));
			sb_reset(&srcPath);
			sb_reset(&command);
			++count;
		}
		sb_reset(&objectPath);
	}
	sb_reset(&parameterizedCommand);
	return count;
}

void buildDependencyTable_dump(buildDependencyTable *table)
{
	for(u32 i = 0; i < table->count; ++i) {
		buildDependencyChain *chain = table->data + i;
		for(u32 j = 0; j < chain->count; ++j) {
			buildDependencyEntry *entry = chain->data + j;
			BB_LOG("Dependencies", "[%d,%d] %s (%u)", i, j, sb_get(&entry->key), entry->deps.count);
		}
	}
}

void sourceTimestampTable_dump(sourceTimestampTable *table)
{
	for(u32 i = 0; i < table->count; ++i) {
		sourceTimestampChain *chain = table->data + i;
		for(u32 j = 0; j < chain->count; ++j) {
			sourceTimestampEntry *entry = chain->data + j;
			BB_LOG("Timestamps", "[%d,%d] %s", i, j, sb_get(&entry->key));
		}
	}
}
