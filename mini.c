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
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <curses.h>

#define BUFFER_NAME_SIZE   512
#define BUFFER_ALLOC_CHUNK 256

#define KEY_ESC 0x1b
#define CTRL(c) ((c) - 0x60)
#define CTRL_SPACE 0x00

#define TAB_STOP 8

#define C_COMMAND 1
#define C_EDITING 2
#define C_SELECTION 3
#define C_ERROR 4

struct buffer {
	struct buffer *buf_next;
	struct buffer *buf_prev;
	char name[BUFFER_NAME_SIZE];
	char *path;
	unsigned size;
	unsigned used;
	int cursor;
	int cur_line;
	int last_line;
	bool modified;
	int cursor_column;
	int sel_start;
	int sel_end;
	bool sel_active;
	int gap_start;
	int gap_end;
	char *data;
};

/* buffer content */
struct buffer *buffer_new(void);
void buffer_free(struct buffer *buf);
int buffer_save(struct buffer *buf, const char *path);
int buffer_load(struct buffer *buf, const char *path);
void buffer_set_path(struct buffer *buf, const char *path);

/* buffer internal */
int cursor_to_data(struct buffer *buf, int pos);
char buffer_data_at(struct buffer *buf, int pos);
void buffer_expand(struct buffer *buf, size_t chunk);
void buffer_adjust_gap(struct buffer *buf);
int buffer_get_next_newline(struct buffer *buf, int from, int way);
int buffer_get_line_beginning(struct buffer *buf);
int buffer_get_line_end(struct buffer *buf);
int buffer_get_line_length(struct buffer *buf);
void buffer_get_region(struct buffer *buf, int line_start, int lines, int *beg, int *end);
void buffer_get_yx(struct buffer *buf, int *y, int *x);

/* buffer movement */
void buffer_move_forward_char(struct buffer *buf);
void buffer_move_backward_char(struct buffer *buf);
void buffer_move_forward_word(struct buffer *buf);
void buffer_move_backward_word(struct buffer *buf);
void buffer_move_forward_line(struct buffer *buf);
void buffer_move_backward_line(struct buffer *buf);
void buffer_move_beginning_of_line(struct buffer *buf);
void buffer_move_end_of_line(struct buffer *buf);
void buffer_move_beginning_of_buffer(struct buffer *buf);
void buffer_move_end_of_buffer(struct buffer *buf);

/* buffer insertion and deletion */
void buffer_insert_char(struct buffer *buf, const char c);
void buffer_insert_string(struct buffer *buf, const char *str, size_t len);
void buffer_delete_forward_char(struct buffer *buf);
void buffer_delete_backward_char(struct buffer *buf);
void buffer_delete_forward_word(struct buffer *buf, char **out, int *n_out);
void buffer_delete_backward_word(struct buffer *buf, char **out, int *n_out);
void buffer_delete_region(struct buffer *buf, int beg, int end, char **out, int *n_out);
void buffer_delete_line(struct buffer *buf, char **out, int *n_out);
void buffer_delete_selection(struct buffer *buf, char **out, int *n_out);

/* buffer selection */
void buffer_selection_toggle(struct buffer *buf);
void buffer_selection_update(struct buffer *buf);

/* utils */
bool is_position_in_buffer(int pos, struct buffer *buf);
bool is_position_in_region(int pos, int beg, int end);
int str_newlines(const char *str, int n);
int region_newlines(struct buffer *buf, int beg, int end);
void oom();

/* editor */
struct editor {
	struct buffer *buf_first;
	struct buffer *buf_last;
	struct buffer *buf_current;
	enum mode {
		M_COMMAND,
		M_EDITING,
		M_SELECTION,
	} mode;
	int screen_start;
	int screen_width;
	int clipboard_len;
	char *clipboard;
};

static struct editor editor = {};

void editor_init(int argc, char *argv[]);
void editor_add_buffer(struct buffer *buf);
void editor_next_buffer(void);
void editor_previous_buffer(void);
void editor_save(bool save_as);
void editor_load_file(void);
char *editor_dialog(const char *prompt);
void editor_error(const char *error);
void editor_show_status_line(void);
void editor_update_screen(void);
void editor_redisplay(void);


struct buffer *buffer_new(void)
{
	struct buffer *buf = NULL;

	buf = calloc(1, sizeof(struct buffer));
	if (!buf)
		oom();
	strcpy(buf->name, "*Untitled*");
	buf->size = BUFFER_ALLOC_CHUNK;
	buf->data = calloc(BUFFER_ALLOC_CHUNK, 1);
	if (!buf->data)
		oom();
	buf->gap_end = buf->size;

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
	FILE *fp;
	char *tmp_buf;
	size_t read;
	int rc = 0;

	assert(buf);

	fp = fopen(path, "r");
	if (!fp)
		return -1;

	tmp_buf = malloc(BUFFER_ALLOC_CHUNK);
	assert(tmp_buf);
	while ((read = fread(tmp_buf, 1, BUFFER_ALLOC_CHUNK, fp)) > 0)
		buffer_insert_string(buf, tmp_buf, read);
	free(tmp_buf);
	fclose(fp);

	buffer_set_path(buf, path);
	buf->modified = false;

	return rc;
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
	strncpy(buf->name, name, BUFFER_NAME_SIZE);
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

	from += d;
	while (from >= 0 && from < buf->used) {
		if (buffer_data_at(buf, from) == '\n')
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

int buffer_get_line_length(struct buffer *buf)
{
	return buffer_get_line_end(buf) - buffer_get_line_beginning(buf);
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
	int p;

	p = buffer_get_line_beginning(buf);
	*y = buf->cur_line;
	*x = 0;
	while (p < buf->cursor) {
		if (buffer_data_at(buf, p) == '\t')
			*x += TAB_STOP - *x % TAB_STOP;
		else
			*x += 1;
		p++;
	}
}

static void buffer_cursor_column_update(struct buffer *buf)
{
	buf->cursor_column = buf->cursor - buffer_get_line_beginning(buf);
}

void buffer_move_forward_char(struct buffer *buf)
{
	int p = buf->cursor + 1;

	if (p <= buf->used) {
		if (buffer_data_at(buf, buf->cursor) == '\n')
			buf->cur_line++;
		buf->cursor = p;

	}
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_backward_char(struct buffer *buf)
{
	int p = buf->cursor - 1;

	if (p >= 0) {
		buf->cursor = p;
		if (buffer_data_at(buf, buf->cursor) == '\n')
			buf->cur_line--;
	}
	buffer_cursor_column_update(buf);
	buffer_selection_update(buf);
}

void buffer_move_forward_word(struct buffer *buf)
{
	while (!isalnum(buffer_data_at(buf, buf->cursor)) && buf->cursor < buf->used)
		buffer_move_forward_char(buf);
	while (isalnum(buffer_data_at(buf, buf->cursor)) && buf->cursor < buf->used)
		buffer_move_forward_char(buf);
}

void buffer_move_backward_word(struct buffer *buf)
{
	while (!isalnum(buffer_data_at(buf, buf->cursor)) && buf->cursor > 0)
		buffer_move_backward_char(buf);
	while (isalnum(buffer_data_at(buf, buf->cursor)) && buf->cursor > 0)
		buffer_move_backward_char(buf);
}

void buffer_move_forward_line(struct buffer *buf)
{
	int p = buf->cursor;

	if (buffer_data_at(buf, buf->cursor) != '\n')
		p = buffer_get_next_newline(buf, buf->cursor, 1);
	if (p >= 0) {
		int ll;

		buf->cursor = p + 1;
		ll = buffer_get_line_length(buf);
		if (buf->cursor_column < ll)
			buf->cursor += buf->cursor_column;
		else
			buf->cursor += ll;
		buf->cur_line++;
	}
	buffer_selection_update(buf);
}

void buffer_move_backward_line(struct buffer *buf)
{
	int p;

	p = buffer_get_next_newline(buf, buf->cursor, -1);
	if (p >= 0) {
		int ll;

		p = buffer_get_next_newline(buf, p, -1);
		/* p is -1 if the cursor is currently on the second line */
		buf->cursor = p + 1;
		ll = buffer_get_line_length(buf);
		if (buf->cursor_column < ll)
			buf->cursor += buf->cursor_column;
		else
			buf->cursor += ll;
		buf->cur_line--;
	}
	buffer_selection_update(buf);
}

void buffer_move_beginning_of_line(struct buffer *buf)
{
	int p, lb;

	/* Be smart about blanks */
	p = lb = buffer_get_line_beginning(buf);
	while (isblank(buffer_data_at(buf, p)))
		p++;
	if (buf->cursor == p)
		buf->cursor = lb;
	else
		buf->cursor = p;
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
	if (buf->cursor >= buf->used)
		return;

	buffer_delete_region(buf, buf->cursor, buf->cursor, NULL, NULL);
}

void buffer_delete_backward_char(struct buffer *buf)
{
	if (buf->cursor == 0)
		return;

	buffer_delete_region(buf, buf->cursor - 1, buf->cursor - 1, NULL, NULL);
}

void buffer_delete_forward_word(struct buffer *buf, char **out, int *n_out)
{
	int p = buf->cursor;

	buffer_move_forward_word(buf);
	buffer_delete_region(buf, p, buf->cursor, out, n_out);
}

void buffer_delete_backward_word(struct buffer *buf, char **out, int *n_out)
{
	int p = buf->cursor;

	buffer_move_backward_word(buf);
	buffer_delete_region(buf, buf->cursor, p, out, n_out);
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

void buffer_selection_toggle(struct buffer *buf)
{
	buf->sel_active = !buf->sel_active;

	if (buf->sel_active)
		buf->sel_start = buf->sel_end = buf->cursor;
}

void buffer_selection_update(struct buffer *buf)
{
	if (buf->sel_active)
		buf->sel_end = buf->cursor;
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

	way = beg < end ? 1 : -1;
	do {
		if (buffer_data_at(buf, p) == '\n')
			nl++;
		p += way;
	} while (is_position_in_region(p, beg, end));

	return nl;
}

void oom()
{
	die("Out of mana!");
}

void editor_init(int argc, char *argv[])
{
	struct buffer *buf = NULL;

	editor.buf_first = NULL;
	editor.buf_last = NULL;
	editor.buf_current = NULL;
	editor.mode = M_COMMAND;
	editor.screen_start = 0;
	editor.screen_width = getmaxy(stdscr) - 5;
	editor.clipboard_len = 0;
	editor.clipboard = NULL;

	buf = buffer_new();
	if (!buf)
		oom();

	if (argc > 1) {
		buffer_load(buf, argv[1]);
		if (!buf) {
			endwin();
			die("Can't open '%s'\n", argv[1]);
		}
	}
	buffer_move_beginning_of_buffer(buf);
	editor_add_buffer(buf);
	editor.buf_current = buf;
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

	attron(COLOR_PAIR(C_ERROR));
	attron(A_BOLD);
	printw(error);
	attroff(A_BOLD);
	attroff(COLOR_PAIR(C_ERROR));
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
		attron(COLOR_PAIR(C_COMMAND));
	} else if (editor.mode == M_EDITING) {
		mode = "[E]";
		attron(COLOR_PAIR(C_EDITING));
	} else {
		attron(COLOR_PAIR(C_SELECTION));
		mode = "[S]";
	}

	move(1, 0);

	buffer_get_yx(buf, &y, &x);
	attron(A_BOLD);
	printw(mode);
	attroff(COLOR_PAIR(C_SELECTION));
	attroff(COLOR_PAIR(C_EDITING));
	attroff(COLOR_PAIR(C_COMMAND));
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
			attron(A_REVERSE);
		if (c == '\t') {
			attron(A_BOLD);
			is_tab = true;
		}
		printw("%c", c);
		if (is_tab)
			attroff(A_BOLD);
		if (buf->sel_active && pos >= sel_start && pos <= sel_end)
			attroff(A_REVERSE);
	}

	buffer_get_yx(buf, &y, &x);
	y -= editor.screen_start;
	move(y + 2, x);
}

static void init_colors(void)
{
	if (has_colors()) {
		start_color();
		init_pair(C_COMMAND, COLOR_BLUE, COLOR_BLACK);
		init_pair(C_EDITING, COLOR_GREEN, COLOR_BLACK);
		init_pair(C_SELECTION, COLOR_YELLOW, COLOR_BLACK);
		init_pair(C_ERROR, COLOR_RED, COLOR_BLACK);
	}
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

static void _devel_show_status_line(struct buffer *buf)
{
	int y;
	const char *mode =
		(editor.mode == M_COMMAND) ? "[C]" :
		(editor.mode == M_EDITING) ? "[E]" :
		"[S]";

	char *selection = NULL;

	if (buf->sel_active)
		asprintf(&selection, "SS:|%d|  SE:|%d|", buf->sel_start, buf->sel_end);
	y = getmaxy(stdscr) - 3;
	attron(A_BOLD);
	mvprintw(y, 1,   "*DEV* buf->name:|%s|  buf->path:|%s|  buf->used:|%d|  buf->size:|%d|",
		 buf->name, buf->path, buf->used, buf->size);
	mvprintw(y+1, 1, "      %s  C:|%d|  CL:|%d|  LL:|%d|  GS:|%d|  GE:|%d|  %s",
		 mode, buf->cursor, buf->cur_line, buf->last_line, buf->gap_start, buf->gap_end, selection);
	mvprintw(y+2, 1, "      SS:%d  SE:%d  CC:%d",
		 editor.screen_start, editor.screen_start + editor.screen_width - 1, buf->cursor_column);
	attroff(A_BOLD);
}

int main(int argc, char **argv)
{
	signal(SIGINT, finish);

	initscr();
	init_colors();
	raw();
	noecho();
	keypad(stdscr, TRUE);

	editor_init(argc, argv);

	for (;;) {
		int c;

		doupdate();

		clear();
		/* DEVEL */
		_devel_show_status_line(editor.buf_current);

		editor_show_status_line();
		editor_update_screen();
		editor_redisplay();

		c = get_input();
		if (c == CTRL('q'))
			finish(0);
		if (editor.mode == M_COMMAND || editor.mode == M_SELECTION) {
			if (c == 'n')
				buffer_move_forward_char(editor.buf_current);
			else if (c == 'h')
				buffer_move_backward_char(editor.buf_current);
			else if (c == 'g')
				buffer_move_backward_word(editor.buf_current);
			else if (c == 'r')
				buffer_move_forward_word(editor.buf_current);
			else if (c == 'N')
				buffer_move_end_of_line(editor.buf_current);
			else if (c == 'H')
				buffer_move_beginning_of_line(editor.buf_current);
			else if (c == 't')
				buffer_move_forward_line(editor.buf_current);
			else if (c == 'c')
				buffer_move_backward_line(editor.buf_current);
			else if (c == 'T') {
				/* TODO: CMD: page down */
				for (int i = 0; i < editor.screen_width; i++)
					buffer_move_forward_line(editor.buf_current);
			}
			else if (c == 'C') {
				/* TODO: CMD: page up */
				for (int i = 0; i < editor.screen_width; i++)
					buffer_move_backward_line(editor.buf_current);
			}
			else if (c == 'd')
				buffer_move_end_of_buffer(editor.buf_current);
			else if (c == 'f')
				buffer_move_beginning_of_buffer(editor.buf_current);

			/* TODO: deletions leak */
			else if (c == KEY_DC || c == 'u')
				buffer_delete_forward_char(editor.buf_current);
			else if (c == KEY_BACKSPACE || c == 'e')
				buffer_delete_backward_char(editor.buf_current);
			else if (c == '.')
				buffer_delete_backward_word(editor.buf_current, &editor.clipboard, &editor.clipboard_len);
			else if (c == 'p')
				buffer_delete_forward_word(editor.buf_current, &editor.clipboard, &editor.clipboard_len);
			else if (c == 'q') {
				/* TODO: CMD: delete active selection or whole line */
				if (editor.buf_current->sel_active)
					buffer_delete_selection(editor.buf_current, &editor.clipboard, &editor.clipboard_len);
				else
					buffer_delete_line(editor.buf_current, &editor.clipboard, &editor.clipboard_len);
			}
			else if (c == 'k')
				/* TODO: CMD?: paste */
				buffer_insert_string(editor.buf_current, editor.clipboard, editor.clipboard_len);

			else if (c == 'v') {
				buffer_selection_toggle(editor.buf_current);
				if (editor.buf_current->sel_active)
					editor.mode = M_SELECTION;
				else
					editor.mode = M_COMMAND;
			}

			else if (c == CTRL('s'))
				editor_save(false);
			else if (c == CTRL('w'))
				editor_save(true);
			else if (c == CTRL('l'))
				editor_load_file();
			else if (c == '[')
				editor_previous_buffer();
			else if (c == ']')
				editor_next_buffer();
			else if (c == KEY_ENTER)
				editor.mode = M_EDITING;

		} else if (editor.mode == M_EDITING) {
			if (c == KEY_ENTER)
				buffer_insert_char(editor.buf_current, '\n');
			else if (c == KEY_DC)
				buffer_delete_forward_char(editor.buf_current);
			else if (c == KEY_BACKSPACE)
				buffer_delete_backward_char(editor.buf_current);
			else if (c == KEY_ESC)
				editor.mode = M_COMMAND;
			else
				buffer_insert_char(editor.buf_current, c);
		}
	}
}
