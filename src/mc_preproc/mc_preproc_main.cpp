#include "mc_preproc.h"

#include "bbclient/bb_string.h"
#include "cmdline.h"
#include "crt_leak_check.h"

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
bool g_bHeaderOnly;

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

bool mm_lexer_parse_struct(lexer *lex, bool autovalidate, bool headerOnly, bool fromLoc, bool isTypedef)
{
	lexer_token name;
	if(!lexer_read_on_line(lex, &name)) {
		BB_ERROR("mm_lexer::parse_struct", "Failed to parse 'unknown': expected name on line %u", lex->line);
		return false;
	}

	struct_s s = { lexer_token_string(name), "", autovalidate, headerOnly, fromLoc };
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

void mm_lexer_scan_file(const char *text, lexer_size text_length, const char *path, const sb_t *basePath)
{
	/* initialize lexer */
	lexer lex;
	lexer_init(&lex, text, text_length, NULL, NULL, NULL);

	/* parse tokens */

	bool foundAny = false;
	while(lexer_skip_until(&lex, "AUTOJSON")) {
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

			while(1) {
				if(lexer_check_string(&lex, "AUTOVALIDATE")) {
					autovalidate = true;
				} else if(lexer_check_string(&lex, "AUTOHEADERONLY")) {
					headerOnly = true;
				} else if(lexer_check_string(&lex, "AUTOFROMLOC")) {
					fromLoc = true;
				} else if(lexer_check_string(&lex, "typedef")) {
					isTypedef = true;
				} else {
					break;
				}
			}

			if(lexer_check_string(&lex, "struct")) {
				foundAny = mm_lexer_parse_struct(&lex, autovalidate, headerOnly, fromLoc, isTypedef) || foundAny;
			}
		}
	}

	if(foundAny) {
		g_paths.insert(path + basePath->count);
	}
}

void find_files_in_dir(const char *dir, const char *desiredExt, sdict_t *sd)
{
	WIN32_FIND_DATA find;
	HANDLE hFind;

	sb_t filter = {};
	sb_va(&filter, "%s\\*.*", dir);
	if(INVALID_HANDLE_VALUE != (hFind = FindFirstFileA(sb_get(&filter), &find))) {
		do {
			if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if(find.cFileName[0] != '.') {
					sb_t subdir = {};
					sb_va(&subdir, "%s\\%s", dir, find.cFileName);
					find_files_in_dir(sb_get(&subdir), desiredExt, sd);
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

static void scanHeaders(const sb_t *scanDir)
{
	sdict_t sd = {};
	find_files_in_dir(sb_get(scanDir), ".h", &sd);
	sdict_sort(&sd);

	for(u32 i = 0; i < sd.count; ++i) {
		const char *path = sb_get(&sd.data[i].key);
		fileData_t contents = fileData_read(path);
		if(!contents.buffer)
			continue;

		BB_LOG("mm_lexer::scan_file", "^8%s", path);
		mm_lexer_scan_file((char *)contents.buffer, contents.bufferSize, path, scanDir);
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
	sb_append(s, "#include \"bbclient/bb_common.h\"\n");
	sb_append(s, "\n");
	if(file_readable(sb_get(&freetypePath))) {
		sb_append(s, "#define FEATURE_FREETYPE BB_ON\n");
	} else {
		sb_append(s, "#define FEATURE_FREETYPE BB_OFF\n");
	}

	sb_t path;
	sb_init(&path);
	sb_va(&path, "%s\\fonts_generated.h", sb_get(outDir));
	if(fileData_writeIfChanged(sb_get(&path), NULL, { data.data, sb_len(s) })) {
		BB_LOG("preproc", "updated %s", sb_get(&path));
	}
	sb_reset(&path);
	sb_reset(&data);
	sb_reset(&freetypePath);
}

int CALLBACK WinMain(_In_ HINSTANCE /*Instance*/, _In_opt_ HINSTANCE /*PrevInstance*/, _In_ LPSTR CommandLine, _In_ int /*ShowCode*/)
{
	crt_leak_check_init();

	cmdline_init_composite(CommandLine);

	for(int i = 1; i < cmdline_argc(); ++i) {
		if(!bb_stricmp(cmdline_argv(i), "-bb")) {
			BB_INIT("mc_common_preproc");
			break;
		}
	}

	BB_THREAD_SET_NAME("main");
	BB_LOG("Startup", "Command line: %s", CommandLine);

	const char *prefix = "";
	sb_t srcDir = { BB_EMPTY_INITIALIZER };
	sb_t includeDir = { BB_EMPTY_INITIALIZER };
	b32 checkFonts = false;

	for(int i = 1; i < cmdline_argc(); ++i) {
		const char *arg = cmdline_argv(i);
		if(!bb_strnicmp(arg, "-prefix=", 8)) {
			prefix = arg + 8;
		} else if(!bb_strnicmp(arg, "-src=", 5)) {
			sb_append(&srcDir, arg + 5);
		} else if(!bb_strnicmp(arg, "-include=", 9)) {
			sb_append(&includeDir, arg + 9);
		} else if(!bb_stricmp(cmdline_argv(i), "-fonts")) {
			checkFonts = true;
		}
	}

	if(sb_len(&srcDir) < 1) {
		sb_append(&srcDir, ".");
	}
	if(sb_len(&includeDir) < 1) {
		sb_append(&includeDir, sb_get(&srcDir));
	}

	path_resolve_inplace(&srcDir);
	path_resolve_inplace(&includeDir);

	for(int i = 1; i < cmdline_argc(); ++i) {
		const char *arg = cmdline_argv(i);
		if(!bb_strnicmp(arg, "-prefix=", 8)) {
		} else if(!bb_strnicmp(arg, "-src=", 5)) {
		} else if(!bb_strnicmp(arg, "-include=", 9)) {
		} else if(!bb_stricmp(cmdline_argv(i), "-fonts")) {
		} else if(!bb_strnicmp(arg, "-header=", 8)) {
			sb_t scanDir = {};
			sb_append(&scanDir, arg + 8);
			path_resolve_inplace(&scanDir);
			g_bHeaderOnly = true;
			scanHeaders(&scanDir);
			g_bHeaderOnly = false;
			sb_reset(&scanDir);
		} else {
			sb_t scanDir = {};
			sb_append(&scanDir, arg);
			path_resolve_inplace(&scanDir);
			scanHeaders(&scanDir);
			sb_reset(&scanDir);
		}
	}

	GenerateJson(prefix, &srcDir, &includeDir);
	GenerateStructs(prefix, &srcDir, &includeDir);
	if(checkFonts) {
		CheckFreeType(&includeDir);
	}
	sb_reset(&srcDir);
	sb_reset(&includeDir);

	BB_SHUTDOWN();

	cmdline_shutdown();

	return 0;
}
