// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#pragma once

#include "parson/parson.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sb_s sb_t;
typedef struct sbs_s sbs_t;
typedef struct sdictEntry_s sdictEntry_t;
typedef struct sdict_s sdict_t;


sb_t json_deserialize_sb_t(JSON_Value *src);
sbs_t json_deserialize_sbs_t(JSON_Value *src);
sdictEntry_t json_deserialize_sdictEntry_t(JSON_Value *src);
sdict_t json_deserialize_sdict_t(JSON_Value *src);

JSON_Value *json_serialize_sb_t(const sb_t *src);
JSON_Value *json_serialize_sbs_t(const sbs_t *src);
JSON_Value *json_serialize_sdictEntry_t(const sdictEntry_t *src);
JSON_Value *json_serialize_sdict_t(const sdict_t *src);



#if defined(__cplusplus)
} // extern "C"
#endif