// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "sdict.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tag_messageBox messageBox;
typedef void(messageBoxFunc)(messageBox *mb, const char *action);

typedef struct tag_messageBox {
	sdict_t data;
	messageBoxFunc *callback;
} messageBox;

typedef struct tag_messageBoxes {
	u32 count;
	u32 allocated;
	messageBox *data;
} messageBoxes;

void mb_queue(messageBox mb, messageBoxes* boxes);
messageBox *mb_get_active(messageBoxes *boxes);
void mb_remove_active(messageBoxes *boxes);
void mb_shutdown(messageBoxes *boxes);

#if defined(__cplusplus)
}
#endif
