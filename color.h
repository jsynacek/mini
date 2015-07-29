#pragma once
/*
 * Copyright 2015 Jan Synáček
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or
 * (at your option) any later version.
 */

#include <ncurses.h>

enum color_id {
	/* Solarized palette. */
	COLOR_ID_BASE03 = 15,
	COLOR_ID_BASE02,
	COLOR_ID_BASE01,
	COLOR_ID_BASE00,
	COLOR_ID_BASE0,
	COLOR_ID_BASE1,
	COLOR_ID_BASE2,
	COLOR_ID_BASE3,
	COLOR_ID_YELLOW,
	COLOR_ID_ORANGE,
	COLOR_ID_RED,
	COLOR_ID_MAGENTA,
	COLOR_ID_VIOLET,
	COLOR_ID_BLUE,
	COLOR_ID_CYAN,
	COLOR_ID_GREEN,
};
struct color {
	int id;
	int r;
	int g;
	int b;
};

enum color_pair_id {
	CP_NORMAL_TEXT,
	CP_ERROR,
	CP_HIGHLIGHT_SELECTION,
	CP_MODE_COMMAND,
	CP_MODE_EDITING,
	CP_MODE_SELECTION,
};
struct color_pair {
	int id;
	int fg;
	int bg;
};

static inline short rgb2color(short c) { return 1000.0 * c / 0xff; }
void init_colors(const struct color *colors, const struct color_pair *color_pairs);
