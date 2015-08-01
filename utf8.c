/*
 * Copyright 2015 Jan Synáček
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or
 * (at your option) any later version.
 */

#include "utf8.h"

/* Convert a unicode codepoint into a UTF-8 byte sequence.
 * Return number of converted bytes or -1 if there are any errors. */
int unicode_to_utf8(unsigned int codepoint, unsigned char *utf8)
{
	const unsigned int masks[] = {
		0xc080,    /* len = 2 */
		0xe08080,  /* len = 3 */
		0xf0808080 /* len = 4 */
	};
	unsigned int mask;
	int i, b, len;

	if (codepoint < 0 || codepoint > 0x1fffff)
		return -1;
	if (codepoint < 0x80) {
		*utf8 = codepoint;
		return 1;
	}

	len = (codepoint < 0x0800) ? 2 :
	      (codepoint < 0x10000) ? 3 :
	      4;
	i = 0;
	b = len - 1;
	mask = masks[len - 2];
	while (i < len) {
		utf8[i] = (mask >> (b*8)) | ((codepoint >> (b*6) & 0x3f));
		i++;
		b--;
	}

	return len;
}
