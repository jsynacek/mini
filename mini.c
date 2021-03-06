/*
 * Copyright (C) 2015 Jan Synáček
 *
 * Author: Jan Synáček <jan.synacek@gmail.com>
 * URL: https://github.com/jsynacek/mini
 * Created: Jun 2015
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <curses.h>
#include "mini.h"
#include "color.h"
#include "utf8.h"

static struct editor editor = {};
struct keybinding dvorak_keybindings[] = {
	{'n', M_COMMAND|M_SELECTION, command_move_forward_char},
	{'h', M_COMMAND|M_SELECTION, command_move_backward_char},
	{'r', M_COMMAND|M_SELECTION, command_move_forward_word},
	{'g', M_COMMAND|M_SELECTION, command_move_backward_word},
	{'t', M_COMMAND|M_SELECTION, command_move_forward_line},
	{'c', M_COMMAND|M_SELECTION, command_move_backward_line},
	{'H', M_COMMAND|M_SELECTION, command_move_beginning_of_line},
	{'N', M_COMMAND|M_SELECTION, command_move_end_of_line},
	{'C', M_COMMAND|M_SELECTION, command_move_page_up},
	{'T', M_COMMAND|M_SELECTION, command_move_page_down},
	{'f', M_COMMAND|M_SELECTION, command_move_beginning_of_buffer},
	{'d', M_COMMAND|M_SELECTION, command_move_end_of_buffer},
	{'R', M_COMMAND|M_SELECTION, command_move_forward_bracket},
	{'G', M_COMMAND|M_SELECTION, command_move_backward_bracket},
	{'b', M_COMMAND|M_SELECTION, command_goto_line},
	{CTRL('u'), M_COMMAND|M_EDITING, command_insert_unicode},
	{'o', M_COMMAND, command_open_below},
	{'O', M_COMMAND, command_open_above},
	{KEY_DC, M_ALL_BASIC, command_delete_forward_char},
	{'u', M_COMMAND, command_delete_forward_char},
	{KEY_BACKSPACE, M_ALL_BASIC, command_delete_backward_char},
	{'e', M_COMMAND, command_delete_backward_char},
	{'p', M_COMMAND, command_delete_forward_word},
	{'.', M_COMMAND, command_delete_backward_word},
	{'q', M_COMMAND|M_SELECTION, command_delete_selection_or_line},
	{'Q', M_COMMAND|M_EDITING, command_clear},
	{'k', M_COMMAND, command_paste},
	{'v', M_COMMAND|M_SELECTION, command_toggle_selection_mode},
	{KEY_ESC, M_ALL, command_editor_command_mode},
	{KEY_ENTER, M_COMMAND, command_editor_editing_mode},
	{CTRL('s'), M_ALL_BASIC, command_save_buffer},
	{CTRL('w'), M_ALL_BASIC, command_write_buffer},
	{CTRL('o'), M_ALL_BASIC, command_load_buffer},
	{CTRL('q'), M_ALL, command_editor_quit},
	{']', M_COMMAND|M_SELECTION, command_next_buffer},
	{'[', M_COMMAND|M_SELECTION, command_previous_buffer},
	{CTRL('l'), M_ALL_BASIC, command_recenter},
	/* Searching. */
	{'s', M_COMMAND, command_search_forward},
	{'S', M_COMMAND, command_search_backward},
	{'w', M_COMMAND, command_goto_next_search},
	{'W', M_COMMAND, command_goto_previous_search},
	/* Self insertion. */
	{KEY_ENTER, M_EDITING, command_insert_newline},
	{KEY_ANY, M_EDITING, command_insert_self},
	/* Minibuffer. */
	{KEY_ENTER, M_MINIBUFFER, command_minibuffer_do_action},
	{KEY_BACKSPACE, M_MINIBUFFER, command_minibuffer_delete_backward_char},
	{CTRL('x'), M_MINIBUFFER, command_minibuffer_clear},
	{KEY_ESC, M_MINIBUFFER, command_minibuffer_cancel},
	{KEY_ANY, M_MINIBUFFER, command_minibuffer_insert_self_and_update},
	{-1, -1, NULL}
};
/* XXX: Testing only */
struct keybinding qwerty_keybindings[] = {
	{'l', M_COMMAND|M_SELECTION, command_move_forward_char},
	{'j', M_COMMAND|M_SELECTION, command_move_backward_char},
	{'o', M_COMMAND|M_SELECTION, command_move_forward_word},
	{'u', M_COMMAND|M_SELECTION, command_move_backward_word},
	{'k', M_COMMAND|M_SELECTION, command_move_forward_line},
	{'i', M_COMMAND|M_SELECTION, command_move_backward_line},
	{'J', M_COMMAND|M_SELECTION, command_move_beginning_of_line},
	{'L', M_COMMAND|M_SELECTION, command_move_end_of_line},
	{'I', M_COMMAND|M_SELECTION, command_move_page_up},
	{'K', M_COMMAND|M_SELECTION, command_move_page_down},
	{'y', M_COMMAND|M_SELECTION, command_move_beginning_of_buffer},
	{'h', M_COMMAND|M_SELECTION, command_move_end_of_buffer},
	{'O', M_COMMAND|M_SELECTION, command_move_forward_bracket},
	{'U', M_COMMAND|M_SELECTION, command_move_backward_bracket},
	{'n', M_COMMAND|M_SELECTION, command_goto_line},
	{CTRL('u'), M_COMMAND|M_EDITING, command_insert_unicode},
	{'s', M_COMMAND, command_open_below},
	{'S', M_COMMAND, command_open_above},
	{KEY_DC, M_ALL_BASIC, command_delete_forward_char},
	{'f', M_COMMAND, command_delete_forward_char},
	{KEY_BACKSPACE, M_ALL_BASIC, command_delete_backward_char},
	{'d', M_COMMAND, command_delete_backward_char},
	{'r', M_COMMAND, command_delete_forward_word},
	{'e', M_COMMAND, command_delete_backward_word},
	{'x', M_COMMAND|M_SELECTION, command_delete_selection_or_line},
	{'X', M_COMMAND|M_EDITING, command_clear},
	{'v', M_COMMAND, command_paste},
	{'.', M_COMMAND|M_SELECTION, command_toggle_selection_mode},
	{KEY_ESC, M_ALL, command_editor_command_mode},
	{KEY_ENTER, M_COMMAND, command_editor_editing_mode},
	{CTRL('s'), M_ALL_BASIC, command_save_buffer},
	{CTRL('w'), M_ALL_BASIC, command_write_buffer},
	{CTRL('o'), M_ALL_BASIC, command_load_buffer},
	{CTRL('q'), M_ALL, command_editor_quit},
	{']', M_COMMAND|M_SELECTION, command_next_buffer},
	{'[', M_COMMAND|M_SELECTION, command_previous_buffer},
	{CTRL('l'), M_ALL_BASIC, command_recenter},
	/* Searching. */
	{';', M_COMMAND, command_search_forward},
	{':', M_COMMAND, command_search_backward},
	{',', M_COMMAND, command_goto_next_search},
	{'<', M_COMMAND, command_goto_previous_search},
	/* Self insertion. */
	{KEY_ENTER, M_EDITING, command_insert_newline},
	{KEY_ANY, M_EDITING, command_insert_self},
	/* Minibuffer. */
	{KEY_ENTER, M_MINIBUFFER, command_minibuffer_do_action},
	{KEY_BACKSPACE, M_MINIBUFFER, command_minibuffer_delete_backward_char},
	{CTRL('x'), M_MINIBUFFER, command_minibuffer_clear},
	{KEY_ESC, M_MINIBUFFER, command_minibuffer_cancel},
	{KEY_ANY, M_MINIBUFFER, command_minibuffer_insert_self_and_update},
	{-1, -1, NULL}
};

/* Solarized palette. */
struct color colors[] = {
	{COLOR_ID_BASE03,  0x00, 0x2b, 0x36},
	{COLOR_ID_BASE02,  0x07, 0x36, 0x42},
	{COLOR_ID_BASE01,  0x58, 0x6e, 0x75},
	{COLOR_ID_BASE00,  0x65, 0x7b, 0x83},
	{COLOR_ID_BASE0,   0x83, 0x94, 0x96},
	{COLOR_ID_BASE1,   0x93, 0xa1, 0xa1},
	{COLOR_ID_BASE2,   0xee, 0xe8, 0xd5},
	{COLOR_ID_BASE3,   0xfd, 0xf6, 0xe3},
	{COLOR_ID_YELLOW,  0xb5, 0x89, 0x00},
	{COLOR_ID_ORANGE,  0xcb, 0x4b, 0x16},
	{COLOR_ID_RED,     0xdc, 0x32, 0x2f},
	{COLOR_ID_MAGENTA, 0xd3, 0x36, 0x82},
	{COLOR_ID_VIOLET,  0x6c, 0x71, 0xc4},
	{COLOR_ID_BLUE,    0x26, 0x8b, 0xd2},
	{COLOR_ID_CYAN,    0x2a, 0xa1, 0x98},
	{COLOR_ID_GREEN,   0x85, 0x99, 0x00},
	{-1, -1, -1, -1}
};
struct color_pair color_pairs[] = {
	{CP_NORMAL_TEXT,         COLOR_ID_BASE00, -1},
	{CP_ERROR,               COLOR_ID_RED,    -1},
	{CP_HIGHLIGHT_SELECTION, COLOR_ID_BASE00,  COLOR_ID_BASE2},
	{CP_MODE_COMMAND,        COLOR_ID_BLUE,   -1},
	{CP_MODE_EDITING,        COLOR_ID_GREEN,  -1},
	{CP_MODE_SELECTION,      COLOR_ID_ORANGE, -1},
	{-1, -1, -1}
};

static void buffer_set_name(struct buffer *buf, const char *name)
{
	if (buf->name)
		free(buf->name);
	buf->name = strdup(name);
}

struct buffer *buffer_new(void)
{
	struct buffer *buf = NULL;

	buf = calloc(1, sizeof(struct buffer));
	if (!buf)
		oom();
	buf->size = BUFFER_ALLOC_CHUNK;
	buf->data = calloc(BUFFER_ALLOC_CHUNK, 1);
	if (!buf->data)
		oom();
	buf->gap_end = buf->size;
	buffer_set_name(buf, "*Untitled*");

	return buf;
}

int buffer_save(struct buffer *buf, const char *path)
{
	FILE *fp;

	fp = fopen(path, "w+");
	if (!fp)
		return -1;

	if (fwrite(buf->data, 1, buf->gap_start, fp) == 0)
		if (ferror(fp))
		    return -1;
	if (buf->gap_end != buf->size)
		if (fwrite(buf->data + buf->gap_end, 1, buf->size - buf->gap_end, fp) == 0)
			if (ferror(fp))
				return -1;
	fclose(fp);

	buffer_set_path(buf, path);
	buf->modified = false;

	return 0;

}

int buffer_load(struct buffer *buf, const char *path)
{
	struct stat sb;

	assert(buf);

	int rc = stat(path, &sb);
	if (rc < 0) {
		if (errno != ENOENT)
			return -errno;
	} else {
		FILE *fp;
		char *tmp_buf;
		size_t read;

		if (S_ISDIR(sb.st_mode)) {
			/* TODO: For now. */
			errno = EISDIR;
			return -errno;
		} else if (!S_ISREG(sb.st_mode)) {
			errno = ENOTSUP;
			return -errno;
		}

		/* At this point, fopen() should always succeed. */
		fp = fopen(path, "r");
		assert(fp);

		tmp_buf = malloc(BUFFER_ALLOC_CHUNK);
		assert(tmp_buf);
		while ((read = fread(tmp_buf, 1, BUFFER_ALLOC_CHUNK, fp)) > 0)
			buffer_insert_string(buf, tmp_buf, read);
		free(tmp_buf);
		fclose(fp);
	}
	buffer_set_path(buf, path);
	buf->modified = false;
	buf->cursor = 0;
	buf->cur_line = 0;

	return 0;
}

void buffer_set_path(struct buffer *buf, const char *path)
{
	char *name;

	if (buf->path)
		free(buf->path);
	buf->path = strdup(path);
	name = strrchr(path, '/');
	if (name)
		name += 1;
	else
		name = (char *)path;
	buffer_set_name(buf, name);
}

int cursor_to_data(struct buffer *buf, int pos)
{
	if (pos >= buf->gap_start)
		pos += buf->gap_end - buf->gap_start;
	return pos;
}

char buffer_data_at(struct buffer *buf, int pos)
{
	if (!is_position_in_buffer(pos, buf))
		return -1;
	return buf->data[cursor_to_data(buf, pos)];
}

void buffer_expand(struct buffer *buf, size_t chunk)
{
	int size = buf->size + chunk;

	buf->data = realloc(buf->data, size);
	assert(buf->data);
	memmove(buf->data + buf->gap_end + chunk,
		buf->data + buf->gap_end,
		buf->size - buf->gap_end);
	buf->size = size;
	buf->gap_end += chunk;
}

void buffer_adjust_gap(struct buffer *buf)
{
	int p, n;

	p = cursor_to_data(buf, buf->cursor);

	if (p == buf->gap_end) {
		buf->cursor = buf->gap_start;
		return;
	}

	if (p < buf->gap_start) {
		n = buf->gap_start - p;
		memmove(buf->data + buf->gap_end - n,
			buf->data + p,
			n);
		buf->gap_start = p;
		buf->gap_end -= n;
	} else {
		n = p - buf->gap_end;
		memmove(buf->data + buf->gap_start,
			buf->data + buf->gap_end,
			n);
		buf->gap_start += n;
		buf->gap_end = p;
		p = buf->gap_start;
	}

}

int buffer_get_next_newline(struct buffer *buf, int from, int way)
{
	int d = (way > 0) ? 1 : -1;
	char c;

	from += d;
	while (from >= 0 && from < buf->used) {
		c = buffer_data_at(buf, from);
		if (is_utf8(c) && c == '\n')
			return from;
		else
			from += d;
	}

	return -1;
}

int buffer_get_line_beginning(struct buffer *buf)
{
	int nl;

	nl = buffer_get_next_newline(buf, buf->cursor, -1);
	if (nl < 0)
		return 0;

	return nl + 1;
}

int buffer_get_line_end(struct buffer *buf)
{
	int nl;

	if (buf->used == 0)
		return 0;

	nl = buffer_get_next_newline(buf, buf->cursor - 1, 1);
	if (nl < 0)
		return buf->used;

	return nl;
}

/* Get cursor offset on the current line.
 * Number of characters is returned.
 * UTF-8 friendly.
 */
int buffer_get_line_offset(struct buffer *buf)
{
	int p, x = 0;
	char c;

	p = buffer_get_line_beginning(buf);

	while (p < buf->cursor) {
		c = buffer_data_at(buf, p);
		if (is_utf8(c)) {
			if (buffer_data_at(buf, p) == '\t')
				x += TAB_STOP - x % TAB_STOP;
			else
				x++;
		}
		p++;
	}
	return x;
}

int buffer_get_line_length(struct buffer *buf)
{
	int p, end, l = 0;

	p = buffer_get_line_beginning(buf);
	end = buffer_get_line_end(buf);
	while (p < end) {
		if (is_utf8(buffer_data_at(buf, p)))
			l++;
		p++;
	}

	return l;
}

/* Get buffer region between lines, including the lines.
 * Start at "line_start", end at "line_start" + "lines".
 * Beginning of the region is output in "beg", end of the region in "end".
 * Both output positions are in cursor (user) coordinates.
 */
void buffer_get_region(struct buffer *buf, int line_start, int lines, int *beg, int *end)
{
	int p, l = 0;

	/* TODO: move into buffer_goto_line() or similar */
	for (p = 0; p < buf->used && l != line_start; p++) {
		if (buffer_data_at(buf, p) == '\n')
			l++;
	}
	*beg = p;
	for (l = 0; p < buf->used && l < lines; p++) {
		if (buffer_data_at(buf, p) == '\n')
			l++;
	}
	*end = p - 1;
}

void buffer_get_yx(struct buffer *buf, int *y, int *x)
{
	*y = buf->cur_line;
	*x = buffer_get_line_offset(buf);
}

char *buffer_get_content(struct buffer *buf)
{
	char *text;

	text = malloc(buf->used + 1);
	if (!text)
		oom();
	memcpy(text, buf->data, buf->gap_start);
	memcpy(text + buf->gap_start, buf->data + buf->gap_end, buf->size - buf->gap_end);
	text[buf->used] = '\0';

	return text;
}

/* TODO: util functions needed for buffer operations: */
/* strstr() and reverse */

/* TODO: buffer_find_char() and friends could be used to find newlines in text as well. */

/* Find the next occurence of any of the characters specified in "accept".
 * Start at "from", go forward if "way" is positive or backward if negative.
 * Number of lines passed is returned in "newlines".
 * Return a position of the first found character, or -1 if not found.
 */
int buffer_find_char(struct buffer *buf, int from, int way, const char *accept, int *newlines)
{
	int p = from;

	if (!is_position_in_buffer(p, buf))
		return -1;

	*newlines = 0;
	while (is_position_in_buffer(p, buf)) {
		const char *a = accept;
		char c;

		c = buffer_data_at(buf, p);
		while (*a != '\0') {
			if (*a++ == c)
				return p;
		}
		if (c == '\n')
			(*newlines)++;
		p += way;
	}

	return -1;
}

int buffer_find_char_next(struct buffer *buf, int from, const char *accept, int *newlines)
{
	return buffer_find_char(buf, from, 1, accept, newlines);
}

int buffer_find_char_prev(struct buffer *buf, int from, const char *accept, int *newlines)
{
	return buffer_find_char(buf, from, -1, accept, newlines);
}

int buffer_find_str_next(struct buffer *buf, int from, const char *str, int *newlines)
{
	char *text, *start, *s;
	int p = -1;

	text = buffer_get_content(buf);
	start = text + from;
	s = strstr(start, str);
	*newlines = 0;
	if (s) {
		p = s - start;
		*newlines = str_newlines(start, p);
	}
	free(text);

	return p;
}
int buffer_find_str_prev(struct buffer *buf, int from, const char *str, int *newlines)
{
	char *text, *s;
	int p = -1;

	text = buffer_get_content(buf);

	*newlines = 0;
	/* TODO: If the stars ever align correctly, this mess should be simplified... */
	s = strstr(text, str);
	if (s) {
		while (1) {
			int len;

			s = strstr(text + p + 1, str);
			if (s) {
				len = s - (text + p);
				if (p + len < from)
					p += len;
				else
					break;
			} else {
				break;
			}
		}
		*newlines += str_newlines(text, p);

	}
	free(text);

	return p;
}

static void buffer_cursor_column_update(struct buffer *buf)
{
	buf->cursor_column = buffer_get_line_offset(buf);
}

void buffer_move_forward_char(struct buffer *buf)
{
	int p = buf->cursor + 1;

	if (p > buf->used)
		return;

	if (buffer_data_at(buf, buf->cursor) == '\n')
		buf->cur_line++;
	while (p < buf->used && !is_utf8(buffer_data_at(buf, p)))
		p++;
	buf->cursor = p;

	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_backward_char(struct buffer *buf)
{
	int p = buf->cursor - 1;

	if (p < 0)
		return;

	while (p > 0 && !is_utf8(buffer_data_at(buf, p)))
		p--;
	if (buffer_data_at(buf, p) == '\n')
			buf->cur_line--;
	buf->cursor = p;

	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_forward_word(struct buffer *buf)
{
}

void buffer_move_backward_word(struct buffer *buf)
{
}

void buffer_move_forward_line(struct buffer *buf)
{
	int ll, cc;

	cc = buf->cursor_column;

	buffer_move_end_of_line(buf);
	buffer_move_forward_char(buf);

	ll = buffer_get_line_length(buf);
	while (buffer_get_line_offset(buf) < cc
	       && buffer_get_line_offset(buf) < ll)
		buffer_move_forward_char(buf);

	buf->cursor_column = cc;
}

void buffer_move_backward_line(struct buffer *buf)
{
	int ll, cc;

	cc = buf->cursor_column;

	buffer_move_beginning_of_line(buf);
	buffer_move_backward_char(buf);
	buffer_move_beginning_of_line(buf);

	ll = buffer_get_line_length(buf);
	while (buffer_get_line_offset(buf) < cc
	       && buffer_get_line_offset(buf) < ll)
		buffer_move_forward_char(buf);

	buf->cursor_column = cc;
}

void buffer_move_beginning_of_line(struct buffer *buf)
{
	int p, lb;

	/* TODO: move the smartness somewhere else, perhaps to a separate command */
	/* Be smart about blanks */
	p = lb = buffer_get_line_beginning(buf);
	while (isblank(buffer_data_at(buf, p)))
		p++;
	/* if (buf->cursor == p) */
	buf->cursor = lb;
	/* else */
	/* 	buf->cursor = p; */
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_end_of_line(struct buffer *buf)
{
	buf->cursor = buffer_get_line_end(buf);
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_beginning_of_buffer(struct buffer *buf)
{
	buf->cursor = 0;
	buf->cur_line = 0;
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_end_of_buffer(struct buffer *buf)
{
	buf->cursor = buf->used;
	buf->cur_line = buf->last_line;
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_forward_bracket(struct buffer *buf)
{
	int p, nl;

	p = buffer_find_char_next(buf, buf->cursor + 1, "([{<", &nl);
	if (p >= 0) {
		if (buffer_data_at(buf, buf->cursor) == '\n')
			nl++;
		buf->cursor = p;
		buf->cur_line += nl;
	}
}

void buffer_move_backward_bracket(struct buffer *buf)
{
	int p, nl;

	p = buffer_find_char_prev(buf, buf->cursor - 1, ")]}>", &nl);
	if (p >= 0) {
		buf->cursor = p;
		buf->cur_line -= nl;
	}
}

void buffer_goto_line(struct buffer *buf, int line)
{
	int p = 0, nl = 0, l = line, lastp;

	if (line < 0)
		return;

	while (l >= 0) {
		int tmp;

		lastp = p;
		p = buffer_find_char_next(buf, p, "\n", &tmp);
		if (p < 0)
			break;
		else {
			buf->cursor = p;
			buf->cursor = buffer_get_line_beginning(buf);
			buf->cur_line = nl;
		}
		l--;
		nl++;
		p++;
	}
	if (p < 0 && line != 0) {
		buf->cursor = lastp;
		buf->cur_line = buf->last_line;
	}

	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_insert_char(struct buffer *buf, const char c)
{
	char str[] = {c};

	buffer_insert_string(buf, str, 1);
}

void buffer_insert_string(struct buffer *buf, const char *str, size_t len)
{
	int nl;

	if (!str || len <= 0)
		return;

	if (buf->used + len >= buf->size)
		buffer_expand(buf, BUFFER_ALLOC_CHUNK);

	buffer_adjust_gap(buf);
	memcpy(buf->data + buf->gap_start, str, len);
	nl = str_newlines(str, len);
	buf->gap_start += len;
	buf->used += len;
	buf->cursor += len;
	buf->cur_line += nl;
	buf->last_line += nl;
	buf->modified = true;
	buffer_cursor_column_update(buf);
}


void buffer_delete_forward_char(struct buffer *buf)
{
	int p;

	if (buf->cursor >= buf->used)
		return;

	p = buf->cursor;
	while (!is_utf8(buffer_data_at(buf, p + 1)) && (p + 1 < buf->used))
		    p++;

	buffer_delete_region(buf, buf->cursor, p, NULL, NULL);
}

void buffer_delete_backward_char(struct buffer *buf)
{
	int p;

	if (buf->cursor == 0)
		return;

	p = buf->cursor;
	while (!is_utf8(buffer_data_at(buf, p - 1)) && (p - 1 < buf->used))
		    p--;

	buffer_delete_region(buf, buf->cursor - 1, p - 1, NULL, NULL);
}

void buffer_delete_forward_word(struct buffer *buf, char **out, int *n_out)
{
}

void buffer_delete_backward_word(struct buffer *buf, char **out, int *n_out)
{
}

void buffer_delete_region(struct buffer *buf, int beg, int end, char **out, int *n_out)
{
	int n = end - beg + 1, from, nl, nl_to_cursor;

	if (beg < 0 && end < 0 ||
	    beg >= buf->used && end >= buf->used)
		return;

	/* Right now, this can happen during text selection */
	if (end < beg) {
		int tmp = beg;
		beg = end;
		end = tmp;
	}

	if (end >= buf->used)
		end = buf->used - 1;

	n = end - beg + 1;
	nl = region_newlines(buf, beg, end);
	nl_to_cursor = region_newlines(buf, beg, buf->cursor);
	if (buffer_data_at(buf, buf->cursor) == '\n')
		nl_to_cursor--;
	buf->cursor = beg;
	buffer_adjust_gap(buf);
	from = buf->gap_end;
	buf->gap_end += n;
	buf->used -= n;
	buf->cur_line -= nl_to_cursor;
	buf->last_line -= nl;
	buf->modified = true;
	buffer_cursor_column_update(buf);

	if (out) {
		*out = calloc(1, n);
		if (!*out)
			oom();
		memcpy(*out, buf->data + from, n);
	}
	if (n_out)
		*n_out = n;
}

void buffer_delete_line(struct buffer *buf, char **out, int *n_out)
{
	buffer_delete_region(buf, buffer_get_line_beginning(buf), buffer_get_line_end(buf), out, n_out);
}

void buffer_delete_selection(struct buffer *buf, char **out, int *n_out)
{
	if (buf->sel_active) {
		buffer_delete_region(buf, buf->sel_start, buf->sel_end, out, n_out);
		buf->sel_active = false;
	}
}

void buffer_clear(struct buffer *buf)
{
	buf->used = 0;
	buf->gap_start = 0;
	buf->gap_end = buf->size;
	buf->cursor = 0;
	buf->cur_line = 0;
	buf->last_line = 0;
}

void buffer_selection_toggle(struct buffer *buf)
{
	buf->sel_active = !buf->sel_active;

	if (buf->sel_active)
		buf->sel_start = buf->sel_end = buf->cursor;
}

/* TODO: must be fixed to keep the selection properly on UTF-8 characters */
void buffer_selection_update(struct buffer *buf)
{
	if (buf->sel_active)
		buf->sel_end = buf->cursor;
}

/* TODO: searches work from cursor; add a helper search function to utils that is more general */
int buffer_search_forward(struct buffer *buf, const char *str)
{
	char *text, *from, *s;
	int p = -1;

	text = malloc(buf->used + 1);
	assert(text);
	memcpy(text, buf->data, buf->gap_start);
	memcpy(text + buf->gap_start, buf->data + buf->gap_end, buf->size - buf->gap_end);
	text[buf->used] = '\0';
	from = text + buf->cursor;
	s = strstr(from, str);
	if (s) {
		p = s - from;
		buf->cursor += p;
		buf->cur_line += str_newlines(from, p);
	}
	free(text);
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);

	return p;
}

int buffer_search_backward(struct buffer *buf, const char *str)
{
	char *text, *s;
	int p = 0;

	text = malloc(buf->used + 1);
	assert(text);
	memcpy(text, buf->data, buf->gap_start);
	memcpy(text + buf->gap_start, buf->data + buf->gap_end, buf->size - buf->gap_end);
	text[buf->used] = '\0';

	/* TODO: If the stars ever align correctly, this mess should be simplified... */
	s = strstr(text, str);
	if (s) {
		while (1) {
			int len;

			s = strstr(text + p + 1, str);
			if (s) {
				len = s - (text + p);
				if (p + len < buf->cursor)
					p += len;
				else
					break;
			} else {
				break;
			}
		}
		buf->cursor = p;
		buf->cur_line = str_newlines(text, p);
	}
	free(text);
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);

	return p;
}

bool is_position_in_buffer(int pos, struct buffer *buf)
{
	return pos >= 0 && pos < buf->used;
}

/* Return true if position is in a buffer region.
 * Position "pos" is in cursor coordinates.
 */
bool is_position_in_region(int pos, int beg, int end)
{
	if (beg <= end)
		return pos >= beg && pos <= end;
	return pos >= end && pos <= beg;
}

/* Count newlines in a string.
 * String "str" doesn't have to be 0-terminated and is "n" characters in
 * length.
 */
int str_newlines(const char *str, int n)
{
	int l = 0, i = 0;

	while (i < n) {
		if (str[i] == '\n')
			l++;
		i++;
	}

	return l;
}

/* Count newlines in a buffer region.
 * Input must be a valid buffer region. */
int region_newlines(struct buffer *buf, int beg, int end)
{
	int nl = 0, p = beg, way;
	char c;

	way = beg < end ? 1 : -1;
	do {
		c = buffer_data_at(buf, p);
		if (is_utf8(c) && c == '\n')
			nl++;
		p += way;
	} while (is_position_in_region(p, beg, end));

	return nl;
}

void die(const char *fmt, ...)
{
	va_list va;

	/* In case curses are still active */
	endwin();

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");

	exit(1);
}

void oom()
{
	die("Out of mana!");
}

static void editor_create_buffer(const char *path)
{
	struct buffer *buf;

	buf = buffer_new();
	if (!buf)
		oom();
	editor_add_buffer(buf);
	editor.buf_current = buf;

	if (path && buffer_load(buf, path) < 0)
		die("Can't open '%s': %m", path);
}

void editor_init(int argc, char *argv[])
{
	struct buffer *buf = NULL;
	int i = 1;

	editor.buf_first = NULL;
	editor.buf_last = NULL;
	editor.buf_current = NULL;
	editor.mode = M_COMMAND;
	editor.screen_start = 0;
	editor.screen_width = getmaxy(stdscr) - 3;
	editor.clipboard_len = 0;
	editor.clipboard = NULL;
	editor.cursor_last = 0;
	editor.line_last = 0;
	editor.key_last = 0;
	editor.search_last = NULL;
	editor.search_dir = SEARCH_FORWARD;
	editor.keybindings = DEFAULT_KEYBINDINGS;

	if (argc == 1)
		editor_create_buffer(NULL);

	while (i < argc)
		editor_create_buffer(argv[i++]);

	/* Special-purpose minibuffer. Can't be accessed directly. */
	buf = buffer_new();
	if (!buf)
		oom();
	editor.minibuf.prompt = NULL;
	editor.minibuf.buf = buf;
	editor.minibuf.action_cb = NULL;
	editor.minibuf.update_cb = NULL;
	editor.minibuf.cancel_cb = NULL;
}

void editor_add_buffer(struct buffer *buf)
{
	if (!editor.buf_first) {
		editor.buf_first = buf;
		editor.buf_last = buf;
	} else {
		struct buffer *last = editor.buf_last;

		last->buf_next = buf;
		buf->buf_prev = last;
		editor.buf_last = buf;
	}
}

void editor_next_buffer(void)
{
	editor.buf_current = editor.buf_current->buf_next;
	if (!editor.buf_current)
		editor.buf_current = editor.buf_first;
}

void editor_previous_buffer(void)
{
	editor.buf_current = editor.buf_current->buf_prev;
	if (!editor.buf_current)
		editor.buf_current = editor.buf_last;
}

void editor_save(bool save_as)
{
	char *path;

	if (!editor.buf_current->path || save_as)
		path = editor_dialog("Save as: ");
	else
		path = strdup(editor.buf_current->path);
	if (buffer_save(editor.buf_current, path) < 0)
		editor_error("Failed to save file");

	free(path);
}

void editor_load_file(void)
{
	struct buffer *buf;

	buf = buffer_new();
	if (!buf)
		oom();

	if (buffer_load(buf, editor_dialog("Load file: ")) < 0) {
		editor_error("Failed to load file");
		free(buf); /* TODO: buffer_free() or something */
	} else {
		buffer_move_beginning_of_buffer(buf);
		editor_add_buffer(buf);
		editor.buf_current = buf;
	}
}

char *editor_dialog(const char *prompt)
{
	const int n = 1024;
	char *str;

	str = calloc(n, 1);
	assert(str);
	move(0, 0);
	deleteln();
	/* This insertln() call is needed when the dialog is shown at the top of the screen,
	   because deleteln() actually moves all lines up by one. */
	insertln();

	echo();
	attron(A_BOLD);
	mvprintw(0, 0, prompt);
	attroff(A_BOLD);
	getnstr(str, n - 1);
	noecho();

	return str;
}

void editor_error(const char *error)
{
	curs_set(0);
	move(0, 0);
	deleteln();
	/* This insertln() call is needed when the dialog is shown at the top of the screen,
	   because deleteln() actually moves all lines up by one. */
	insertln();

	attron(COLOR_PAIR(CP_ERROR));
	attron(A_BOLD);
	printw(error);
	attroff(A_BOLD);
	attroff(COLOR_PAIR(CP_ERROR));
	getch();
	curs_set(1);
}

void editor_show_status_line(void)
{
	struct buffer *buf = editor.buf_current;
	const char *mode;
	const char *modified = (buf->modified) ? "[+]" : "";
	int y, x;

	if (editor.mode == M_COMMAND) {
		mode = "[C]";
		attron(COLOR_PAIR(CP_MODE_COMMAND));
	} else if (editor.mode == M_EDITING) {
		mode = "[E]";
		attron(COLOR_PAIR(CP_MODE_EDITING));
	} else if (editor.mode == M_SELECTION) {
		attron(COLOR_PAIR(CP_MODE_SELECTION));
		mode = "[S]";
	} else {
		mode = "[M]";
	}

	move(1, 0);

	buffer_get_yx(buf, &y, &x);
	attron(A_BOLD);
	printw(mode);
	attroff(COLOR_PAIR(CP_MODE_SELECTION));
	attroff(COLOR_PAIR(CP_MODE_EDITING));
	attroff(COLOR_PAIR(CP_MODE_COMMAND));
	printw(" %s %s", buf->name, modified);

	char *s;
	if (asprintf(&s, "%d:%d (%d)", x, y, buf->used) < 0)
		oom();
	move(1, getmaxx(stdscr) - strlen(s));
	printw(s);
	free(s);
	attroff(A_BOLD);
}

void editor_update_screen(void)
{
	int cl = editor.buf_current->cur_line;
	int endl = editor.screen_start + editor.screen_width;

	if (cl >= endl)
		editor.screen_start += cl - endl + 1;
	else if (cl < editor.screen_start)
		editor.screen_start -= editor.screen_start - cl;
}

void editor_redisplay(void)
{
	struct buffer *buf = editor.buf_current;
	int sel_start, sel_end, display_start, display_end, pos;
	int y, x;
	char c;

	if (editor.mode & M_MINIBUFFER) {
		char *text;

		text = buffer_get_content(editor.minibuf.buf);
		move(0, 0);
		attron(A_BOLD);
		printw(editor.minibuf.prompt);
		attroff(A_BOLD);
		printw(text);
		free(text);
	}

	/* TODO: During selection, this should be automatically handled */
	if (buf->sel_active && buf->sel_start < buf->sel_end) {
		sel_start = buf->sel_start;
		sel_end = buf->sel_end;
	} else {
		sel_start = buf->sel_end;
		sel_end = buf->sel_start;
	}

	move(2, 0);

	buffer_get_region(buf, editor.screen_start, editor.screen_width, &display_start, &display_end);

	for (pos = display_start; pos <= display_end; pos++) {
		bool is_tab = false;

		c = buffer_data_at(buf, pos);

		if (buf->sel_active && pos >= sel_start && pos <= sel_end)
			attron(COLOR_PAIR(CP_HIGHLIGHT_SELECTION));
		if (c == '\t') {
			attron(A_BOLD);
			is_tab = true;
		}
		printw("%c", c);
		if (is_tab)
			attroff(A_BOLD);
		if (buf->sel_active && pos >= sel_start && pos <= sel_end)
			attroff(COLOR_PAIR(CP_HIGHLIGHT_SELECTION));
	}

	buffer_get_yx(buf, &y, &x);
	y -= editor.screen_start;
	move(y + 2, x);
}

int editor_process_key(int key)
{
	int i = 0;

	editor.key_last = key;
	/* TODO: assert that editor has keybindings */
	while (editor.keybindings[i].key >= 0) {
		if ((editor.keybindings[i].key == key || editor.keybindings[i].key == KEY_ANY)
		    && editor.keybindings[i].modemask & editor.mode)
		{
			return editor.keybindings[i].command();
		}
		i++;
	}

	return 0;
}

/* commands */

int command_move_forward_char(void)
{
	buffer_move_forward_char(editor.buf_current);
	return 0;
}

int command_move_backward_char(void)
{
	buffer_move_backward_char(editor.buf_current);
	return 0;
}

int command_move_forward_word(void)
{
	buffer_move_forward_word(editor.buf_current);
	return 0;
}

int command_move_backward_word(void)
{
	buffer_move_backward_word(editor.buf_current);
	return 0;
}

int command_move_forward_line(void)
{
	buffer_move_forward_line(editor.buf_current);
	return 0;
}

int command_move_backward_line(void)
{
	buffer_move_backward_line(editor.buf_current);
	return 0;
}

int command_move_beginning_of_line(void)
{
	buffer_move_beginning_of_line(editor.buf_current);
	return 0;
}

int command_move_end_of_line(void)
{
	buffer_move_end_of_line(editor.buf_current);
	return 0;
}

int command_move_page_up(void)
{
	int i;

	for (i = 0; i < editor.screen_width; i++)
		buffer_move_backward_line(editor.buf_current);

	return 0;
}

int command_move_page_down(void)
{
	int i;

	for (i = 0; i < editor.screen_width; i++)
		buffer_move_forward_line(editor.buf_current);

	return 0;
}

int command_move_beginning_of_buffer(void)
{
	buffer_move_beginning_of_buffer(editor.buf_current);
	return 0;
}

int command_move_end_of_buffer(void)
{
	buffer_move_end_of_buffer(editor.buf_current);
	return 0;
}

int command_move_forward_bracket(void)
{
	buffer_move_forward_bracket(editor.buf_current);
	return 0;
}

int command_insert_newline(void)
{
	buffer_insert_char(editor.buf_current, '\n');
	return 0;
}

int command_insert_self(void)
{
	buffer_insert_char(editor.buf_current, editor.key_last);
	return 0;
}

int command_insert_unicode(void)
{
	char *str;
	unsigned char utf8[4];
	int len;

	str = editor_dialog("Unicode U+");
	/* TODO: This should be improved to handle invalid input. */
	len = unicode_to_utf8(strtol(str, NULL, 16), utf8);
	if (len > 0)
		buffer_insert_string(editor.buf_current, (const char *)utf8, len);
	else
		editor_error("Invalid unicode value");

	free(str);
	return 0;
}

int command_open_below(void)
{
	buffer_move_end_of_line(editor.buf_current);
	buffer_insert_string(editor.buf_current, "\n", 1);
	editor.mode = M_EDITING;
	return 0;
}

int command_open_above(void)
{
	buffer_move_beginning_of_line(editor.buf_current);
	buffer_insert_string(editor.buf_current, "\n", 1);
	buffer_move_backward_char(editor.buf_current);
	editor.mode = M_EDITING;
	return 0;
}

int command_move_backward_bracket(void)
{
	buffer_move_backward_bracket(editor.buf_current);
	return 0;
}

static void goto_line_action(void)
{
	char *text;

	text = buffer_get_content(editor.minibuf.buf);

	if (editor.minibuf.buf->used == 0) {
		editor.buf_current->cursor = editor.cursor_last;
		editor.buf_current->cur_line = editor.line_last;
	} else {
		buffer_goto_line(editor.buf_current, atoi(text));
	}
	free(text);
	editor.mode = M_COMMAND;
}
static void goto_line_update(void)
{
	goto_line_action();
	command_recenter();
	editor.mode = M_MINIBUFFER;
}
static void goto_line_cancel(void)
{
	editor.mode = M_COMMAND;
	editor.buf_current->cursor = editor.cursor_last;
	editor.buf_current->cur_line = editor.line_last;
	command_recenter();
}
int command_goto_line(void)
{
	buffer_clear(editor.minibuf.buf);
	editor.cursor_last = editor.buf_current->cursor;
	editor.line_last = editor.buf_current->cur_line;
	editor.mode = M_MINIBUFFER;
	editor.minibuf.prompt = "Line → ";
	editor.minibuf.action_cb = goto_line_action;
	editor.minibuf.update_cb = goto_line_update;
	editor.minibuf.cancel_cb = goto_line_cancel;
	return 0;
}

/* TODO: deletions should be smart when in selection mode */
/* TODO: deletions leak */
int command_delete_forward_char(void)
{
	buffer_delete_forward_char(editor.buf_current);
	return 0;
}

int command_delete_backward_char(void)
{
	buffer_delete_backward_char(editor.buf_current);
	return 0;
}

int command_delete_forward_word(void)
{
	buffer_delete_forward_word(editor.buf_current, NULL, NULL);
	return 0;
}

int command_delete_backward_word(void)
{
	buffer_delete_backward_word(editor.buf_current, NULL, NULL);
	return 0;
}

int command_delete_selection_or_line(void)
{
	if (editor.buf_current->sel_active)
		buffer_delete_selection(editor.buf_current, &editor.clipboard, &editor.clipboard_len);
	else
		buffer_delete_line(editor.buf_current, &editor.clipboard, &editor.clipboard_len);
	editor.mode = M_COMMAND;
	return 0;
}

int command_clear(void)
{
	buffer_clear(editor.buf_current);
	return 0;
}

int command_paste(void)
{
	buffer_insert_string(editor.buf_current, editor.clipboard, editor.clipboard_len);
	return 0;
}

int command_toggle_selection_mode(void)
{
	buffer_selection_toggle(editor.buf_current);
	if (editor.buf_current->sel_active)
		editor.mode = M_SELECTION;
	else
		editor.mode = M_COMMAND;
	return 0;
}

static void search_action(void)
{
	editor.mode = M_COMMAND;
}
static void search_update_common(char *str)
{
	int p, nl;

	if (editor.search_dir == SEARCH_FORWARD) {
		p = buffer_find_str_next(editor.buf_current,
					 editor.cursor_last,
					 str,
					 &nl);
		if (p >= 0) {
			editor.buf_current->cursor = editor.cursor_last + p;
			editor.buf_current->cur_line = editor.line_last + nl;
		}
	} else {
		p = buffer_find_str_prev(editor.buf_current,
					 editor.cursor_last,
					 str,
					 &nl);
		if (p >= 0) {
			editor.buf_current->cursor = p;
			editor.buf_current->cur_line = nl;
		}
	}
}
static void search_update(void)
{
	char *text;

	text = buffer_get_content(editor.minibuf.buf);

	if (editor.minibuf.buf->used == 0) {
		editor.buf_current->cursor = editor.cursor_last;
		editor.buf_current->cur_line = editor.line_last;
	} else {
		search_update_common(text);
		editor.search_last = strdup(text); /* TODO: LEAK! */
	}
	free(text);

	command_recenter();
	editor.mode = M_MINIBUFFER;
}
static void search_cancel(void)
{
	editor.mode = M_COMMAND;
	editor.buf_current->cursor = editor.cursor_last;
	editor.buf_current->cur_line = editor.line_last;
	command_recenter();
}
static int command_search_common(char *prompt)
{
	buffer_clear(editor.minibuf.buf);
	editor.cursor_last = editor.buf_current->cursor;
	editor.line_last = editor.buf_current->cur_line;
	editor.mode = M_MINIBUFFER;
	editor.minibuf.prompt = prompt;
	editor.minibuf.action_cb = search_action;
	editor.minibuf.update_cb = search_update;
	editor.minibuf.cancel_cb = search_cancel;
	return 0;
}

int command_search_forward(void)
{
	editor.search_dir = SEARCH_FORWARD;
	return command_search_common("Search → ");
}

int command_search_backward(void)
{
	editor.search_dir = SEARCH_BACKWARD;
	return command_search_common("Search ← ");
}

int command_goto_next_search(void)
{
	int p, nl;

	editor.mode = M_COMMAND;
	if (!editor.search_last)
		return 0;

	p = buffer_find_str_next(editor.buf_current,
				 editor.buf_current->cursor + 1,
				 editor.search_last,
				 &nl);
	if (p >= 0) {
		editor.buf_current->cursor += p + 1;
		editor.buf_current->cur_line += nl;
	}

	return 0;
}

int command_goto_previous_search(void)
{
	int p, nl;

	editor.mode = M_COMMAND;
	if (!editor.search_last)
		return 0;

	p = buffer_find_str_prev(editor.buf_current,
				 editor.buf_current->cursor,
				 editor.search_last,
				 &nl);
	if (p >= 0) {
		editor.buf_current->cursor = p;
		editor.buf_current->cur_line = nl;
	}

	return 0;
}

int command_editor_command_mode(void)
{
	editor.mode = M_COMMAND;
	return 0;
}

int command_editor_editing_mode(void)
{
	editor.mode = M_EDITING;
	return 0;
}

int command_save_buffer(void)
{
	editor_save(false);
	return 0;
}

int command_write_buffer(void)
{
	editor_save(true);
	return 0;
}

int command_load_buffer(void)
{
	editor_load_file();
	return 0;
}

int command_next_buffer(void)
{
	editor_next_buffer();
	return 0;
}

int command_previous_buffer(void)
{
	editor_previous_buffer();
	return 0;
}

int command_recenter(void)
{
	editor.screen_start = max(0, editor.buf_current->cur_line - editor.screen_width / 2 + 2);
	return 0;
}

int command_minibuffer_do_action(void)
{
	if (editor.minibuf.action_cb)
		editor.minibuf.action_cb();
	return 0;
}

int command_minibuffer_delete_backward_char(void)
{
	buffer_delete_backward_char(editor.minibuf.buf);
	if (editor.minibuf.update_cb)
		editor.minibuf.update_cb();
	return 0;
}

int command_minibuffer_clear(void)
{
	buffer_clear(editor.minibuf.buf);
	if (editor.minibuf.update_cb)
		editor.minibuf.update_cb();
	return 0;
}

int command_minibuffer_insert_self_and_update(void)
{
	buffer_insert_char(editor.minibuf.buf, editor.key_last);
	if (editor.minibuf.update_cb)
		editor.minibuf.update_cb();
	return 0;
}

int command_minibuffer_cancel(void)
{
	if (editor.minibuf.cancel_cb)
		editor.minibuf.cancel_cb();
	return 0;
}

/* Does not return */
int command_editor_quit(void)
{
	endwin();
	exit(0);
}

static int get_input(void)
{
	int c;

	c = wgetch(stdscr);
	if (c == KEY_ENTER || c == '\n')
		return KEY_ENTER;
	else if (c == KEY_BACKSPACE || c == 127)
		return KEY_BACKSPACE;

	return c;
}

static void finish(int sig)
{
	endwin();
	exit(0);
}

int main(int argc, char **argv)
{
	signal(SIGINT, finish);

	setlocale(LC_ALL, "");
	initscr();
	init_colors(colors, color_pairs);
	raw();
	noecho();
	keypad(stdscr, TRUE);

	editor_init(argc, argv);

	for (;;) {
		doupdate();

		clear();
		editor_show_status_line();
		editor_update_screen();
		editor_redisplay();
		editor_process_key(get_input());
	}
}
