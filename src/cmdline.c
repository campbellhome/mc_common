// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "cmdline.h"
#include "bb_array.h"
#include "bb_string.h"
#include "sb.h"
#include "tokenize.h"
#include <stdlib.h>

static int s_argc;
static const char **s_argv;
static char *s_exeDir;
static char *s_exeFilename;
static sb_t s_commandline;

static void cmdline_quote_and_append(sb_t *str, const char *arg)
{
	if(sb_len(str)) {
		sb_append_char(str, ' ');
	}
	if(strchr(arg, ' ')) {
		sb_append_char(str, '\"');
		sb_append(str, arg);
		sb_append_char(str, '\"');
	} else {
		sb_append(str, arg);
	}
}

static void cmdline_build_full(void)
{
	for(int i = 0; i < s_argc; ++i) {
		cmdline_quote_and_append(&s_commandline, cmdline_argv(i));
	}
}

#if BB_USING(BB_PLATFORM_WINDOWS)

static char *s_argBuffer;
static char **s_argvBuffer;
static char s_argv0[_MAX_PATH];

void cmdline_init_exe(void)
{
	char exePath[_MAX_PATH] = "";
	GetModuleFileNameA(0, exePath, sizeof(exePath));
	char *exeSep = strrchr(exePath, '\\');
	*exeSep = 0;
	s_exeDir = bb_strdup(exePath);
	s_exeFilename = bb_strdup(exeSep + 1);
}

void cmdline_init_composite(const char *src)
{
	cmdline_init_exe();
	s_argBuffer = bb_strdup(src);
	if(s_argBuffer) {
		++s_argc;
		const char *cursor = s_argBuffer;
		span_t token = tokenize(&cursor, " ");
		while(token.start) {
			++s_argc;
			token = tokenize(&cursor, " ");
		}

		s_argvBuffer = malloc(sizeof(char *) * s_argc);
		s_argv = (const char **)s_argvBuffer;
		if(s_argv) {
			s_argv[0] = s_argv0;
			GetModuleFileNameA(NULL, s_argv0, BB_ARRAYSIZE(s_argv0));
			cursor = s_argBuffer;
			for(int i = 1; i < s_argc; ++i) {
				token = tokenize(&cursor, " ");
				if(*cursor)
					++cursor;
				*(char *)token.end = '\0';
				*(s_argv + i) = token.start;
			}
		} else {
			s_argc = 0;
		}
	}
	cmdline_build_full();
}
#else // #if BB_USING(BB_PLATFORM_WINDOWS)

void cmdline_init_exe(void)
{
	char *exePath = bb_strdup(s_argv[0]);
	char *exeSep = strrchr(exePath, '/');
	*exeSep = 0;
	s_exeDir = bb_strdup(exePath);
	s_exeFilename = bb_strdup(exeSep + 1);
}

#endif // #else // #if BB_USING(BB_PLATFORM_WINDOWS)

void cmdline_init(int _argc, const char **_argv)
{
	s_argc = _argc;
	s_argv = _argv;
	cmdline_init_exe();
	cmdline_build_full();
}

void cmdline_shutdown(void)
{
	sb_reset(&s_commandline);
	if(s_exeDir) {
		free(s_exeDir);
	}
	if(s_exeFilename) {
		free(s_exeFilename);
	}
#if BB_USING(BB_PLATFORM_WINDOWS)
	if(s_argBuffer) {
		free(s_argBuffer);
	}
	if(s_argvBuffer) {
		free(s_argvBuffer);
	}
#endif // #if BB_USING(BB_PLATFORM_WINDOWS)
}

int cmdline_argc(void)
{
	return s_argc;
}

const char *cmdline_argv(int index)
{
	return index >= 0 && index < s_argc ? s_argv[index] : NULL;
}

int cmdline_find(const char *arg)
{
	for(int i = 1; i < s_argc; ++i) {
		if(!bb_stricmp(cmdline_argv(i), arg)) {
			return i;
		}
	}
	return -1;
}

const char *cmdline_find_prefix(const char *prefix)
{
	size_t prefixLen = strlen(prefix);
	for(int i = 1; i < s_argc; ++i) {
		if(!bb_strnicmp(cmdline_argv(i), prefix, prefixLen)) {
			return s_argv[i] + prefixLen;
		}
	}
	return NULL;
}

const char *cmdline_get_exe_dir(void)
{
	return s_exeDir;
}

const char *cmdline_get_exe_filename(void)
{
	return s_exeFilename;
}

const char *cmdline_get_full(void)
{
	return sb_get(&s_commandline);
}
