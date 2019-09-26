// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "mc_preproc_config.h"
#include "bb_array.h"
#include "json_utils.h"
#include "parson/parson.h"

preprocInputDir json_deserialize_preprocInputDir(JSON_Value *src)
{
	preprocInputDir dst = { BB_EMPTY_INITIALIZER };
	JSON_Object *obj = src ? json_value_get_object(src) : NULL;
	if(obj) {
		dst.dir = json_deserialize_sb_t(json_object_get_value(obj, "dir"));
		dst.base = json_deserialize_sb_t(json_object_get_value(obj, "base"));
	}
	return dst;
}

preprocInputDirs json_deserialize_preprocInputDirs(JSON_Value *src)
{
	preprocInputDirs dst = { BB_EMPTY_INITIALIZER };
	JSON_Array *arr = src ? json_value_get_array(src) : NULL;
	if(arr) {
		for(u32 i = 0; i < json_array_get_count(arr); ++i) {
			bba_push(dst, json_deserialize_preprocInputDir(json_array_get_value(arr, i)));
		}
	}
	return dst;
}

preprocInputConfig json_deserialize_preprocInputConfig(JSON_Value *src)
{
	preprocInputConfig dst = { BB_EMPTY_INITIALIZER };
	JSON_Object *obj = src ? json_value_get_object(src) : NULL;
	if(obj) {
		dst.checkFonts = json_object_get_boolean_safe(obj, "checkFonts");
		dst.sourceDirs = json_deserialize_preprocInputDirs(json_object_get_value(obj, "sourceDirs"));
		dst.includeDirs = json_deserialize_preprocInputDirs(json_object_get_value(obj, "includeDirs"));
	}
	return dst;
}

preprocOutputConfig json_deserialize_preprocOutputConfig(JSON_Value *src)
{
	preprocOutputConfig dst = { BB_EMPTY_INITIALIZER };
	JSON_Object *obj = src ? json_value_get_object(src) : NULL;
	if(obj) {
		dst.prefix = json_deserialize_sb_t(json_object_get_value(obj, "prefix"));
		dst.sourceDir = json_deserialize_sb_t(json_object_get_value(obj, "sourceDir"));
		dst.includeDir = json_deserialize_sb_t(json_object_get_value(obj, "includeDir"));
		dst.baseDir = json_deserialize_sb_t(json_object_get_value(obj, "baseDir"));
	}
	return dst;
}

preprocConfig json_deserialize_preprocConfig(JSON_Value *src)
{
	preprocConfig dst = { BB_EMPTY_INITIALIZER };
	JSON_Object *obj = src ? json_value_get_object(src) : NULL;
	if(obj) {
		dst.bb = json_object_get_boolean_safe(obj, "bb");
		dst.input = json_deserialize_preprocInputConfig(json_object_get_value(obj, "input"));
		dst.output = json_deserialize_preprocOutputConfig(json_object_get_value(obj, "output"));
	}
	return dst;
}

void reset_preprocInputDir(preprocInputDir *val)
{
	if(val) {
		sb_reset(&val->dir);
		sb_reset(&val->base);
	}
}

void reset_preprocInputDirs(preprocInputDirs *val)
{
	if(val) {
		for(u32 i = 0; i < val->count; ++i) {
			reset_preprocInputDir(val->data + i);
		}
		bba_free(*val);
	}
}

void reset_preprocInputConfig(preprocInputConfig *val)
{
	if(val) {
		reset_preprocInputDirs(&val->sourceDirs);
		reset_preprocInputDirs(&val->includeDirs);
	}
}

void reset_preprocOutputConfig(preprocOutputConfig *val)
{
	if(val) {
		sb_reset(&val->prefix);
		sb_reset(&val->sourceDir);
		sb_reset(&val->includeDir);
		sb_reset(&val->baseDir);
	}
}

void reset_preprocConfig(preprocConfig *val)
{
	if(val) {
		reset_preprocInputConfig(&val->input);
		reset_preprocOutputConfig(&val->output);
	}
}

preprocConfig read_preprocConfig(const char *path)
{
	preprocConfig dst = { BB_EMPTY_INITIALIZER };
	JSON_Value *val = json_parse_file(path);
	if(val) {
		dst = json_deserialize_preprocConfig(val);
		json_value_free(val);
	}
	return dst;
}
