// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "path_utils.h"
#include "bbclient/bb_array.h"
#include "bbclient/bb_wrap_stdio.h"

const char *path_get_filename(const char *path)
{
	const char *sep = strrchr(path, '/');
	if(sep)
		return sep + 1;
	sep = strrchr(path, '\\');
	if(sep)
		return sep + 1;
	return path;
}

sb_t path_resolve(sb_t src)
{
	sb_t pathData;
	sb_init(&pathData);
	sb_t *path = &pathData;

	const char *in = sb_get(&src);
	while(*in) {
		char c = *in;
		if(c == '/') {
			c = '\\';
		}

		b32 handled = false;
		if(c == '.' && path->count > 1 && path->data[path->count - 2] == '\\') {
			if(in[1] == '.' && (in[2] == '/' || in[2] == '\\' || !in[2])) {
				char *prevSep = path->data + path->count - 2;
				while(prevSep > path->data) {
					--prevSep;
					if(*prevSep == '\\') {
						*prevSep = 0;
						path->count = (u32)(prevSep - path->data) + 1;
						sb_append_char(path, '\\');
						in += 2;
						if(*in) {
							++in;
						}
						handled = true;
						break;
					}
				}
			} else if(in[1] == '/' || in[1] == '\\' || !in[1]) {
				++in;
				if(*in) {
					++in;
				}
				handled = true;
			}
		}

		if(path->count > 2 && c == '\\' && path->data[path->count - 2] == '\\') {
			handled = true;
			++in;
		}

		if(!handled) {
			sb_append_char(path, c);
			++in;
		}
	}

	if(path->count > 1 && path->data[path->count - 2] == '\\') {
		path->data[path->count - 2] = '\0';
		--path->count;
	}

#if !BB_USING(BB_PLATFORM_WINDOWS)
	for(u32 i = 0; i < pathData.count; ++i) {
		if(pathData.data[i] == '\\') {
			pathData.data[i] = '/';
		}
	}
#endif
	return pathData;
}

void path_resolve_inplace(sb_t *path)
{
	sb_t tmp = path_resolve(*path);
	sb_reset(path);
	*path = tmp;
}

void path_remove_filename(sb_t *path)
{
	if(!path || !path->data)
		return;

	char *forwardSlash = strrchr(path->data, '/');
	char *backslash = strrchr(path->data, '\\');
	char *sep = forwardSlash > backslash ? forwardSlash : backslash;
	if(sep) {
		*sep = 0;
		path->count = (u32)(sep - path->data) + 1;
	} else {
		path->data[0] = 0;
		path->count = 0;
	}
}

#if BB_USING(BB_PLATFORM_WINDOWS)
#include <direct.h>
b32 path_mkdir(const char *path)
{
	b32 success = true;
	char *temp = _strdup(path);
	char *s = temp;
	while(*s) {
		if(*s == '/' || *s == '\\') {
			char c = *s;
			*s = '\0';
			if(s - temp > 2) {
				if(_mkdir(temp) == -1) {
					if(errno != EEXIST) {
						success = false;
					}
				}
			}
			*s = c;
		}
		++s;
	}
	free(temp);
	if(_mkdir(path) == -1) {
		if(errno != EEXIST) {
			success = false;
		}
	}
	return success;
}
b32 path_rmdir(const char *path)
{
	return _rmdir(path) == 0;
}
#else
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
static const char *errno_str(int e)
{
	switch(e) {
#define CASE(x) \
	case x:     \
		return #x
		CASE(EACCES);
		CASE(EEXIST);
		CASE(ELOOP);
		CASE(EMLINK);
		CASE(ENAMETOOLONG);
		CASE(ENOENT);
		CASE(ENOSPC);
		CASE(ENOTDIR);
		CASE(EROFS);
	case 0:
		return "success";
	default:
		return "???";
	}
}

b32 path_mkdir(const char *path)
{
	mode_t process_mask = umask(0);
	int ret = mkdir(path, S_IRWXU);
	umask(process_mask);
	if(ret && errno != EEXIST) {
		BB_WARNING("mkdir", "mkdir '%s' returned %d (errno %d %s)\n", path, ret, errno, errno_str(errno));
	}
	return !ret || errno == EEXIST; // not completely correct as EEXIST could be a file
}

b32 path_rmdir(const char *path)
{
	return rmdir(path) == 0;
}
#endif
