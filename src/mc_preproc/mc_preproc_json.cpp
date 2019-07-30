#define _HAS_EXCEPTIONS 0
#define _ITERATOR_DEBUG_LEVEL 0

#include "mc_preproc.h"

#include "file_utils.h"
#include "sdict.h"

static void GenerateJsonHeader(const char *prefix, sb_t *srcDir)
{
	sb_t data;
	sb_init(&data);
	sb_t *s = &data;

	sb_append(s, "// Copyright (c) 2012-2019 Matt Campbell\n");
	sb_append(s, "// MIT license (see License.txt)\n");
	sb_append(s, "\n");
	sb_append(s, "// AUTOGENERATED FILE - DO NOT EDIT\n");
	sb_append(s, "\n");
	sb_append(s, "// clang-format off\n");
	sb_append(s, "\n");
	sb_append(s, "#pragma once\n");
	sb_append(s, "\n");
	sb_append(s, "#include \"parson/parson.h\"\n");
	sb_append(s, "\n");
	sb_append(s, "#if defined(__cplusplus)\n");
	sb_append(s, "extern \"C\" {\n");
	sb_append(s, "#endif\n");
	sb_append(s, "\n");
	for(const struct_s &o : g_structs) {
		if(o.typedefBaseName.empty()) {
			sb_va(s, "struct %s;\n", o.name.c_str());
		} else {
			sb_va(s, "typedef struct %s %s;\n", o.typedefBaseName.c_str(), o.name.c_str());
		}
	}
	sb_append(s, "\n");
	for(const enum_s &o : g_enums) {
		if(o.typedefBaseName.empty()) {
			sb_va(s, "enum %s;\n", o.name.c_str());
		} else {
			sb_va(s, "typedef enum %s %s;\n", o.typedefBaseName.c_str(), o.name.c_str());
		}
	}
	sb_append(s, "\n");
	for(const struct_s &o : g_structs) {
		sb_va(s, "%s json_deserialize_%s(JSON_Value *src);\n", o.name.c_str(), o.name.c_str());
	}
	sb_append(s, "\n");
	for(const struct_s &o : g_structs) {
		sb_va(s, "JSON_Value *json_serialize_%s(const %s *src);\n", o.name.c_str(), o.name.c_str());
	}
	sb_append(s, "\n");
	sb_append(s, "\n");
	for(const enum_s &o : g_enums) {
		sb_va(s, "%s json_deserialize_%s(JSON_Value *src);\n", o.name.c_str(), o.name.c_str());
	}
	sb_append(s, "\n");
	for(const enum_s &o : g_enums) {
		sb_va(s, "JSON_Value *json_serialize_%s(const %s src);\n", o.name.c_str(), o.name.c_str());
	}
	sb_append(s, "#if defined(__cplusplus)\n");
	sb_append(s, "} // extern \"C\"\n");
	sb_append(s, "#endif\n");

	sb_replace_all_inplace(s, "\n", "\r\n");

	sb_t path;
	sb_init(&path);
	sb_va(&path, "%s\\%sjson_generated.h", sb_get(srcDir), prefix);
	if(fileData_writeIfChanged(sb_get(&path), NULL, { data.data, sb_len(s) })) {
		BB_LOG("preproc", "updated %s", sb_get(&path));
	}
	sb_reset(&path);
	sb_reset(&data);
}

enum memberJsonType_e {
	kMemberJsonObject,
	kMemberJsonBoolean,
	kMemberJsonNumber,
	kMemberJsonEnum,
};

static memberJsonType_e ClassifyMemberJson(const struct_member_s &m)
{
	if(m.typeStr == "bool" || m.typeStr == "b32" || m.typeStr == "b8") {
		return kMemberJsonBoolean;
	}
	for(const struct_s &s : g_structs) {
		if(s.name == m.typeStr) {
			return kMemberJsonObject;
		}
	}
	for(const enum_s &e : g_enums) {
		if(e.name == m.typeStr) {
			return kMemberJsonEnum;
		}
	}
	return kMemberJsonNumber;
}

static void GenerateJsonSource(const char *prefix, sb_t *srcDir)
{
	sb_t data;
	sb_init(&data);
	sb_t *s = &data;

	sb_append(s, "// Copyright (c) 2012-2019 Matt Campbell\n");
	sb_append(s, "// MIT license (see License.txt)\n");
	sb_append(s, "\n");
	sb_append(s, "// AUTOGENERATED FILE - DO NOT EDIT\n");
	sb_append(s, "\n");
	sb_append(s, "// clang-format off\n");
	sb_append(s, "\n");
	sb_va(s, "#include \"%sjson_generated.h\"\n", prefix);
	sb_append(s, "#include \"bb_array.h\"\n");
	sb_append(s, "#include \"json_utils.h\"\n");
	sb_append(s, "#include \"va.h\"\n");
	sb_append(s, "\n");
	for(const std::string &str : g_paths) {
		sb_va(s, "#include \"%s\"\n", str.c_str());
	}
	sb_append(s, "\n");
	sb_append(s, "//////////////////////////////////////////////////////////////////////////\n");
	sb_append(s, "\n");
	for(const struct_s &o : g_structs) {
		if(o.headerOnly)
			continue;
		const struct_member_s *m_count = nullptr;
		const struct_member_s *m_allocated = nullptr;
		const struct_member_s *m_data = nullptr;
		for(const struct_member_s &m : o.members) {
			if(m.name == "count") {
				m_count = &m;
			} else if(m.name == "allocated") {
				m_allocated = &m;
			} else if(m.name == "data") {
				m_data = &m;
			}
		}
		if(o.members.size() == 3 && m_count && m_allocated && m_data) {
			sb_va(s, "%s json_deserialize_%s(JSON_Value *src)\n", o.name.c_str(), o.name.c_str());
			sb_append(s, "{\n");
			sb_va(s, "\t%s dst;\n", o.name.c_str());
			sb_append(s, "\tmemset(&dst, 0, sizeof(dst));\n");
			sb_append(s, "\tif(src) {\n");
			sb_append(s, "\t\tJSON_Array *arr = json_value_get_array(src);\n");
			sb_append(s, "\t\tif(arr) {\n");
			sb_append(s, "\t\t\tfor(u32 i = 0; i < json_array_get_count(arr); ++i) {\n");
			sb_va(s, "\t\t\t\tbba_push(dst, json_deserialize_%.*s(json_array_get_value(arr, i)));\n", m_data->typeStr.length() - 1, m_data->typeStr.c_str());
			sb_append(s, "\t\t\t}\n");
			sb_append(s, "\t\t}\n");
			sb_append(s, "\t}\n");
			sb_append(s, "\treturn dst;\n");
			sb_append(s, "}\n");
			sb_append(s, "\n");
			continue;
		}
		sb_va(s, "%s json_deserialize_%s(JSON_Value *src)\n", o.name.c_str(), o.name.c_str());
		sb_append(s, "{\n");
		sb_va(s, "\t%s dst;\n", o.name.c_str());
		sb_append(s, "\tmemset(&dst, 0, sizeof(dst));\n");
		sb_append(s, "\tif(src) {\n");
		sb_append(s, "\t\tJSON_Object *obj = json_value_get_object(src);\n");
		sb_append(s, "\t\tif(obj) {\n");
		for(const struct_member_s &m : o.members) {
			if(m.arr.empty()) {
				switch(ClassifyMemberJson(m)) {
				case kMemberJsonObject:
					sb_va(s, "\t\t\tdst.%s = json_deserialize_%s(json_object_get_value(obj, \"%s\"));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonEnum:
					sb_va(s, "\t\t\tdst.%s = json_deserialize_%s(json_object_get_value(obj, \"%s\"));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonNumber:
					sb_va(s, "\t\t\tdst.%s = (%s)json_object_get_number(obj, \"%s\");\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonBoolean:
					sb_va(s, "\t\t\tdst.%s = json_object_get_boolean_safe(obj, \"%s\");\n",
					      m.name.c_str(), m.name.c_str());
					break;
				}
			} else {
				sb_va(s, "\t\t\tfor(u32 i = 0; i < BB_ARRAYSIZE(dst.%s); ++i) {\n", m.name.c_str());
				switch(ClassifyMemberJson(m)) {
				case kMemberJsonObject:
					sb_va(s, "\t\t\t\tdst.%s[i] = json_deserialize_%s(json_object_get_value(obj, va(\"%s.%%u\", i)));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonEnum:
					sb_va(s, "\t\t\t\tdst.%s[i] = json_deserialize_%s(json_object_get_value(obj, va(\"%s.%%u\", i)));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonNumber:
					sb_va(s, "\t\t\t\tdst.%s[i] = (%s)json_object_get_number(obj, va(\"%s.%%u\", i));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonBoolean:
					sb_va(s, "\t\t\t\tdst.%s[i] = json_object_get_boolean_safe(obj, va(\"%s.%%u\", i));\n",
					      m.name.c_str(), m.name.c_str());
					break;
				}
				sb_append(s, "\t\t\t}\n");
			}
		}
		sb_append(s, "\t\t}\n");
		sb_append(s, "\t}\n");
		sb_append(s, "\treturn dst;\n");
		sb_append(s, "}\n");
		sb_append(s, "\n");
	}
	sb_append(s, "//////////////////////////////////////////////////////////////////////////\n");
	sb_append(s, "\n");
	for(const struct_s &o : g_structs) {
		if(o.headerOnly)
			continue;
		const struct_member_s *m_count = nullptr;
		const struct_member_s *m_allocated = nullptr;
		const struct_member_s *m_data = nullptr;
		for(const struct_member_s &m : o.members) {
			if(m.name == "count") {
				m_count = &m;
			} else if(m.name == "allocated") {
				m_allocated = &m;
			} else if(m.name == "data") {
				m_data = &m;
			}
		}
		if(o.members.size() == 3 && m_count && m_allocated && m_data) {
			sb_va(s, "JSON_Value *json_serialize_%s(const %s *src)\n", o.name.c_str(), o.name.c_str());
			sb_append(s, "{\n");
			sb_append(s, "\tJSON_Value *val = json_value_init_array();\n");
			sb_append(s, "\tJSON_Array *arr = json_value_get_array(val);\n");
			sb_append(s, "\tif(arr) {\n");
			sb_append(s, "\t\tfor(u32 i = 0; i < src->count; ++i) {\n");
			sb_va(s, "\t\t\tJSON_Value *child = json_serialize_%.*s(src->data + i);\n", m_data->typeStr.length() - 1, m_data->typeStr.c_str());
			sb_append(s, "\t\t\tif(child) {\n");
			sb_append(s, "\t\t\t\tjson_array_append_value(arr, child);\n");
			sb_append(s, "\t\t\t}\n");
			sb_append(s, "\t\t}\n");
			sb_append(s, "\t}\n");
			sb_append(s, "\treturn val;\n");
			sb_append(s, "}\n");
			sb_append(s, "\n");
			continue;
		}
		sb_va(s, "JSON_Value *json_serialize_%s(const %s *src)\n", o.name.c_str(), o.name.c_str());
		sb_append(s, "{\n");
		sb_append(s, "\tJSON_Value *val = json_value_init_object();\n");
		sb_append(s, "\tJSON_Object *obj = json_value_get_object(val);\n");
		sb_append(s, "\tif(obj) {\n");
		for(const struct_member_s &m : o.members) {
			if(m.arr.empty()) {
				switch(ClassifyMemberJson(m)) {
				case kMemberJsonObject:
					sb_va(s, "\t\tjson_object_set_value(obj, \"%s\", json_serialize_%s(&src->%s));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonEnum:
					sb_va(s, "\t\tjson_object_set_value(obj, \"%s\", json_serialize_%s(src->%s));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonNumber:
					sb_va(s, "\t\tjson_object_set_number(obj, \"%s\", src->%s);\n",
					      m.name.c_str(), m.name.c_str());
					break;
				case kMemberJsonBoolean:
					sb_va(s, "\t\tjson_object_set_boolean(obj, \"%s\", src->%s);\n",
					      m.name.c_str(), m.name.c_str());
					break;
				}
			} else {
				sb_va(s, "\t\tfor(u32 i = 0; i < BB_ARRAYSIZE(src->%s); ++i) {\n", m.name.c_str());
				switch(ClassifyMemberJson(m)) {
				case kMemberJsonObject:
					sb_va(s, "\t\t\tjson_object_set_value(obj, va(\"%s.%%u\", i), json_serialize_%s(&src->%s[i]));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonEnum:
					sb_va(s, "\t\t\tjson_object_set_value(obj, va(\"%s.%%u\", i), json_serialize_%s(src->%s[i]));\n",
					      m.name.c_str(), m.typeStr.c_str(), m.name.c_str());
					break;
				case kMemberJsonNumber:
					sb_va(s, "\t\t\tjson_object_set_number(obj, va(\"%s.%%u\", i), src->%s[i]);\n",
					      m.name.c_str(), m.name.c_str());
					break;
				case kMemberJsonBoolean:
					sb_va(s, "\t\t\tjson_object_set_boolean(obj, va(\"%s.%%u\", i), src->%s[i]);\n",
					      m.name.c_str(), m.name.c_str());
					break;
				}
				sb_append(s, "\t\t}\n");
			}
		}
		sb_append(s, "\t}\n");
		sb_append(s, "\treturn val;\n");
		sb_append(s, "}\n");
		sb_append(s, "\n");
	}
	sb_append(s, "//////////////////////////////////////////////////////////////////////////\n");
	sb_append(s, "\n");
	for(const enum_s &o : g_enums) {
		sb_va(s, "%s json_deserialize_%s(JSON_Value *src)\n", o.name.c_str(), o.name.c_str());
		sb_append(s, "{\n");
		sb_va(s, "\t%s dst = %s;\n", o.name.c_str(), o.defaultVal.c_str());
		sb_append(s, "\tif(src) {\n");
		sb_append(s, "\t\tconst char *str = json_value_get_string(src);\n");
		sb_append(s, "\t\tif(str) {\n");
		for(const enum_member_s &m : o.members) {
			sb_va(s, "\t\t\tif(!strcmp(str, \"%s\")) { dst = %s; }\n", m.name.c_str(), m.name.c_str());
		}
		sb_append(s, "\t\t}\n");
		sb_append(s, "\t}\n");
		sb_append(s, "\treturn dst;\n");
		sb_append(s, "}\n");
		sb_append(s, "\n");
	}
	sb_append(s, "//////////////////////////////////////////////////////////////////////////\n");
	sb_append(s, "\n");
	for(const enum_s &o : g_enums) {
		sb_va(s, "JSON_Value *json_serialize_%s(const %s src)\n", o.name.c_str(), o.name.c_str());
		sb_append(s, "{\n");
		sb_append(s, "\tconst char *str = \"\";\n");
		sb_append(s, "\tswitch(src) {\n");
		for(const enum_member_s &m : o.members) {
			sb_va(s, "\t\tcase %s: str = \"%s\"; break;\n", m.name.c_str(), m.name.c_str());
		}
		sb_append(s, "\t}\n");
		sb_append(s, "\tJSON_Value *val = json_value_init_string(str);\n");
		sb_append(s, "\treturn val;\n");
		sb_append(s, "}\n");
		sb_append(s, "\n");
	}
	sb_append(s, "//////////////////////////////////////////////////////////////////////////\n");

	sb_replace_all_inplace(s, "\n", "\r\n");

	sb_t path;
	sb_init(&path);
	sb_va(&path, "%s\\%sjson_generated.c", sb_get(srcDir), prefix);
	if(fileData_writeIfChanged(sb_get(&path), NULL, { data.data, sb_len(s) })) {
		BB_LOG("preproc", "updated %s", sb_get(&path));
	}
	sb_reset(&path);
	sb_reset(&data);
}

void GenerateJson(const char *prefix, sb_t *srcDir, sb_t *includeDir)
{
	GenerateJsonHeader(prefix, includeDir);
	GenerateJsonSource(prefix, srcDir);
}
