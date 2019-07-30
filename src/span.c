// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "span.h"
#include "bb_string.h"
#include <string.h>

span_t span_from_string(const char *str)
{
	span_t ret;
	ret.start = ret.end = str;
	if(str) {
		while(*ret.end) {
			++ret.end;
		}
	}
	return ret;
}

int span_strcmp(span_t a, span_t b)
{
	size_t alen = a.end - a.start;
	size_t blen = b.end - b.start;
	size_t len = (alen < blen) ? alen : blen;
	int cmp = strncmp(a.start, b.start, len);
	if(cmp) {
		return cmp;
	}
	if(alen == blen) {
		return 0;
	} else {
		return blen > alen;
	}
}

int span_stricmp(span_t a, span_t b)
{
	size_t alen = a.end - a.start;
	size_t blen = b.end - b.start;
	size_t len = (alen < blen) ? alen : blen;
	int cmp = bb_strnicmp(a.start, b.start, len);
	if(cmp) {
		return cmp;
	}
	if(alen == blen) {
		return 0;
	} else {
		return blen > alen;
	}
}
