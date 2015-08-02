/*
 * Copyright 2015 Jan Synáček
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
/* KEY_ANY keybindings have to be ordered *after* everything else, they are a catch-all. */
#define KEY_ANY 0xff
#define CTRL(c) ((c) - 0x60)
#define CTRL_SPACE 0x00

#define TAB_STOP 8
#define DEFAULT_KEYBINDINGS dvorak_keybindings

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
char *buffer_get_content(struct buffer *buf);
int buffer_find_char(struct buffer *buf, int from, int way, const char *accept, int *newlines);
int buffer_find_next(struct buffer *buf, int from, const char *accept, int *newlines);
int buffer_find_previous(struct buffer *buf, int from, const char *accept, int *newlines);

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
void buffer_move_forward_bracket(struct buffer *buf);
void buffer_move_backward_bracket(struct buffer *buf);
void buffer_goto_line(struct buffer *buf, int line);

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
void buffer_clear(struct buffer *buf);

/* Buffer selection */
void buffer_selection_toggle(struct buffer *buf);
void buffer_selection_update(struct buffer *buf);

/* Buffer search */
int buffer_search_forward(struct buffer *buf, const char *str);
int buffer_search_backward(struct buffer *buf, const char *str);

/* Utils */
static inline int max(int a, int b) { return ((a > b) ? a : b); }
bool is_position_in_buffer(int pos, struct buffer *buf);
bool is_position_in_region(int pos, int beg, int end);
int str_newlines(const char *str, int n);
int region_newlines(struct buffer *buf, int beg, int end);
void oom();

/** Editor */

enum mode {
	M_COMMAND   = 0x1,
	M_EDITING   = 0x2,
	M_SELECTION = 0x4,
	M_ALL_BASIC = 0xff,

	M_MINIBUFFER = 0xff00,
	M_ALL = 0xffff
};

struct minibuffer {
	char *prompt;
	struct buffer *buf;
	void (*action_cb)(void);
	void (*update_cb)(void);
	void (*cancel_cb)(void);
};

struct keybinding {
	int key;
	int modemask;
	int (*command)(void);
};

struct editor {
	struct buffer *buf_first;
	struct buffer *buf_last;
	struct buffer *buf_current;
	struct minibuffer minibuf;
	enum mode mode;
	int screen_start;
	int screen_width;
	int clipboard_len;
	char *clipboard;
	int cursor_last;
	int line_last;
	int key_last;
	struct keybinding *keybindings;
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
int editor_process_key(int key);

/** Commands */

int command_move_forward_char(void);
int command_move_backward_char(void);
int command_move_forward_word(void);
int command_move_backward_word(void);
int command_move_forward_line(void);
int command_move_backward_line(void);
int command_move_beginning_of_line(void);
int command_move_end_of_line(void);
int command_move_page_up(void);
int command_move_page_down(void);
int command_move_beginning_of_buffer(void);
int command_move_end_of_buffer(void);
int command_move_forward_bracket(void);
int command_move_backward_bracket(void);
int command_goto_line(void);
int command_insert_newline(void);
int command_insert_self(void);
int command_insert_unicode(void);
int command_open_below(void);
int command_open_above(void);
int command_delete_forward_char(void);
int command_delete_backward_char(void);
int command_delete_forward_word(void);
int command_delete_backward_word(void);
int command_delete_selection_or_line(void);
int command_clear(void);
int command_paste(void);
int command_toggle_selection_mode(void);
int command_search_forward(void);
int command_search_backward(void);
int command_editor_command_mode(void);
int command_editor_editing_mode(void);
int command_save_buffer(void);
int command_write_buffer(void);
int command_load_buffer(void);
int command_next_buffer(void);
int command_previous_buffer(void);
int command_recenter(void);
int command_minibuffer_do_action(void);
int command_minibuffer_delete_backward_char(void);
int command_minibuffer_clear(void);
int command_minibuffer_insert_self_and_update(void);
int command_minibuffer_cancel(void);
int command_editor_quit(void);

#endif
