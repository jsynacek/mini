#pragma once
/*
 * Copyright 2015 Jan Synáček
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or
 * (at your option) any later version.
 */

#include <stdbool.h>

static inline bool is_utf8(char c) { return (c & 0xc0) != 0x80; }

int unicode_to_utf8(unsigned int codepoint, unsigned char *utf8);
