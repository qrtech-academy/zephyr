/*
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include "shout.h"

void shout(const char *in, char *out, size_t size)
{
	size_t i = 0;

	for (; (i + 1 < size) && (in[i] != '\0'); i++) {
		out[i] = (char)toupper((unsigned char)in[i]);
	}
	out[i] = '\0';
}
