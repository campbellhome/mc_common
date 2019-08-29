// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "sb.h"
#include "bb_array.h"
#include "bb_wrap_stdio.h"
#include "parson/parson.h"
#include <string.h>

void sb_init(sb_t *sb)
{
	sb->count = sb->allocated = 0;
	sb->data = NULL;
}

void sb_reset_from_loc(const char *file, int line, sb_t *sb)
{
	bba_free_from_loc(file, line, *sb);
}

sb_t sb_from_c_string_from_loc(const char *file, int line, const char *src)
{
	sb_t dst = { BB_EMPTY_INITIALIZER };
	sb_append_from_loc(file, line, &dst, src);
	return dst;
}

sb_t sb_clone_from_loc(const char *file, int line, const sb_t *src)
{
	sb_t dst = { BB_EMPTY_INITIALIZER };
	sb_append_from_loc(file, line, &dst, sb_get(src));
	return dst;
}

u32 sb_len(sb_t *sb)
{
	return sb->count ? sb->count - 1 : 0;
}

b32 sb_reserve_from_loc(const char *file, int line, sb_t *sb, u32 len)
{
	if(sb->allocated < len && len > 0) {
		sb_t tmp = { BB_EMPTY_INITIALIZER };
		bba_add_noclear_from_loc(file, line, tmp, len);
		if(tmp.data) {
			if(sb->count) {
				memcpy(tmp.data, sb->data, (u64)sb->count + 1);
			} else {
				*tmp.data = '\0';
			}
			tmp.count = sb->count;
			sb_reset_from_loc(file, line, sb);
			*sb = tmp;
			return true;
		}
		return false;
	}
	return true;
}

b32 sb_grow_from_loc(const char *file, int line, sb_t *sb, u32 len)
{
	u32 originalCount = sb->count;
	u32 addedCount = (originalCount) ? len : len + 1;
	u32 desiredCount = originalCount + addedCount;
	bba_add_noclear_from_loc(file, line, *sb, addedCount);
	if(sb->data && sb->count == desiredCount) {
		sb->data[sb->count - 1] = '\0';
		return true;
	}
	return false;
}

void sb_move_from_loc(const char *file, int line, sb_t *target, sb_t *src)
{
	sb_reset_from_loc(file, line, target);
	*target = *src;
	memset(src, 0, sizeof(*src));
}

void sb_append_from_loc(const char *file, int line, sb_t *sb, const char *text)
{
	if(!text)
		return;
	u32 len = (u32)strlen(text);
	u32 originalCount = sb->count;
	u32 addedCount = (originalCount) ? len : len + 1;
	u32 desiredCount = originalCount + addedCount;
	bba_add_noclear_from_loc(file, line, *sb, addedCount);
	if(sb->data && sb->count == desiredCount) {
		memcpy(sb->data + (originalCount ? originalCount - 1 : 0), text, (u64)len + 1);
	}
}

void sb_append_range_from_loc(const char *file, int line, sb_t *sb, const char *start, const char *end)
{
	u32 len = (u32)(end - start);
	u32 originalCount = sb->count;
	u32 addedCount = (originalCount) ? len : len + 1;
	u32 desiredCount = originalCount + addedCount;
	bba_add_noclear_from_loc(file, line, *sb, addedCount);
	if(sb->data && sb->count == desiredCount) {
		memcpy(sb->data + (originalCount ? originalCount - 1 : 0), start, len);
		sb->data[sb->count - 1] = '\0';
	}
}

void sb_append_char_from_loc(const char *file, int line, sb_t *sb, char c)
{
	char text[2];
	text[0] = c;
	text[1] = '\0';
	sb_append_from_loc(file, line, sb, text);
}

const char *sb_get(const sb_t *sb)
{
	if(sb && sb->data) {
		return sb->data;
	}
	return "";
}

void sb_va_from_loc(const char *file, int line, sb_t *sb, const char *fmt, ...)
{
	u32 offset = sb_len(sb);
	int len;
	va_list args;
	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	if(len > 0) {
		if(sb_grow_from_loc(file, line, sb, len)) {
			va_start(args, fmt);
			vsnprintf(sb->data + offset, (u64)len + 1, fmt, args);
			va_end(args);
		}
	}
}

void sb_va_list_from_loc(const char *file, int line, sb_t *sb, const char *fmt, va_list args)
{
	u32 offset = sb_len(sb);
	int len;
	len = vsnprintf(NULL, 0, fmt, args);
	if(len > 0) {
		if(sb_grow_from_loc(file, line, sb, len)) {
			vsnprintf(sb->data + offset, (u64)len + 1, fmt, args);
		}
	}
}

sb_t sb_replace_all_from_loc(const char *file, int line, const sb_t *src, const char *replaceThis, const char *replacement)
{
	sb_t outData;
	sb_init(&outData);
	sb_t *out = &outData;

	if(src && replaceThis && replacement) {
		size_t replacementLen = strlen(replaceThis);
		const char *in = sb_get(src);
		while(*in) {
			char c = *in;
			if(strncmp(in, replaceThis, replacementLen)) {
				sb_append_char_from_loc(file, line, out, c);
			} else {
				sb_append_from_loc(file, line, out, replacement);
			}
			++in;
		}
	}

	return outData;
}

void sb_replace_all_inplace_from_loc(const char *file, int line, sb_t *src, const char *replaceThis, const char *replacement)
{
	if(src) {
		sb_t temp = sb_replace_all_from_loc(file, line, src, replaceThis, replacement);
		sb_reset(src);
		*src = temp;
	}
}

sbs_t sbs_clone_from_loc(const char *file, int line, const sbs_t *src)
{
	sbs_t dst = { BB_EMPTY_INITIALIZER };
	for(u32 i = 0; i < src->count; ++i) {
		sb_t *sb = bba_add_from_loc(file, line, dst, 1);
		if(sb) {
			*sb = sb_clone_from_loc(file, line, src->data + i);
		}
	}
	return dst;
}

void sbs_reset_from_loc(const char *file, int line, sbs_t *sbs)
{
	for(u32 i = 0; i < sbs->count; ++i) {
		sb_reset_from_loc(file, line, sbs->data + i);
	}
	bba_free_from_loc(file, line, *sbs);
}

JSON_Value *json_serialize_sb_t(const sb_t *src)
{
	JSON_Value *val = json_value_init_string(sb_get(src));
	return val;
}

sb_t json_deserialize_sb_t(JSON_Value *src)
{
	sb_t dst;
	sb_init(&dst);
	const char *str = json_value_get_string(src);
	if(str) {
		sb_append(&dst, str);
	}
	return dst;
}

JSON_Value *json_serialize_sbs_t(const sbs_t *src)
{
	JSON_Value *val = json_value_init_array();
	JSON_Array *arr = json_value_get_array(val);
	if(arr) {
		for(u32 i = 0; i < src->count; ++i) {
			JSON_Value *child = json_serialize_sb_t(src->data + i);
			if(child) {
				json_array_append_value(arr, child);
			}
		}
	}
	return val;
}

sbs_t json_deserialize_sbs_t(JSON_Value *src)
{
	sbs_t dst;
	memset(&dst, 0, sizeof(dst));
	if(src) {
		JSON_Array *arr = json_value_get_array(src);
		if(arr) {
			for(u32 i = 0; i < json_array_get_count(arr); ++i) {
				bba_push(dst, json_deserialize_sb_t(json_array_get_value(arr, i)));
			}
		}
	}
	return dst;
}
