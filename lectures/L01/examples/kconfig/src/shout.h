/*
 * SPDX-License-Identifier: MIT
 */

#ifndef SHOUT_H_
#define SHOUT_H_

#include <stddef.h>

/**
 * @brief Copy a string into a buffer in capitals.
 *
 * @param in   The string to copy.
 * @param out  The buffer to copy it into. Always NUL-terminated.
 * @param size The size of the buffer.
 */
void shout(const char *in, char *out, size_t size);

#endif /* SHOUT_H_ */
