/*
 * Copyright (C) 2015 Jan Synáček
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

#ifndef MINI_H
#define MINI_H

/** General */

#define KEY_ESC 0x1b
#define CTRL(c) ((c) - 0x60)
#define CTRL_SPACE 0x00

#define TAB_STOP 8

#define C_COMMAND 1
#define C_EDITING 2
#define C_SELECTION 3
#define C_ERROR 4

/** Buffer */

#define BUFFER_NAME_SIZE   512
#define BUFFER_ALLOC_CHUNK 256

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

/* Buffer content */
struct buffer *buffer_new(void);
void buffer_free(struct buffer *buf);
int buffer_save(struct buffer *buf, const char *path);
int buffer_load(struct buffer *buf, const char *path);
void buffer_set_path(struct buffer *buf, const char *path);

/* Buffer internal */
int cursor_to_data(struct buffer *buf, int pos);
char buffer_data_at(struct buffer *buf, int pos);
void buffer_expand(struct buffer *buf, size_t chunk);
void buffer_adjust_gap(struct buffer *buf);
int buffer_get_next_newline(struct buffer *buf, int from, int way);
int buffer_get_line_beginning(struct buffer *buf);
int buffer_get_line_end(struct buffer *buf);
int buffer_get_line_offset(struct buffer *buf);
int buffer_get_line_length(struct buffer *buf);
void buffer_get_region(struct buffer *buf, int line_start, int lines, int *beg, int *end);
void buffer_get_yx(struct buffer *buf, int *y, int *x);

/* Buffer movement */
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

/* Buffer insertion and deletion */
void buffer_insert_char(struct buffer *buf, const char c);
void buffer_insert_string(struct buffer *buf, const char *str, size_t len);
void buffer_delete_forward_char(struct buffer *buf);
void buffer_delete_backward_char(struct buffer *buf);
void buffer_delete_forward_word(struct buffer *buf, char **out, int *n_out);
void buffer_delete_backward_word(struct buffer *buf, char **out, int *n_out);
void buffer_delete_region(struct buffer *buf, int beg, int end, char **out, int *n_out);
void buffer_delete_line(struct buffer *buf, char **out, int *n_out);
void buffer_delete_selection(struct buffer *buf, char **out, int *n_out);

/* Buffer selection */
void buffer_selection_toggle(struct buffer *buf);
void buffer_selection_update(struct buffer *buf);

/* Utils */
#define is_utf8(c) (((c) & 0xC0) != 0x80)
bool is_position_in_buffer(int pos, struct buffer *buf);
bool is_position_in_region(int pos, int beg, int end);
int str_newlines(const char *str, int n);
int region_newlines(struct buffer *buf, int beg, int end);
void oom();

/** Editor */

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

#endif
