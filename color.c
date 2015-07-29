/*
 * Copyright 2015 Jan Synáček
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or
 * (at your option) any later version.
 */

#include "color.h"

void init_colors(const struct color *colors, const struct color_pair *color_pairs)
{
	int i;

	if (!has_colors())
		return;

	start_color();
	use_default_colors();

	for (i = 0; colors[i].id >= 0; i++)
		init_color(colors[i].id,
			   rgb2color(colors[i].r),
			   rgb2color(colors[i].g),
			   rgb2color(colors[i].b));
	for (i = 0; color_pairs[i].id >= 0; i++)
		init_pair(color_pairs[i].id, color_pairs[i].fg, color_pairs[i].bg);

	/* Default window foreground and background. */
	assume_default_colors(COLOR_ID_BASE00, COLOR_ID_BASE3);
}
