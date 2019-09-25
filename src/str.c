// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "str.h"

#include <stdlib.h>

u32 strtou32(const char *s)
{
	return atoi(s);
}

s32 strtos32(const char *s)
{
	return atoi(s);
}

// adapted from http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)HashTables.html
u64 strsimplehash(const char *s)
{
	u64 h = 0;
	for(unsigned const char *us = (unsigned const char *)s; *us; ++us) {
		h = h * 97 + *us;
	}
	return h;
}
