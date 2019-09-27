#include "mc_preproc.h"

#include "bb_string.h"
#include "cmdline.h"
#include "crt_leak_check.h"
#include "mc_preproc_config.h"
#include "span.h"
#include "va.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEXER_IMPLEMENTATION
#define LEXER_STATIC
#include "mmx/lexer.h"

#include "parson/parson.h"

#include <windows.h>

std::vector< enum_s > g_enums;
std::vector< struct_s > g_structs;
std::set< std::string > g_paths;
std::set< std::string > g_ignorePaths;
bool g_bHeaderOnly;
const char *g_includePrefix = "";

std::string lexer_token_string(const lexer_token &tok)
{
	char buffer[1024];
	buffer[0] = 0;
	if(tok.type == LEXER_TOKEN_STRING) {
		buffer[0] = '\"';
		lexer_size count = lexer_token_cpy(buffer + 1, 1022, &tok);
		buffer[count + 1] = '\"';
		buffer[count + 2] = 0;
	} else if(tok.type == LEXER_TOKEN_NUMBER && (tok.subtype & LEXER_TOKEN_SINGLE_PREC) != 0) {
		lexer_size count = lexer_token_cpy(buffer, 1023, &tok);
		buffer[count] = 'f';
		buffer[count + 1] = 0;
	} else {
		lexer_token_cpy(buffer, 1024, &tok);
	}
	return buffer;
}

bool mm_lexer_parse_enum(lexer *lex, std::string defaultVal, bool isTypedef)
{
	lexer_token name;
	if(!lexer_read_on_line(lex, &name))
		return false;

	BB_LOG("mm_lexer", "AUTOJSON enum %s", lexer_token_string(name).c_str());
	enum_s e = { lexer_token_string(name), "" };

	lexer_token tok;
	if(!lexer_expect_type(lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_BRACE_OPEN, &tok))
		return false;

	while(1) {
		if(!lexer_read(lex, &tok))
			break;

		if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACE_CLOSE) {
			BB_LOG("mm_lexer", "AUTOJSON end enum");
			if(isTypedef) {
				e.typedefBaseName = e.name;
				if(lexer_read(lex, &tok) && tok.type == LEXER_TOKEN_NAME) {
					e.name = lexer_token_string(tok);
				}
			}
			break;
		}

		if(tok.type != LEXER_TOKEN_NAME) {
			break;
		}

		enum_member_s member = { lexer_token_string(tok) };
		BB_LOG("mm_lexer", "member: %s", member.name.c_str());

		e.members.push_back(member);

		if(!lexer_read_on_line(lex, &tok)) {
			if(!lexer_read(lex, &tok))
				break;
			if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACE_CLOSE) {
				BB_LOG("mm_lexer", "AUTOJSON end enum");
				if(isTypedef) {
					e.typedefBaseName = e.name;
					if(lexer_read(lex, &tok) && tok.type == LEXER_TOKEN_NAME) {
						e.name = lexer_token_string(tok);
					}
				}
			}
			break;
		}

		if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_COMMA) {
			BB_LOG("mm_lexer", "no value");
		} else if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_ASSIGN) {
			BB_LOG("mm_lexer", "value");
			lexer_skip_line(lex);
		} else {
			break;
		}
	}

	if(!e.members.empty()) {
		if(defaultVal.empty()) {
			e.defaultVal = e.members.back().name;
		} else {
			e.defaultVal = defaultVal;
		}
		g_enums.push_back(e);
		return true;
	}
	return false;
}

bool mm_lexer_parse_struct(lexer *lex, bool autovalidate, bool headerOnly, bool fromLoc, bool isTypedef, bool jsonSerialization, bool bStringHash)
{
	lexer_token name;
	if(!lexer_read_on_line(lex, &name)) {
		BB_ERROR("mm_lexer::parse_struct", "Failed to parse 'unknown': expected name on line %u", lex->line);
		return false;
	}

	struct_s s = { lexer_token_string(name), "", autovalidate, headerOnly, fromLoc, jsonSerialization, bStringHash };
	//BB_LOG("mm_lexer", "AUTOJSON struct %s on line %u", s.name.c_str(), name.line);

	lexer_token tok;
	if(!lexer_expect_type(lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_BRACE_OPEN, &tok)) {
		BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected { on line %u", s.name.c_str(), lex->line);
		return false;
	}

	int indent = 1;
	while(1) {
		if(!lexer_read(lex, &tok)) {
			BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': out of data on line %u", s.name.c_str(), lex->line);
			return false;
		}

		if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACE_CLOSE) {
			--indent;
			if(!indent) {
				if(isTypedef) {
					s.typedefBaseName = s.name;
					if(lexer_read(lex, &tok) && tok.type == LEXER_TOKEN_NAME) {
						s.name = lexer_token_string(tok);
					}
				}
				break;
			}
		} else if(tok.type == LEXER_TOKEN_NAME) {
			std::vector< lexer_token > tokens;
			tokens.push_back(tok);
			size_t equalsIndex = 0;
			size_t bracketIndex = 0;
			while(1) {
				if(!lexer_read(lex, &tok)) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': out of data on line %u", s.name.c_str(), lex->line);
					return false;
				}
				if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_SEMICOLON) {
					break;
				} else {
					if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_ASSIGN) {
						if(!equalsIndex) {
							equalsIndex = tokens.size();
						}
					}
					if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACKET_OPEN) {
						if(!bracketIndex) {
							bracketIndex = tokens.size();
						}
					}
					tokens.push_back(tok);
				}
			}

			if(tokens.size() < 2)
				return false;

			if(equalsIndex > 0 && bracketIndex > 0) {
				BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': [] and = cannot be combined on line %u", s.name.c_str(), lex->line);
				return false;
			}

			if(equalsIndex > 1) {
				lexer_token name = tokens[equalsIndex - 1];
				if(name.type != LEXER_TOKEN_NAME) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected name on line %u", s.name.c_str(), lex->line);
					return false;
				}

				std::string typestr;
				for(auto i = 0; i < equalsIndex - 1; ++i) {
					typestr += " ";
					typestr += lexer_token_string(tokens[i]);
				}

				std::string valstr;
				for(auto i = equalsIndex + 1; i < tokens.size(); ++i) {
					valstr += " ";
					valstr += lexer_token_string(tokens[i]);
				}

				//BB_LOG("mm_lexer", "member %s:\n  type:%s\n  val:%s", lexer_token_string(name), typestr.c_str(), valstr.c_str());

				struct_member_s m;
				m.name = lexer_token_string(name);
				m.val = valstr[0] == ' ' ? valstr.c_str() + 1 : valstr;
				for(auto i = 0; i < equalsIndex - 1; ++i) {
					m.typeTokens.push_back(tokens[i]);
				}
				s.members.push_back(m);
			} else if(bracketIndex > 1) {
				lexer_token name = tokens[bracketIndex - 1];
				if(name.type != LEXER_TOKEN_NAME) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected name on line %u", s.name.c_str(), lex->line);
					return false;
				}

				std::string typestr;
				for(auto i = 0; i < bracketIndex - 1; ++i) {
					typestr += " ";
					typestr += lexer_token_string(tokens[i]);
				}

				std::string valstr;
				for(auto i = bracketIndex + 1; i < tokens.size() - 1; ++i) {
					valstr += " ";
					valstr += lexer_token_string(tokens[i]);
				}

				//BB_LOG("mm_lexer", "member %s:\n  type:%s\n  val:%s", lexer_token_string(name), typestr.c_str(), valstr.c_str());

				struct_member_s m;
				m.name = lexer_token_string(name);
				m.arr = valstr[0] == ' ' ? valstr.c_str() + 1 : valstr;
				for(auto i = 0; i < bracketIndex - 1; ++i) {
					m.typeTokens.push_back(tokens[i]);
				}
				s.members.push_back(m);
			} else if(!equalsIndex) {
				lexer_token name = tokens.back();
				if(name.type != LEXER_TOKEN_NAME) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected name on line %u", s.name.c_str(), lex->line);
					return false;
				}

				std::string typestr;
				for(auto i = 0; i < tokens.size() - 1; ++i) {
					typestr += " ";
					typestr += lexer_token_string(tokens[i]);
				}

				//BB_LOG("mm_lexer", "member %s:\n  type:%s", lexer_token_string(name), typestr.c_str());
				struct_member_s m;

				m.name = lexer_token_string(name);
				for(auto i = 0; i < tokens.size() - 1; ++i) {
					m.typeTokens.push_back(tokens[i]);
				}
				s.members.push_back(m);
			}
		} else {
			BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': unexpected token on line %u", s.name.c_str(), lex->line);
			return false;
		}
	}

	//BB_LOG("mm_lexer", "struct end");

	sb_t sb = {};
	sb_append(&sb, "AUTOJSON ");
	if(isTypedef) {
		sb_append(&sb, "typedef ");
	} else {
		sb_append(&sb, "struct ");
	}

	if(autovalidate) {
		sb_append(&sb, "with AUTOVALIDATE ");
	}
	sb_va(&sb, "'%s'\n", s.name.c_str());

	for(auto &m : s.members) {
		std::string ps;
		lexer_token pt = {};
		for(const auto &it : m.typeTokens) {
			std::string s = lexer_token_string(it);
			if(s == ">") {
				m.typeStr += " >";
			} else {
				if(pt.type == LEXER_TOKEN_NAME && it.type == LEXER_TOKEN_NAME) {
					m.typeStr += " ";
				}
				m.typeStr += s;
				if(s == "<") {
					m.typeStr += " ";
				}
			}
			ps = s;
			pt = it;
		}

		if(!m.val.empty()) {
			sb_va(&sb, "  %s %s = %s\n", m.typeStr.c_str(), m.name.c_str(), m.val.c_str());
		} else if(!m.arr.empty()) {
			sb_va(&sb, "  %s %s[%s]\n", m.typeStr.c_str(), m.name.c_str(), m.arr.c_str());
		} else {
			sb_va(&sb, "  %s %s\n", m.typeStr.c_str(), m.name.c_str());
		}
	}
	g_structs.push_back(s);

	BB_LOG("mm_lexer", "%s", sb_get(&sb));
	sb_reset(&sb);
	return true;
}

static void mm_lexer_scan_file_for_keyword(const char *text, lexer_size text_length, const char *path, const sb_t *basePath, const char *keyword)
{
	/* initialize lexer */
	lexer lex;
	lexer_init(&lex, text, text_length, NULL, NULL, NULL);

	/* parse tokens */

	bool jsonSerialization = !bb_stricmp(keyword, "AUTOJSON");
	bool foundAny = false;
	while(lexer_skip_until(&lex, keyword)) {
		bool isTypedef = lexer_check_string(&lex, "typedef");
		if(lexer_check_string(&lex, "enum")) {
			foundAny = mm_lexer_parse_enum(&lex, "", isTypedef) || foundAny;
		} else if(lexer_check_string(&lex, "AUTODEFAULT")) {
			std::string defaultVal;
			lexer_token tok;
			if(lexer_check_type(&lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_PARENTHESE_OPEN, &tok)) {
				if(lexer_read(&lex, &tok)) {
					if(tok.type == LEXER_TOKEN_NAME) {
						defaultVal = lexer_token_string(tok);
						if(lexer_check_type(&lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_PARENTHESE_CLOSE, &tok)) {
							if(lexer_check_string(&lex, "typedef")) {
								isTypedef = true;
							}
							if(lexer_check_string(&lex, "enum")) {
								foundAny = mm_lexer_parse_enum(&lex, defaultVal, isTypedef) || foundAny;
							}
						}
					}
				}
			}
		} else {
			bool autovalidate = false;
			bool headerOnly = g_bHeaderOnly;
			bool fromLoc = false;
			bool bStringHash = false;

			while(1) {
				if(lexer_check_string(&lex, "AUTOVALIDATE")) {
					autovalidate = true;
				} else if(lexer_check_string(&lex, "AUTOHEADERONLY")) {
					headerOnly = true;
				} else if(lexer_check_string(&lex, "AUTOFROMLOC")) {
					fromLoc = true;
				} else if(lexer_check_string(&lex, "AUTOSTRINGHASH")) {
					bStringHash = true;
				} else if(lexer_check_string(&lex, "typedef")) {
					isTypedef = true;
				} else {
					break;
				}
			}

			if(lexer_check_string(&lex, "struct")) {
				foundAny = mm_lexer_parse_struct(&lex, autovalidate, headerOnly, fromLoc, isTypedef, jsonSerialization, bStringHash) || foundAny;
			}
		}
	}

	if(foundAny) {
		if(g_bHeaderOnly) {
			g_paths.insert(path + basePath->count);
		} else {
			g_paths.insert((std::string)g_includePrefix + (path + basePath->count));
		}
	}
}

void mm_lexer_scan_file(const char *text, lexer_size text_length, const char *path, const sb_t *basePath)
{
	mm_lexer_scan_file_for_keyword(text, text_length, path, basePath, "AUTOJSON");
	mm_lexer_scan_file_for_keyword(text, text_length, path, basePath, "AUTOSTRUCT");
}

void find_files_in_dir(const char *dir, const char *desiredExt, sdict_t *sd, bool bRecurse)
{
	WIN32_FIND_DATA find;
	HANDLE hFind;

	sb_t filter = {};
	sb_va(&filter, "%s\\*.*", dir);
	if(INVALID_HANDLE_VALUE != (hFind = FindFirstFileA(sb_get(&filter), &find))) {
		do {
			if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if(find.cFileName[0] != '.' && bRecurse) {
					sb_t subdir = {};
					sb_va(&subdir, "%s\\%s", dir, find.cFileName);
					if(g_ignorePaths.find(sb_get(&subdir)) == g_ignorePaths.end()) {
						find_files_in_dir(sb_get(&subdir), desiredExt, sd, bRecurse);
					}
					sb_reset(&subdir);
				}
			} else {
				const char *ext = strrchr(find.cFileName, '.');
				if(ext && !_stricmp(ext, desiredExt)) {
					sdictEntry_t entry = {};
					sb_va(&entry.key, "%s\\%s", dir, find.cFileName);
					path_resolve_inplace(&entry.key);
					sb_append(&entry.value, find.cFileName);
					sdict_add(sd, &entry);
				}
			}
		} while(FindNextFileA(hFind, &find));
		FindClose(hFind);
	}
	sb_reset(&filter);
}

static void scanHeaders(const sb_t *scanDir, bool bRecurse, const sb_t *baseDir)
{
	BB_LOG("mm_lexer::scan_dir", "^=%s dir: %s", g_bHeaderOnly ? "header" : "src", sb_get(scanDir));
	sdict_t sd = {};
	find_files_in_dir(sb_get(scanDir), ".h", &sd, bRecurse);
	sdict_sort(&sd);

	for(u32 i = 0; i < sd.count; ++i) {
		const char *path = sb_get(&sd.data[i].key);
		fileData_t contents = fileData_read(path);
		if(!contents.buffer)
			continue;

		BB_LOG("mm_lexer::scan_file", "^8%s", path);
		mm_lexer_scan_file((char *)contents.buffer, contents.bufferSize, path, (baseDir && baseDir->count <= scanDir->count) ? baseDir : scanDir);
	}

	sdict_reset(&sd);
}

void CheckFreeType(sb_t *outDir)
{
	sb_t data;
	sb_init(&data);
	sb_t *s = &data;

	sb_t freetypePath;
	sb_init(&freetypePath);
	sb_va(&freetypePath, "%s\\..\\submodules\\freetype\\include\\freetype\\freetype.h", sb_get(outDir));
	path_resolve_inplace(&freetypePath);

	sb_append(s, "// Copyright (c) 2012-2019 Matt Campbell\n");
	sb_append(s, "// MIT license (see License.txt)\n");
	sb_append(s, "\n");
	sb_append(s, "// AUTOGENERATED FILE - DO NOT EDIT\n");
	sb_append(s, "\n");
	sb_append(s, "// clang-format off\n");
	sb_append(s, "\n");
	sb_append(s, "#pragma once\n");
	sb_append(s, "\n");
	sb_append(s, "#include \"bb_common.h\"\n");
	sb_append(s, "\n");
	if(file_readable(sb_get(&freetypePath))) {
		sb_append(s, "#define FEATURE_FREETYPE BB_ON\n");
	} else {
		sb_append(s, "#define FEATURE_FREETYPE BB_OFF\n");
	}

	sb_t path;
	sb_init(&path);
	sb_va(&path, "%s\\fonts_generated.h", sb_get(outDir));
	ReportFileDataWriteIfChanged(fileData_writeIfChanged(sb_get(&path), NULL, { data.data, sb_len(s) }), sb_get(&path));
	sb_reset(&path);
	sb_reset(&data);
	sb_reset(&freetypePath);
}

void ReportFileDataWriteIfChanged(fileData_writeResult result, const char *path)
{
	switch(result) {
	case kFileData_Error:
		BB_ERROR("preproc", "Failed to update %s", path);
		break;
	case kFileData_Success:
		BB_LOG("preproc", "updated %s", path);
		break;
	case kFileData_Unmodified:
		BB_LOG("preproc", "Skipped updating %s", path);
		break;
	}
}

static sb_t GetConfigRelativePath(const span_t *configDir, const sb_t *dir)
{
	sb_t ret = { BB_EMPTY_INITIALIZER };
	if(configDir->end > configDir->start) {
		ret = sb_from_va("%.*s/%s", configDir->end - configDir->start, configDir->start, sb_get(dir));
	} else {
		ret = sb_clone(dir);
	}
	path_resolve_inplace(&ret);
	return ret;
}

static void scanHeaders(span_t *configDir, preprocInputDir *inputDir)
{
	sb_t scanDir = GetConfigRelativePath(configDir, &inputDir->dir);
	sb_t baseDir = { BB_EMPTY_INITIALIZER };
	if(inputDir->base.count) {
		baseDir = GetConfigRelativePath(configDir, &inputDir->base);
	} else {
		baseDir = sb_clone(&scanDir);
	}
	scanHeaders(&scanDir, false, &baseDir);
	sb_reset(&scanDir);
	sb_reset(&baseDir);
}

static void InitLogging(b32 bb)
{
	for(int i = 1; i < cmdline_argc(); ++i) {
		if(!bb_stricmp(cmdline_argv(i), "-bb")) {
			bb = true;
			break;
		}
	}

	if(bb) {
		BB_INIT("mc_common_preproc");
		bb_set_send_callback(&bb_echo_to_stdout, nullptr);
	}

	BB_THREAD_SET_NAME("main");
	BB_LOG("Startup", "Command line: %s", cmdline_get_full());

	char currentDirectory[1024];
	GetCurrentDirectoryA(sizeof(currentDirectory), currentDirectory);
	BB_LOG("Startup", "Working Directory: %s", currentDirectory);
}

static void GenerateFromJson(const char *configPath)
{
	span_t configDir = { BB_EMPTY_INITIALIZER };
	configDir.start = configPath;
	configDir.end = path_get_filename(configPath);
	preprocConfig config = read_preprocConfig(configPath);
	InitLogging(config.bb);
	printf("Running mc_common_preproc %s\n", path_get_filename(configPath));

	if(!config.input.sourceDirs.count && !config.input.includeDirs.count) {
		BB_ERROR("Config", "Empty config - bailing");
		reset_preprocConfig(&config);
		return;
	}

	g_bHeaderOnly = true;
	for(u32 i = 0; i < config.input.includeDirs.count; ++i) {
		scanHeaders(&configDir, config.input.includeDirs.data + i);
	}
	g_bHeaderOnly = false;
	for(u32 i = 0; i < config.input.sourceDirs.count; ++i) {
		scanHeaders(&configDir, config.input.sourceDirs.data + i);
	}

	const char *prefix = sb_get(&config.output.prefix);
	sb_t outputSourceDir = GetConfigRelativePath(&configDir, &config.output.sourceDir);
	sb_t outputIncludeDir = GetConfigRelativePath(&configDir, &config.output.includeDir);
	sb_t outputBaseDir = GetConfigRelativePath(&configDir, &config.output.baseDir);
	sb_t includePrefix = { BB_EMPTY_INITIALIZER };

	if(config.output.baseDir.count < config.output.includeDir.count &&
	   !bb_strnicmp(sb_get(&outputIncludeDir), sb_get(&outputBaseDir), outputBaseDir.count - 1)) {
		includePrefix = sb_from_va("%s/", sb_get(&outputIncludeDir) + outputBaseDir.count);
	}
	GenerateJson(prefix, sb_get(&includePrefix), &outputSourceDir, &outputIncludeDir);
	GenerateStructs(prefix, sb_get(&includePrefix), &outputSourceDir, &outputIncludeDir);
	if(config.input.checkFonts) {
		CheckFreeType(&outputIncludeDir);
	}
	sb_reset(&outputSourceDir);
	sb_reset(&outputIncludeDir);
	sb_reset(&outputBaseDir);
	sb_reset(&includePrefix);

	reset_preprocConfig(&config);
}

int CALLBACK WinMain(_In_ HINSTANCE /*Instance*/, _In_opt_ HINSTANCE /*PrevInstance*/, _In_ LPSTR CommandLine, _In_ int /*ShowCode*/)
{
	crt_leak_check_init();

	cmdline_init_composite(CommandLine);

	const char *prefix = "";
	sb_t srcDir = { BB_EMPTY_INITIALIZER };
	sb_t includeDir = { BB_EMPTY_INITIALIZER };
	sb_t configPath = { BB_EMPTY_INITIALIZER };
	b32 checkFonts = false;

	std::vector< const char * > deferredArgs;
	for(int i = 1; i < cmdline_argc(); ++i) {
		const char *arg = cmdline_argv(i);
		if(!bb_strnicmp(arg, "-prefix=", 8)) {
			prefix = arg + 8;
		} else if(!bb_strnicmp(arg, "-includePrefix=", 15)) {
			g_includePrefix = arg + 15;
		} else if(!bb_strnicmp(arg, "-src=", 5)) {
			sb_append(&srcDir, arg + 5);
		} else if(!bb_strnicmp(arg, "-include=", 9)) {
			sb_append(&includeDir, arg + 9);
		} else if(!bb_stricmp(arg, "-fonts")) {
			checkFonts = true;
		} else if(!bb_strnicmp(arg, "-config=", 8)) {
			sb_append(&configPath, arg + 8);
		} else if(!bb_stricmp(arg, "-bb")) {
			// already handled
		} else {
			// handle later
			deferredArgs.push_back(arg);
		}
	}

	if(configPath.data) {
		GenerateFromJson(sb_get(&configPath));
	} else {
		InitLogging(false);
	}

	if(configPath.count == 0 && 0) {
		if(sb_len(&srcDir) < 1) {
			sb_append(&srcDir, ".");
		}
		if(sb_len(&includeDir) < 1) {
			sb_append(&includeDir, sb_get(&srcDir));
		}

		path_resolve_inplace(&srcDir);
		path_resolve_inplace(&includeDir);

		bool bHeaderOnly = false;
		bool bRecurse = true;
		for(const char *arg : deferredArgs) {
			if(!bb_stricmp(arg, "-headeronly")) {
				bHeaderOnly = true;
			} else if(!bb_stricmp(arg, "-noheaderonly")) {
				bHeaderOnly = false;
			} else if(!bb_stricmp(arg, "-recurse")) {
				bRecurse = true;
			} else if(!bb_stricmp(arg, "-norecurse")) {
				bRecurse = false;
			} else if(!bb_strnicmp(arg, "-ignore=", 8)) {
				sb_t scanDir = {};
				sb_append(&scanDir, arg + 8);
				path_resolve_inplace(&scanDir);
				g_ignorePaths.insert(sb_get(&scanDir));
				sb_reset(&scanDir);
			} else if(!bb_strnicmp(arg, "-header=", 8)) {
				sb_t scanDir = {};
				sb_append(&scanDir, arg + 8);
				path_resolve_inplace(&scanDir);
				g_bHeaderOnly = true;
				scanHeaders(&scanDir, true, nullptr);
				g_bHeaderOnly = false;
				sb_reset(&scanDir);
			} else if(!bb_strnicmp(arg, "-headernorecurse=", 17)) {
				sb_t scanDir = {};
				sb_append(&scanDir, arg + 17);
				path_resolve_inplace(&scanDir);
				g_bHeaderOnly = true;
				scanHeaders(&scanDir, false, nullptr);
				g_bHeaderOnly = false;
				sb_reset(&scanDir);
			} else {
				sb_t scanDir = {};
				sb_append(&scanDir, arg);
				path_resolve_inplace(&scanDir);
				g_bHeaderOnly = bHeaderOnly;
				scanHeaders(&scanDir, bRecurse, nullptr);
				g_bHeaderOnly = false;
				sb_reset(&scanDir);
			}
		}

		GenerateJson(prefix, g_includePrefix, &srcDir, &includeDir);
		GenerateStructs(prefix, g_includePrefix, &srcDir, &includeDir);
		if(checkFonts) {
			CheckFreeType(&includeDir);
		}
	}
	sb_reset(&srcDir);
	sb_reset(&includeDir);
	sb_reset(&configPath);

	BB_SHUTDOWN();

	cmdline_shutdown();

	return 0;
}
