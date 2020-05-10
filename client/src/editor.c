/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* file : editor.c                                                           *
*        Editor interno per il cittaclient                                  *
****************************************************************************/
#include <assert.h>
#include "editor.h"
#include "cterminfo.h"
#include "room_cmd.h" /* for blog_display_pre */

/* Global variables to allow access to Display data for inkey.c */
int gl_Editor_Pos;   /* Top terminal row for the editor, row of status bar */
int gl_Editor_Hcurs; /* Posizione cursore orizzontale...                   */
int gl_Editor_Vcurs; /*                  ... e verticale.                  */

#define EDITOR_PROMPT_CHAR  '>'
#define EDITOR_PROMPT_COLOR COLOR(YELLOW, BLACK, ATTR_BOLD)

static void display_invalidate_top_line(void *line);

#ifdef HAVE_CTI

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "cittaclient.h"
#include "cittacfg.h" /* for CLIENT_TABSIZE */
#include "cml.h"
#include "conn.h"
#include "edit.h"
#include "editor.h"
#include "extract.h"
#include "file_flags.h"
#include "inkey.h"
#include "macro.h"
#include "metadata.h"
#include "metadata_ops.h"
#include "prompt.h"
#include "tabc.h"
#include "text.h"
#include "utility.h"
#include "generic_cmd.h"

/* Per avere informazione sul debugging definire EDITOR_DEBUG */
#undef EDITOR_DEBUG
//#define EDITOR_DEBUG

#ifdef EDITOR_DEBUG
# define DEB(...) console_printf(__VA_ARGS__)
#else
# define DEB(...)
#endif

#ifdef EDITOR_DEBUG
# define PERFORM_EDITOR_TESTS
#endif

#define MODE_INSERT     0
#define MODE_OVERWRITE  1
#define MODE_ASCII_ART  2


/* Note: the `str` and `col` strings do not require a terminating 0. */
typedef struct Line_t {
	int str[MAX_EDITOR_COL];    /* Stringa in input              */
	int col[MAX_EDITOR_COL];    /* Colori associati ai caratteri */
	int num;                    /* Numero riga                   */
	int pos;                    /* Posizione del cursore         */
	int len;                    /* Lunghezza della stringa       */
	char flag;                  /* 1 se contiene testo           */
	bool dirty; 		    /* true if contents have changed */
	int dirt_begin;
	int dirt_end;
	struct Line_t *prev; /* Linea precedente              */
	struct Line_t *next; /* Linea successiva              */
} Line;

static void dirty_clear(Line *line)
{
	line->dirty = false;
	line->dirt_begin = 0;
	line->dirt_end = 0;
}

static void dirty_all(Line *line)
{
	line->dirty = true;
	line->dirt_begin = 0;
	line->dirt_end = line->len;
}

static void dirty_index(Line *line, int index)
{
	if (line->dirty) {
		line->dirt_begin = MIN(line->dirt_begin, index);
		line->dirt_end = MAX(line->dirt_end, index + 1);
	} else {
		line->dirty = true;
		line->dirt_begin = index;
		line->dirt_end = index + 1;
	}
}

static void dirty_range(Line *line, int begin, int end)
{
	assert(begin <= end);
	if (begin == end) {
		return;
	}
	if (line->dirty) {
		line->dirt_begin = MIN(line->dirt_begin, begin);
		line->dirt_end = MAX(line->dirt_end, end);
	} else {
		line->dirty = true;
		line->dirt_begin = begin;
		line->dirt_end = end;
	}
}

/*
 * Allocate and return a new, empty string.
 * Note: The new line starts 'dirty', since it has not yet been displayed.
 */
static Line *line_new(void)
{
	Line *line;
	CREATE(line, Line, 1, 0);
	*line = (Line){0};
	line->dirty = true;
	line->dirt_begin = 0;
	line->dirt_end = 0;
	return line;
}

static inline
int line_get_mdnum(Line *line, int pos)
{
	int c = line->col[pos];
	return (c >> 16) & 0xff;
}

static inline
void line_set_mdnum(Line *line, int pos, int mdnum)
{
	int c = line->col[pos];
	line->col[pos] = (mdnum << 16) | (c & 0xffff);
	dirty_index(line, pos);
}

void lines_update_nums(Line *line)
{
	int num = line->prev ? line->prev->num : 0;

	while(line) {
		line->num = ++num;
		line = line->next;
	}
}

static inline
void line_replace_char_idx(Line *line, int index, int ch, int col) {
	assert(index < line->len);
	/* assert(line_get_mdnum(line, index) == 0); */
	line->str[index] = ch;
	line->col[index] = col;
	dirty_index(line, index);
}

static inline
void line_replace_attr_idx(Line *line, int index, int attr) {
	assert(index < line->len);
	assert(line_get_mdnum(line, index) == 0);
	line->col[index] = attr;
	dirty_index(line, index);
}

/*
static inline
void line_replace_attr_range(Line *line, int first, int last, int attr) {
	assert(index < line->len);
	assert(line_get_mdnum(line, index) == 0);
	line->col[index] = attr;
	line->dirty = true;
}
*/

/* Copy the chars [src_offset, src_offset + char_count) in src to
 * dst at slots [dst_offset, dst_offset + char_count). If the copied
 * characters do not fit the original dst string, extend dst accordingly,
 * to length dst_offset + char_count. */
void line_copy_n(Line *dst, int dst_offset, Line *src, int src_offset,
		 int char_count)
{
	assert(dst != src);
	assert(0 <= src_offset && src_offset <= src->len);
	assert(0 <= dst_offset && dst_offset <= dst->len);
	assert(src_offset + char_count <= src->len);
	int bytes = char_count*sizeof(int);
	memcpy(dst->str + dst_offset, src->str + src_offset, bytes);
	memcpy(dst->col + dst_offset, src->col + src_offset, bytes);
	if (dst->len < dst_offset + char_count) {
		dst->len = dst_offset + char_count;
	}
	dirty_range(dst, dst_offset, dst_offset + char_count);
}

/* Copy the chars [src_offset, src->len) in src to dst, starting at dst_offset.
 * If the copied characters do not fit the original dst string, extend dst
 * accordingly, to length dst_offset + src->len - src_offset. */
static inline
void line_copy(Line *dst, int dst_offset, Line *src, int src_offset)
{
	line_copy_n(dst, dst_offset, src, src_offset, src->len - src_offset);
}

/* Copy contents of line src to line dest. The contents of src are lost. */
static inline
void line_copy_all(Line *dst, Line *src)
{
	line_copy(dst, 0, src, 0);
}

static inline
void line_copy_from(Line *dst, Line *src, int src_offset)
{
	line_copy(dst, 0, src, src_offset);
}

/* Removes the character at index `pos` */
void line_remove_index(Line *line, int pos)
{
	assert(pos >= 0 && line->len > pos);
	if (pos < line->len - 1) {
		size_t bytes = (line->len - pos)*sizeof(int);
		memmove(line->str + pos, line->str + pos + 1, bytes);
		memmove(line->col + pos, line->col + pos + 1, bytes);
	}
	line->len--;
	dirty_range(line, pos, line->len + 1);
}

/* Expands the string by 1 character at position `pos` */
void line_expand_index(Line *line, int pos)
{
	assert(pos >= 0 && line->len >= pos);
	size_t bytes = (line->len - pos)*sizeof(int);
	memmove(line->str + pos + 1, line->str + pos, bytes);
	memmove(line->col + pos + 1, line->col + pos, bytes);
	line->len++;
	dirty_range(line, pos, line->len);
}

/* Expands the back of the string by 1 character */
static inline
void line_expand_back(Line *line)
{
	line->len++;
	dirty_range(line, line->len - 1, line->len);
}

/* Expands the string by adding `chars_count characters in front.
 * The contents of the line are moved chars_count slots to the right. */
void line_expand_front(Line *line, int chars_count)
{
	assert(chars_count >= 0);
	size_t bytes = line->len*sizeof(int);
	memmove(line->str + chars_count, line->str, bytes);
	memmove(line->col + chars_count, line->col, bytes);
	line->len += chars_count;
	dirty_all(line);
}

/* Expand the front of dst by count character, and copy in the front of dst
 * the characters [offset, offset + count) of src. If separator is true,
 * dst is expanded by an extra slot between the copied characters and its old
 * content. A space, with attributes `color`, is placed in the extra slot. */
void line_insert_range_front(Line *dst, Line *src, int offset, int count,
			     bool separator, int color)
{
	line_expand_front(dst, count + separator);
	line_copy_n(dst, 0, src, offset, count);
	if (separator) {
		dst->str[count] = ' ';
		dst->col[count] = color;
	}
}

/* Remove from line the characters in the interval [begin, end) */
void line_remove_range(Line *line, int begin, int end)
{
	assert(begin >= 0 && begin <= end && end <= line->len);

	if (begin == end) {
		return;
	}
	int initial_len = line->len;
	if (end == line->len) {
		line->len = begin;
	} else {
		int count = line->len - end;
		memmove(line->str + begin, line->str + end, count*sizeof(int));
		memmove(line->col + begin, line->col + end, count*sizeof(int));
 		line->len -= end - begin;
	}
	dirty_range(line, begin, initial_len);
}

/* Append white spaces to the line until it is of length pos + 1 */
void line_extend_to_pos(Line *line, int pos) {
	if (line->len <= pos) {
		dirty_range(line, line->len, pos + 1);
		while (line->len <= pos) {
			line->str[line->len] = ' ';
			line->col[line->len] = C_DEFAULT;
			line->len++;
		}
	}
}

/* Return index of last printable (non blank) char in line, -1 if none */
static int line_last_char_idx(Line *line)
{
	int last = line->len - 1;
	while (last >= 0 && line->str[last] == ' ') {
		last--;
	}
	return last;
}

/* Remove all trailing blank characters from the line. */
static void line_strip_trailing_space(Line *line) {
	int initial_len = line->len;
	while (line->len > 0
	       && *(line->str + line->len - 1) == ' ') {
		line->len--;
	}
	dirty_range(line, line->len, initial_len);
}

/* Truncates the line to length len (must be <= line->len) */
static inline
void line_truncate_at(Line *line, int len)
{
	/* TODO There could be some metadata stored in the part of the string
	   that we truncate; we should free it first. */
	assert(len <= line->len);
	if (len < line->len) {
		int initial_len = line->len;
		line->len = len;
		dirty_range(line, len, initial_len);
	}
}

/*
      first_blank    last
            v          v
  "This is a    sentence  "
   ^            ^         ^
   0           first     len
*/
/* NOTE: if last == -1 the word is empty */
typedef struct {
	int first;
	int last;
	int prev_blank;
} WordPos;

/* Return the position of the last word in *line as a WordPos structure,
 * that includes the indices of the first and last chars in the word, and
 * the starting index of the whitespace in front of the char (or 0) */
WordPos find_last_word(Line *line)
{
	int last = line_last_char_idx(line);
	if (last == -1) {
		return (WordPos){.first = 0, .last = -1};
	}
	int first = last;
	int mdnum = line_get_mdnum(line, last);
	if (mdnum) { /* if there's an attachment, treat it as a single word */
		while ((line_get_mdnum(line, first) == mdnum)
		       && (first > 0)) {
			first--;
		}
	} else {
		while ((first > 0) && (line->str[first - 1] != ' ')) {
			first--;
		}
	}

	int blank = first;
	while (blank && line->str[blank - 1] == ' ') {
		blank--;
	}

	assert(blank == 0 || line->str[blank] == ' ');
	assert(first == 0 || line->str[first - 1] == ' ');
	assert(first == last || line->str[first] != ' ');
	assert(first == last || line->str[last] != ' ');
	assert(first == last || last == line->len - 1
	       || line->str[last + 1] == ' ');

	return (WordPos){.first = first, .last = last, .prev_blank = blank};
}

/* Return the index of the first non-blank character starting from pos */
int line_next_word_idx(Line *line, int pos)
{
	assert(pos <= line->len);
	int offset = pos;
	while(*(line->str + offset) == ' ' && offset < line->len) {
		offset++;
	}
	return offset;
}

/*********************************************************************/

typedef enum {
	NO_CHANGE = 0, LINE_INSERT, LINE_DELETE,
	OP_PAGEUP, OP_PAGEDOWN,
} TextBufOp;

typedef struct TextBuf_t {
	Line *first;		/* first line                       */
	Line *last;    		/* last line                        */
        int lines_count;      	/* number of lines in the list      */
	TextBufOp operation;
	Line *op_first_line;
	Line *op_last_line;
	int op_del_num; /* Line number that has been deleted if LINE_DELETE */
} TextBuf;

/*********************************************************************/

typedef struct {
	int max;      /* Numero massimo di colonne nel testo      */
	char insert;  /* 1 se in insert mode, 0 altrimenti        */
	int curs_col; /* Colore corrente del cursore              */
	int curr_col; /* Ultimo settaggio di colore sul terminale */
	TextBuf *text;     /* text buffer                         */
	TextBuf *killbuf;  /* kill buffer storing kill text       */
	Line *curr; /* current line (where cursor is)      */
	int term_nrows;       /* Num rows in terminal             */
        bool buf_pasted;  /* true se il buffer e' stato incollato */
        bool copy; /* true se aggiunto riga al copy buf nell'ultima op */
        Metadata_List *mdlist; /* metadata and attachments        */
} Editor_Text;

/* Editor_Merge_Lines() result codes */
typedef enum {
	MERGE_NOTHING     = 0, /* The text was not modified                */
	MERGE_ABOVE_EMPTY = 1, /* The line above was empty and was deleted */
	MERGE_BELOW_EMPTY = 2, /* The line balow was empty and was deleted */
	MERGE_EXTRA_SPACE = 3, /* An extra space was added befoer merging  */
	MERGE_REGULAR     = 4, /* No extra space needed; merge performed   */
} Merge_Lines_Result;


/* Macro per settare i colori, attributi e mdnum
 * Col ha la seguente struttura
 *     intero  =     8 bit | 8 bit | 8 bit | 4 bit | 4bit
 *                  liberi   mdnum   attr    bgcol   fgcol
 */
#define Editor_Fg_Color(t, c)                                             \
	Editor_Set_Color((t), COLOR((c) & 0xf, CC_BG((t)->curs_col),	  \
				    CC_ATTR((t)->curs_col)))

#define Editor_Bg_Color(t, c)                                             \
	Editor_Set_Color((t), COLOR(CC_FG((t)->curs_col), (c) & 0xf,	  \
				    CC_ATTR((t)->curs_col)))

#define Editor_Attr_Color(t, a)                                           \
	Editor_Set_Color((t), COLOR(CC_FG((t)->curs_col),                 \
                                    CC_BG((t)->curs_col), (a)))           \

#define Editor_Attr_Toggle(t, a)                                          \
	Editor_Set_Color((t), COLOR(CC_FG((t)->curs_col),                 \
                                    CC_BG((t)->curs_col),                 \
			            CC_ATTR((t)->curs_col)^((a) & ATTR_MASK)))

static inline
int attr_get_mdnum(int c)
{
	return (c >> 16) & 0xff;
}

static inline
int attr_set_mdnum(int c, int mdnum)
{
	return (mdnum << 16) | (c & 0xffff);
}

#define ATTR_SET_MDNUM(c, m) do { (c) = attr_set_mdnum((c), (m)); } while(0)

static inline
int attr_get_color(int c)
{
	return c & 0xff;
}

/* Colori */
#define COL_HEAD_MD COLOR(WHITE, RED, ATTR_BOLD)
#define COL_EDITOR_HEAD COLOR(YELLOW, BLUE, ATTR_BOLD)
#define COL_HEAD_ERROR COLOR(RED, GRAY, ATTR_BOLD)

/* Prototipi funzioni in questo file */
int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp,
		  int color, Metadata_List *mdlist);
static int get_line_wrap(Editor_Text *t, bool wrap);
static void editor_apply_attr(Editor_Text *t);
static void editor_apply_attr_word(Editor_Text *t);
static void editor_apply_attr_line(Editor_Text *t);

static void Editor_Putchar(Editor_Text *t, int c);
static void Editor_Key_Enter(Editor_Text *t);
static void Editor_Key_Backspace(Editor_Text *t);
static void Editor_Backspace(Editor_Text *t);
static void Editor_Key_Delete(Editor_Text *t);
static void Editor_Delete(Editor_Text *t);
static void Editor_Delete_Word(Editor_Text *t);
static void Editor_Delete_Next_Word(Editor_Text *t);
static void Editor_Copy_To_Kill_Buffer(Editor_Text *t);
static int Editor_Wrap_Word(Editor_Text *t);
static void Editor_Kill_Line(Editor_Text *t);
static void Editor_Yank(Editor_Text *t);
static void Editor_Key_Tab(Editor_Text *t);
static void Editor_Key_Up(Editor_Text *t);
static void Editor_Key_Down(Editor_Text *t);
static void Editor_Key_Right(Editor_Text *t);
static void Editor_Curs_Right(Editor_Text *t);
static void Editor_Key_Left(Editor_Text *t);
static void Editor_Curs_Left(Editor_Text *t);
static void Editor_PageDown(Editor_Text *t);
static void Editor_PageUp(Editor_Text *t);
static void Editor_Set_Color(Editor_Text *t, int c);
static void Editor_Insert_Metadata(Editor_Text *t);
static void Editor_Insert_Link(Editor_Text *t);
static void Editor_Insert_PostRef(Editor_Text *t);
static void Editor_Insert_Room(Editor_Text *t);
static void Editor_Insert_User(Editor_Text *t);
static void Editor_Insert_File(Editor_Text *t);
static void Editor_Insert_Text(Editor_Text *t);
static void Editor_Save_Text(Editor_Text *t);
static void Editor_Free(Editor_Text *t);
static void Editor_Free_MD(Editor_Text *t, Line *l);
static void Editor_Free_Copy_Buffer(Editor_Text *t);
static void Editor2CML(Line *line, struct text *txt, int col,
                       Metadata_List *mdlist);
static void text2editor(Editor_Text *t, struct text *txt, int color,
                        int max_col);
static void help_edit(Editor_Text *t);
static int Editor_Ask_Abort(Editor_Text *t);
#ifdef EDITOR_DEBUG
static void console_init(void);
static void console_toggle(void);
static void console_printf(const char *fmt, ...);
static void console_show_copy_buffer(Editor_Text *t);
static void console_update(Editor_Text *t);
static void console_set_status(char *fmt, ...);
static void sanity_checks(Editor_Text *t);
#endif

/******************************************************************/
/* TextBuf functions */

/* Allocate a TextBuf structure */
TextBuf * textbuf_new(void)
{
	TextBuf *buf;
	CREATE(buf, TextBuf, 1, 0);
	return buf;
}

/* Free the lines list and then free the TextBuf */
void textbuf_free(TextBuf *buf)
{
	assert(buf);

	while (buf->first) {
		Line *line = buf->first;
		buf->first = line->next;
		Free(line);
	}

	Free(buf);
}

static inline
void textbuf_clear_op(TextBuf *buf)
{
	buf->operation = NO_CHANGE;
}

static void textbuf_set_op_del(TextBuf *buf, Line *line)
{
	assert(buf->operation == NO_CHANGE);
	buf->operation = LINE_DELETE;
	buf->op_del_num = line ? line->num : 0;
}

static void textbuf_set_op_ins(TextBuf *buf, Line *line)
{
	assert(line);
	if (buf->operation == NO_CHANGE) {
		buf->operation = LINE_INSERT;
		buf->op_first_line = line;
		buf->op_last_line = line;
	} else if (buf->operation == LINE_INSERT) {
		if (!line || line->num < buf->op_first_line->num) {
			buf->op_first_line = line;
		}
		if (!line || line->num > buf->op_last_line->num) {
			buf->op_last_line = line;
		}
	} else {
		assert(false);
	}


}

void textbuf_sanity_check(TextBuf *buf)
{
	Line *line = buf->first;
	if (line == NULL) {
		assert(buf->last == NULL);
		assert(buf->lines_count == 0);
		return;
	}

	/* Check line numbers and lines_count */
	int num = 0;
	while (line) {
		num++;
		assert(line->num == num);
		line = line->next;
	}
	assert(buf->lines_count == num);

	/* Check list links */
	line = buf->first;
	assert(line->prev == NULL);
	for (;;) {
		if (line->prev == NULL) {
			assert(line == buf->first);
		} else {
			assert(line->prev->next == line);
		}
		if (line->next == NULL) {
			assert(line == buf->last);
			break;
		}
		assert(line->next->prev == line);
		line = line->next;
	}
}

/* Append a new line at bottom of text buffer. Returns pointer to new line. */
static Line * textbuf_append_new_line(TextBuf *buf)
{
	Line *line = line_new();

	line->prev = buf->last;
	line->next = NULL;
	if (buf->first == NULL) {
		buf->first = line;
	} else {
		buf->last->next = line;
	}
	buf->last = line;
	buf->lines_count++;
	line->num = buf->lines_count;

	textbuf_set_op_ins(buf, line);

	return line;
}

/* Insert a new line below 'line'. Returns pointer to the new line. */
static Line * textbuf_insert_line_below(TextBuf *buf, Line *line)
{
	Line *new_line = line_new();

	new_line->prev = line;
	new_line->next = line->next;
	line->next = new_line;
	if (new_line->next) {
		new_line->next->prev = new_line;
	} else {
		buf->last = new_line;
	}

	lines_update_nums(new_line);
	buf->lines_count++;
	assert(buf->last->num == buf->lines_count);

	textbuf_set_op_ins(buf, new_line);

	return new_line;
}

/* Insert a new line above 'line'. Returns pointer to the new line. */
static Line * textbuf_insert_line_above(TextBuf *buf, Line *line)
{
	Line *new_line = line_new();

	new_line->prev = line->prev;
	new_line->next = line;
	line->prev = new_line;
	if (new_line->prev) {
		new_line->prev->next = new_line;
	} else {
		buf->first = new_line;
	}

	lines_update_nums(new_line);
	buf->lines_count++;
	assert(buf->last->num == buf->lines_count);

	textbuf_set_op_ins(buf, new_line);

	return new_line;
}

/* Removes 'line' from the text buffer list and frees it. */
static void textbuf_delete_line(TextBuf *buf, Line *line)
{
	/* TODO if the line to be deleted is currently the top line of the
	   display, display.top_line needs to be updated, because it will
	   point to some deallocated piece of memory. We invalidate it here
	   so that it will be recalculated during the display_update()
	   phase. Find a better way to handle this... */
	display_invalidate_top_line(line);

	DEB("DEL line num %d", line->num);
	textbuf_set_op_del(buf, line);

        for (Line *tmp = line->next; tmp; tmp = tmp->next) {
		tmp->num--;
	}
	buf->lines_count--;

	if (line == buf->first) {
		buf->first = line->next;
	}
	if (line == buf->last) {
		buf->last = line->prev;
	}
	if (line->next) {
		line->next->prev = line->prev;
	}
	if (line->prev) {
		line->prev->next = line->next;
	}

	assert(buf->first->prev == NULL);
	assert(buf->last->next == NULL);
	assert(buf->first->num == 1);
	DEB("Delete line (addr %p)", (void *)line);

	Free(line);
}

/* Breaks 'line' into two lines. The original line is cut at position 'pos',
 * and the rest is moved in a new line, that is inserted below the original
 * line in the text list. Any trailing space in the above line and any
 * leading space in the below line is eliminated.                           */
void textbuf_break_line(TextBuf *buf, Line *line, int pos)
{
	textbuf_insert_line_below(buf, line);
	if (pos < line->len) {
		/* the new line starts with the first non-blank character */
		int src_offset = line_next_word_idx(line, pos);
		line_copy_from(line->next, line, src_offset);
		/* and we eliminate the trailing space from the line above */
		line_truncate_at(line, pos);
		line_strip_trailing_space(line);
	}
        line->next->pos = 0;
	line->pos = 0;

	DEB("break_line Above (len %d pos %d) Below (len %d pos %d)",
	    line->len, line->pos, line->next->len, line->next->pos);
}

/*
 * Merges the line 'above' with the line 'below'. If everything fits on a
 * single line the contents of the line below (if any) are appended to
 * the above line and the line below is eliminated. Similarly, if the
 * above line is empty, it is simply eliminated.
 * Otherwise, moves as much as fits from the line below to the above
 * line, inserting an extra space if needed to separate the words, and
 * wrapping the line at word boundaries.
 *
 * Returns an Editor_Merge_Result value specifying how the merge was
 * performed (see the enum definition for the descriptions).
 */
static Merge_Lines_Result textbuf_merge_lines(TextBuf *buf, Line *above,
					      Line *below, int maxlen)
{
	if (above == NULL || below == NULL) {
		DEB("Merge nothing");
		return MERGE_NOTHING;
	}

	/* If line above is empty, just eliminate it. */
	if (above->len == 0) {
		textbuf_delete_line(buf, above);
		return MERGE_ABOVE_EMPTY;
	}

	/* If the line below is empty, just eliminate it. */
	if (below->len == 0) {
		textbuf_delete_line(buf, below);
		return MERGE_BELOW_EMPTY;
	}

	/* Is an extra space necessary to keep words separated after merge? */
	bool need_extra_space = (above->str[above->len - 1] != ' '
				 && below->str[0] != ' ');
	int extra_space = need_extra_space ? 1 : 0;
	/* Remaining space available in the above line */
	int avail_chars = maxlen - 1 - above->len - extra_space;

	if (avail_chars >= below->len) {
		/* the line below fits above */
		int ret;
		if (need_extra_space) {
			assert(above->len > 0);
			above->str[above->len] = ' ';
			/* If the end of the above line and the start of the
			   below line have same background color, use that for
			   the space, otherwise use the default bg color. */
			if (CC_BG(above->col[above->len - 1])
			    == CC_BG(below->col[0])) {
				/* NOTE: the below line could start with
				   an attachment: we make sure we only copy
				   the color/attribute bits! */
				above->col[above->len] = attr_get_color(
							     below->col[0]);
			} else {
				above->col[above->len] = C_DEFAULT;
			}
			above->len++;
			ret = MERGE_EXTRA_SPACE;
		} else {
			ret = MERGE_REGULAR;
		}

		line_copy(above, above->len, below, 0);
		textbuf_delete_line(buf, below);

		return ret;
	}

	/* Find out how much of the string below can be appended to the
	   string above without breaking any word */
	int len = 0;
	if (avail_chars < below->len) {
		for (int i = 0; i <= avail_chars; i++) {
			if (below->str[i] == ' ') {
				len = i;
			}
		}
	} else {
		len = below->len;
	}

	/* If a substring fits, move it above. */
	if (len > 0) {
		int ret;
		if (need_extra_space) {
			assert(above->len > 0);
			above->str[above->len] = ' ';
			/* If the end of the above line and the start of the
			   below line have same background color, use that for
			   the space, otherwise use the default bg color. */
			if (CC_BG(above->col[above->len - 1])
			    == CC_BG(below->col[0])) {
				above->col[above->len] = below->col[0];
			} else {
				above->col[above->len] = C_DEFAULT;
			}
			above->len++;
			ret = MERGE_EXTRA_SPACE;
		} else {
			ret = MERGE_REGULAR;
		}

		// TODO check this assertion
		assert(below->len >= len + 1);
		line_copy_n(above, above->len, below, 0, len);
		line_remove_range(below, 0, len + 1);

		if (below->len <= 0) {
			DEB("Delete below");
			textbuf_delete_line(buf, below);
			//ret = MERGE_BELOW_EMPTY; /* CHECK */
		}
		return ret;
	}
	/* There's no room in the above line to fit something from below */
	return MERGE_NOTHING;
}

typedef enum { DEL_FAIL, DEL_CHAR, DEL_LINE } DelCharResult;

static DelCharResult textbuf_delete_char(TextBuf *buf, Line *line, int pos,
					 int maxlen)
{
	if (pos == line->len && line == buf->last) {
		return DEL_FAIL;
	}

	if (pos == line->len) {
		Line *below = line->next;
		Merge_Lines_Result result = textbuf_merge_lines(buf, line,
								below, maxlen);
		if (result == MERGE_ABOVE_EMPTY) {
			return DEL_LINE;
		}
	} else {
		line_remove_index(line, pos);
	}

	return DEL_CHAR;
}


/* Initialize the text buffer with a single, empty line. */
void textbuf_init(TextBuf *buf)
{
	assert(buf->first == NULL && buf->last == NULL);

	textbuf_append_new_line(buf);
	buf->first->flag = 1;
	textbuf_clear_op(buf);

	assert(buf->last == buf->first);
	assert(buf->first->num == 1);
	assert(buf->first->pos == 0);
	assert(buf->first->len == 0);
	assert(buf->first->next == NULL);
	assert(buf->first->prev == NULL);
	assert(buf->lines_count == 1);
}

#ifdef PERFORM_EDITOR_TESTS
#include "tests_editor.c"
#endif

static void refresh_line_curs(int *pos, int *curs)
{
	int i;

	putchar('\r');
	putchar(EDITOR_PROMPT_CHAR);
	for (i = 0; (i < NCOL - 2) && (pos[i] != 0); i++) {
		putchar(pos[i]);
	}
	for ( ; i < NCOL - 2; i++) {
		putchar(' ');
	}
	cti_mv((int)(curs - pos + 1), NRIGHE - 1);
	fflush(stdout);
}

/****************************************************************************/
/* Editor display */

bool editor_reached_full_size;

typedef struct Display {
	bool reached_full_size;
	Line *top_line;
	int top_line_num;
	int pos;   /* starting terminal row of display (status bar) */
	int hcurs; /* horizontal cursor position                    */
	int vcurs; /* vertical cursor position                      */
	int last_pos; /* display position in previous frame         */
	int color; /* Current cursor color */
	int last_curs_row;
	int last_curs_col;
	int redraw_begin;
	int redraw_end;
	bool force_redraw;
	bool force_redraw_header;
} Display;

Display display;

/* TODO define a C_UNDEFINED that does not correspond to any possible color */
#define C_UNDEFINED 0
static Display display_init(int term_nrows, int initial_size, Line *top_line)
{
	assert(initial_size > 0);
	return (Display) {
		.reached_full_size = false,
			.top_line = top_line,
			.top_line_num = top_line->num,
			.pos = term_nrows - initial_size - 1,
			.vcurs = term_nrows - initial_size,
			.hcurs = 1, /* TODO check */
			.last_pos = -1, /* there was no editor */
			//.last_pos = term_nrows, /* there was no editor */
			.color = C_UNDEFINED,

			.last_curs_col = -1,
			.last_curs_row = -1,

			.redraw_begin = -1,
			.redraw_end = -1,
			.force_redraw = false,
			.force_redraw_header = false,
			};
}

static void display_invalidate_top_line(void *line)
{
	if ((Line *)line == display.top_line) {
		DEB("TOP LINE INVALIDATED!");
		display.top_line = NULL;
	}
}

static void display_move_curs(Display *disp, int row, int col)
{
	if ((row == disp->last_curs_row) && (col == disp->last_curs_col)) {
		return;
	}
	cti_mv(col, row);
	disp->last_curs_row = row;
	disp->last_curs_col = col;
}

static void display_move_curs_row(Display *disp, int row)
{
	if (row == disp->last_curs_row) {
		DEB("Skip curs row; already %d", row);
		return;
	}
	cti_mv(disp->last_curs_col, row);
	disp->last_curs_row = row;
}

static inline
void display_window_push(Display *disp, int first, int last)
{
	window_push(first, last);
	/* We invalidate the cursor position since we're not sure where it
	   ends up after changing the scroll region. */
	disp->last_curs_row = -1;
}

static inline
void display_window_pop(Display *disp)
{
	window_pop();
	/* We invalidate the cursor position since we're not sure where it
	   ends up after changing the scroll region. */
	disp->last_curs_row = -1;
}

/*
 * Fills line vpos with spaces with background color 'color' and leaves
 * the cursor in that row, at column 0.
 */
static void fill_line(Display *disp, int vpos, int color)
{
	display_move_curs(disp, vpos, 0);
	setcolor(color);
	printf(ERASE_TO_EOL "\r");
}

/* Erase current line, using current color, and leave cursor in column 0. */
static inline void erase_current_line(void)
{
	//display.last_curs_col = 0;
	display.last_curs_row = -1;
	printf("\r" ERASE_TO_EOL "\r");
}

static inline void erase_to_eol(void)
{
	//display.last_curs_col = 0;
	display.last_curs_row = -1;
	printf(ERASE_TO_EOL);
}

/* Erases line vpos filling it with spaces of default color. */
static inline
void display_clear_line(Display *disp, int vpos)
{
	display_move_curs(disp, vpos, 0);
	setcolor(C_DEFAULT);
	printf(ERASE_TO_EOL);
}

static inline
void display_setcolor(Display *disp, int color)
{
	if (color != disp->color) {
		setcolor(color);
		disp->color = color;
	}
}

static inline
int display_nrows(Display *disp)
{
	return NRIGHE - 1 - disp->pos;
}

/* This is the would be line number of the last display row if the text does
   not reach it */
/* TODO bad function name */
static inline
int display_bottom_line_num(Display *disp)
{
	return disp->top_line_num + display_nrows(disp) - 1;
}

static inline
int display_top_row(Display *disp)
{
	return disp->pos + 1;
}

static inline
int display_line_row(Display *disp, Line *line)
{
	assert(line);
	return display_top_row(disp) + (line->num - disp->top_line_num);
}

const char status_front[] =
	"--- Inserisci il testo ---   Help: F1   Exit: Ctrl-X   [ ";
const char status_back[] =  "%s %3d/%3d,%2d ] ";
const char *insmode[3] = {"Insert   ", "Overwrite", "ASCII Art"};

static void display_draw_head(Editor_Text *t, Display *disp)
{
	display_setcolor(disp, C_EDITOR);
	display_move_curs(disp, disp->pos, 0);
	printf(status_front);
	printf(status_back, insmode[(int)t->insert], t->curr->num,
	       t->text->lines_count, t->curr->pos + 1);

	if (NCOL > 80) {
		/*
		display_setcolor(disp, C_DEFAULT);
		erase_to_eol();
		*/
		disp->last_curs_col = 81;
	} else {
		disp->last_curs_row = disp->pos + 1;
		disp->last_curs_col = 0;
	}

	/* TODO: what is the role of the following '\n' (and it is needed!) */
	//putchar('\n');
}

/* Update the changing part (right side) of the status bast */
static void display_update_head(Editor_Text *t, Display *disp)
{
	if (disp->reached_full_size) {
		display_window_push(disp, 0, NRIGHE - 1);
	}
	display_setcolor(disp, COL_EDITOR_HEAD);
	display_move_curs(disp, disp->pos, strlen(status_front));

	printf(status_back, insmode[(int) t->insert], t->curr->num,
	       t->text->lines_count, t->curr->pos+1);

	if (disp->reached_full_size) {
		display_window_pop(disp);
	}
}

static void display_draw_prompt(Display *disp, int vpos)
{
	display_move_curs(disp, vpos, 0);
	display_setcolor(disp, EDITOR_PROMPT_COLOR);
	putchar(EDITOR_PROMPT_CHAR);
}

static void display_draw_line_range(Display *disp, Line *line, int vpos,
				    int begin, int end)
{
	display_move_curs(disp, vpos, begin + 1);
	for (int i = begin; i < end; i++) {
		display_setcolor(disp, line->col[i]);
		putchar(line->str[i]);
	}
	disp->last_curs_col += end - begin;
}

#if 0
static inline
void display_draw_line_from(Display *disp, Line *line, int vpos, int start)
{
	display_draw_line_range(disp, line,vpos, start, line->len);
}
#endif

/* Prints `line` at row `vpos` of the terminal, including the editor prompt. */
static void display_draw_line(Display *disp, Line *line, int vpos)
{
	display_draw_prompt(disp, vpos);
	display_draw_line_range(disp, line,vpos, 0, line->len);

        display_setcolor(disp, C_DEFAULT);
	erase_to_eol();
}

/* Redraws the text in the display */
static void display_draw_text(Display *disp)
{
	Line *line = disp->top_line;
	int row = disp->pos + 1;
	while (row < NRIGHE && line) {
		//DEB("Draw row %d, line num %d [%p]", row, line->num, (void*)line);
		display_draw_line(disp, line, row);
		dirty_clear(line);
		line = line->next;
		++row;
	}

	/* clear the remaining rows if the text did not fill the display */
	while (row < NRIGHE) {
		DEB("Clear row %d", row);
		display_clear_line(disp, row);
		row++;
	}
}

static void display_draw(Editor_Text *t, Display *disp)
{
	/*
	  We clear the line just above the editor status bar because the
	  apple Terminal.app behaves erraticaly when resizing the terminal
	  window, sometimes doing an extra scroll up. Erasing the line above
	  the status bar is fine because it is anyway not used.
	*/
	display_setcolor(disp, C_DEFAULT);
	display_move_curs(disp, disp->pos - 1, 0);
	erase_to_eol();

	display_draw_head(t, disp);
	display_draw_text(disp);
}

static void display_refresh_line(Display *disp, Line *line, int vpos)
{
	if (line->dirt_begin == line->dirt_end) {
		return;
	}

	if (line->dirt_begin == 0) {
		display_draw_prompt(disp, vpos);
	} else {
		display_move_curs(disp, vpos, line->dirt_begin + 1);
	}

	int end = MIN(line->dirt_end, line->len);
	display_draw_line_range(disp, line, vpos, line->dirt_begin, end);

	if (line->dirt_end > line->len) {
		display_setcolor(disp, C_DEFAULT);
		erase_to_eol();
	}
}

/* Refresh the displayed text, from terminal row 'start' to the bottom of the
 * terminal window. If 'start' is zero, refreshes the full text display.   */
static void display_refresh_text(Display *disp)
{
	Line *line = disp->top_line;
	int row = disp->pos + 1;
	while (row < NRIGHE && line) {
		if (row >= disp->redraw_begin && row < disp->redraw_end) {
			// TODO the prompt is not always needed
			display_draw_line(disp, line, row);
#ifdef EDITOR_DEBUG
			display_move_curs(disp, row, 0);
			display_setcolor(disp, L_RED);
			putchar(EDITOR_PROMPT_CHAR);
#endif
		} else if (line->dirty) {
			display_refresh_line(disp, line, row);
		}
		dirty_clear(line);
		line = line->next;
		++row;
	}

	/* clear the remaining rows if the text did not fill the display */
	while (row < NRIGHE) {
		display_clear_line(disp, row);
		row++;
	}
}

/* Refresh everything: header and text */
static void display_refresh_all(Editor_Text *t, Display *disp)
{
	/*
	  We clear the line just above the editor status bar because the
	  apple Terminal.app behaves erraticaly when resizing the terminal
	  window, sometimes doing an extra scroll up. Erasing the line above
	  the status bar is fine because it is anyway not used.
	*/
	display_setcolor(disp, C_DEFAULT);
	display_move_curs(disp, disp->pos - 1, 0);
	erase_to_eol();

	if (disp->force_redraw_header) {
		display_draw_head(t, disp);
		disp->force_redraw_header = false;
	} else {
		display_update_head(t, disp);
	}
	display_refresh_text(disp);
}

static void display_region_scroll_up(Display *disp, int first, int last, int n)
{
	assert(last < NRIGHE);
	assert(first < last);
	display_window_push(disp, first, last);
	display_move_curs_row(disp, last);
	for (int i = 0; i != n; ++i) {
		scroll_up();
	}
	display_window_pop(disp);
}

static void display_region_scroll_down(Display *disp, int first, int last,
				       int n)
{
	assert(last < NRIGHE);
	assert(first < last);
	display_window_push(disp, first, last);
	//display_move_curs(disp, first, 0);
	display_move_curs_row(disp, first);
	for (int i = 0; i != n; ++i) {
		scroll_down();
	}
	//putchar('!');
	display_window_pop(disp);

	/* ATTENTION */
	//OK WE FOUND THE PROBLEM! window_pop() moves the cursor!!!

	/*
	putchar('^');
	fflush(stdout);
	sleep(3);
	*/
}

static void display_grow(Display *disp)
{
	assert(!disp->reached_full_size);
	assert(disp->pos > MSG_WIN_SIZE);

	display_move_curs_row(disp, NRIGHE - 1);
	scroll_up();
	--disp->pos;

	if (disp->pos == MSG_WIN_SIZE) {
		/*
		printf("^ here before");
		fflush(stdout);
		sleep(2);
		*/
		display_window_push(disp, disp->pos + 1, NRIGHE - 1);
		disp->reached_full_size = true;
		/*
		printf("* here NR-1");
		fflush(stdout);
		sleep(2);
		*/
		DEB("DISP reached full size");
	}
}

static void display_grow_n(Display *disp, int n)
{
	assert(n >= 0);
	DEB("GROW %d", n);

	for (int i = 0; i != n; i++) {
		display_grow(disp);
	}
}

#if 0
/* This is the display_update() version that redraws the whole display every
   time it is called. High bandwidth, but less to worry about. */
static
void display_update_redraw_all(Editor_Text *t, Display *disp)
{
	if (!disp->top_line) {
		DEB("REVALIDATE top_line");
		disp->top_line = t->curr;
	}

	//disp->top_line = top_line;
	disp->top_line_num = disp->top_line->num;

	int bot_num = display_bottom_line_num(disp);
	DEB("Top L%d - Bot L%d", disp->top_line_num, bot_num);
	if (t->curr->num < disp->top_line_num) {
		disp->top_line = t->curr;
		disp->top_line_num = t->curr->num;
	} else if (t->curr->num > bot_num) {
		int scroll_up_count = t->curr->num - bot_num;
		if (!disp->reached_full_size) {
			int grow_count = MIN(scroll_up_count,
					     disp->pos - MSG_WIN_SIZE);
			DEB("Grow %d scrup %d botnum %d", grow_count,
			    scroll_up_count, bot_num);
			display_grow_n(disp, grow_count);
			scroll_up_count -= grow_count;
		}
		for (int i = 0; i != scroll_up_count; ++i) {
			disp->top_line = disp->top_line->next;
			disp->top_line_num++;
		}
		assert(disp->top_line_num == disp->top_line->num);
	}

	display_draw(t, disp);
	disp->vcurs = (disp->pos + 1) + (t->curr->num - disp->top_line_num);
	disp->hcurs = t->curr->pos + 1;

	textbuf_clear_op(t->text);

	disp->last_pos = disp->pos;
	display_move_curs(disp, disp->vcurs, disp->hcurs);
	display_setcolor(disp, t->curr_col);
}
#endif

static
void display_update(Editor_Text *t, Display *disp)
{
	if (!disp->top_line) {
		DEB("REVALIDATE top_line");
		disp->top_line = t->curr;
	}

	disp->top_line_num = disp->top_line->num;

	/* TODO maybe we can have the init function do the first draw and
	   avoid this */
	if (disp->last_pos == -1) {
		DEB("Display update: setup");
		display_draw(t, disp);
		goto DISPLAY_UPDATE_DONE;
	}

	if (disp->force_redraw) {
		DEB("Force redraw.");
		display_draw(t, disp);
		disp->force_redraw = false;
		goto DISPLAY_UPDATE_DONE;
	}

	/*
	Line *last_top_line = disp->top_line;
	int last_top_line_num = disp->top_line->num;
	int text_nrows = NRIGHE - 1 - disp->pos;

	int bot_row_num = NRIGHE - 1;
	Line *bot_line = disp->top_line;
	{
		int i = disp->pos + 1;
		while (i != NRIGHE && bot_line->next) {
			bot_line = bot_line->next;
			i++;
		}
	}
	bool disp_filled = bot_line->num == bot_line_num;
	bool curs_out = curs_above || curs_below;
	const int final_pos = MSG_WIN_SIZE;
	*/

	/* First we reposition the text inside the text region of the display
	   if the cursor ended up outside the region during the last operation.
	   If the cursor is below the bottom row and the display has not yet
	   reached its maximum size, then grow the display. */

	int top_line_num = disp->top_line_num;
	int bot_line_num = display_bottom_line_num(disp);
	bool curs_above = t->curr->num < top_line_num;
	bool curs_below = t->curr->num > bot_line_num;

	disp->redraw_begin = -1;
	disp->redraw_end = -1;

	/*
	  BUGS (IMPORTANT)

	  add lines when the editor is not full size so that lots of
	  them are below. Now do End; the disp enters full size but text
	  not refreshed correctly.

	  Insert link (even single char); delete it; write a char -> crash
	  This crash happens iff the link is at the end of the line.
	*/

	if (curs_above) {
		int scroll_down_count = disp->top_line_num - t->curr->num;
		DEB("DISP Scroll down %d", scroll_down_count);

		disp->top_line = t->curr;
		disp->top_line_num = t->curr->num;
		disp->redraw_begin = disp->pos + 1;
		disp->redraw_end = disp->pos + 1 + scroll_down_count;

		DEB("last_curs_row %d", disp->last_curs_row);
		display_region_scroll_down(disp, disp->pos + 1, NRIGHE - 1,
					   scroll_down_count);
		DEB("last_curs_row %d", disp->last_curs_row);
	} else if (curs_below) {
		int scroll_up_count = t->curr->num - bot_line_num;
		DEB("CURS below: Scroll up count: %d", scroll_up_count);
		int grow_count = 0;
		if (!disp->reached_full_size) {
			grow_count = MIN(scroll_up_count,
					 disp->pos - MSG_WIN_SIZE);
			display_grow_n(disp, grow_count);
			scroll_up_count -= grow_count;
		}
		assert(scroll_up_count == 0 || disp->reached_full_size);
		//
		if (scroll_up_count) {
			DEB("Scroll up count: %d remaining", scroll_up_count);
			assert(disp->reached_full_size);
			/* The scrolling region is already set to the whole
			   text region since the display reached full size */
			for (int i = 0; i != scroll_up_count; ++i) {
				//DEB("Scroll Up");
				//display_move_curs(disp, NRIGHE - 1, 60);
				display_move_curs_row(disp, NRIGHE - 1);
				/*
				printf("* here NR-1 60");
				fflush(stdout);
				sleep(5);
				*/
				scroll_up();
				disp->top_line = disp->top_line->next;
			}
			disp->top_line_num += scroll_up_count;
			assert(disp->top_line_num == disp->top_line->num);
		}

		DEB("GC %d SUC %d", grow_count, scroll_up_count);
		disp->redraw_begin = NRIGHE - (grow_count + scroll_up_count);
		disp->redraw_end = NRIGHE;
		DEB("redraw %d:%d", disp->redraw_begin, disp->redraw_end);
	}

	if (t->text->operation == OP_PAGEUP) {
		assert(t->curr->num >= disp->top_line->num);
		if (disp->top_line->num == 1) {
			Beep();
		} else {
			assert(disp->top_line->prev);

			int disp_nrows = NRIGHE - 1 - display.pos;
			int scroll_num;
			if (disp->top_line_num > disp_nrows) {
				scroll_num = NRIGHE - 1 - display.pos - 2;
				if ((t->curr != disp->top_line)
				    && (t->curr->num >= scroll_num + 2)) {
					t->curr = disp->top_line->next;
				}
				display_region_scroll_down(disp, disp->pos + 1,
							   NRIGHE - 1,
							   scroll_num);
				for (int i = 0; ((disp->top_line->prev)
						 && (i < scroll_num)); ++i) {
					disp->top_line = disp->top_line->prev;
				}
			} else {
				scroll_num = disp->top_line->num - 1;
				display_region_scroll_down(disp, disp->pos + 1,
							   NRIGHE - 1,
							   scroll_num);
				disp->top_line = t->text->first;
				t->curr = t->text->first;
				for (int i = 0; ((t->curr->next)
						 && (i != disp_nrows - 1));
				     ++i) {
					t->curr = t->curr->next;
				}
				assert(t->curr->num ==
				       disp->top_line->num +
				       display_nrows(disp) - 1);
			}
			disp->redraw_begin = disp->pos + 1;
			disp->redraw_end = disp->pos + 1 + scroll_num;
			disp->top_line_num = disp->top_line->num;
			t->curr->pos = 0;
		}
	} else if (t->text->operation == OP_PAGEDOWN) {
		assert(t->curr->num >= disp->top_line->num);
		int disp_nrows = NRIGHE - 1 - display.pos;

		if (t->text->last->num - disp->top_line->num < disp_nrows) {
			DEB("BOH %d %d", disp->top_line->num,  t->text->last->num);
			Beep();
		} else {
			assert(disp->top_line->next);

			int scroll_num;
			int bot_num = disp->top_line_num + disp_nrows - 1;
			Line *bot_line = disp->top_line;
			for (int i = 0; i != disp_nrows - 1 && bot_line->next;
			     ++i) {
				bot_line = bot_line->next;
			}
			assert(bot_line->num == bot_num);

			scroll_num = NRIGHE - 1 - display.pos - 2;
			if (t->curr->num != bot_num) {
				t->curr = bot_line->prev;
			}
			display_region_scroll_up(disp, disp->pos + 1,
						 NRIGHE - 1, scroll_num);
			disp->top_line = bot_line->prev;

			disp->redraw_begin = NRIGHE - scroll_num;
			disp->redraw_end = NRIGHE;
			disp->top_line_num = disp->top_line->num;
			t->curr->pos = 0;
		}
	}

	/* If some lines were deleted/inserted, perform the resulting scrolling
	   and determine which lines need to be redrawn. */
	if (t->text->operation == LINE_INSERT) {
		int op_begin = display_line_row(disp, t->text->op_first_line);
		int op_end = display_line_row(disp, t->text->op_last_line) + 1;

		DEB("INS num (%d %d) row (%d %d)", t->text->op_first_line->num,
		    t->text->op_last_line->num, op_begin, op_end);
		assert(op_begin > disp->pos);

		int op_nrows = op_end - op_begin;
		/* No need to scroll down if the new lines fill the bottom of
		   the text region. */
		if (op_end < NRIGHE) {
			DEB("RSDN %d %d x%d", op_begin, NRIGHE - 1, op_nrows);
			display_region_scroll_down(disp, op_begin, NRIGHE - 1,
						   op_nrows);
		}
		disp->redraw_begin = op_begin;
		disp->redraw_end = op_end;
		DEB("Redraw %d %d", op_begin, op_end);
	} else if (t->text->operation == LINE_DELETE) {
		int del_num = t->text->op_del_num;
		int del_row = del_num - disp->top_line_num + disp->pos + 1;
		DEB("DEL num %d, row %d, curr_num %d [%c]",
		    t->text->op_del_num, del_row, t->curr->num,
		    t->curr->num < t->text->op_del_num ? 'B' : 'D');
		if (del_row < NRIGHE - 1) {
			display_region_scroll_up(disp, del_row, NRIGHE - 1, 1);
		}
                disp->redraw_begin = NRIGHE - 1;
                disp->redraw_end = NRIGHE;
	}

	assert(t->curr->num >= disp->top_line_num);
	disp->vcurs = (disp->pos + 1) + (t->curr->num - disp->top_line_num);
	disp->hcurs = t->curr->pos + 1;

	display_refresh_all(t, disp);

 DISPLAY_UPDATE_DONE:
	textbuf_clear_op(t->text);

	disp->last_pos = disp->pos;
	display_move_curs(disp, disp->vcurs, disp->hcurs);
	display_setcolor(disp, t->curr_col);
	//fflush(stdout);
}

static void display_window_changed(Editor_Text *t, Display *disp)
{
	DEB("Terminal window resized (WINCH) to %dx%d", NCOL, NRIGHE);
	if (t->term_nrows < NRIGHE) {
		/* the terminal has grown vertically */
		/*
		  int extra_rows = NRIGHE - t->term_nrows;
		*/

		/*
		  display.pos += extra_rows;
		  display.vcurs += extra_rows;
		*/
		init_window();
		if (disp->reached_full_size) {
			display_window_pop(disp);
		} else {

		}
	} else {
		int extra_rows = t->term_nrows - NRIGHE;
		if (disp->pos - extra_rows < MSG_WIN_SIZE) {
			extra_rows = disp->pos - MSG_WIN_SIZE;
			//display.pos = MSG_WIN_SIZE;
		}
		disp->pos -= extra_rows;
		disp->vcurs -= extra_rows;
	}
	t->term_nrows = NRIGHE;
	//display_refresh_all(t, &display);
	/* TODO we probably don't need to redraw the whole display. Check */
	disp->force_redraw = true;
}

/***************************************************************************/
/*
 * Le funzioni di editing implementano i seguenti comandi: (Emacs like)
 *                  Ctrl-A  Vai all'inizio della riga
 *                  Ctrl-B  Sposta il cursore di un carattere a sinistra
 *                  Ctrl-C  Abort
 *                  Ctrl-D  Cancella il carattere sotto al cursore
 *                  Ctrl-E  Vai alla fine della riga
 *                  Ctrl-F  Sposta il cursore di un carattere a destra
 *                  Ctrl-K  Cancella dal cursore fino alla fine della riga
 *                  Ctrl-L  Refresh text
 *                  Ctrl-N  Down
 *                  Ctrl-P  Up
 *                  Ctrl-T  Save to file
 *                  Ctrl-U  Cancella tutta la riga corrente.
 *                  Ctrl-V  Page Down
 *                  Ctrl-W  Cancella una parola all'indietro
 *                  Ctrl-X  Save and exit
 *                  Ctrl-Y  Yank (paste from copy buffer)
 *                  Meta-B  bg blu
 *                  Meta-C  bg cyan
 *                  Meta-G  bg green
 *                  Meta-M  bg magenta
 *                  Meta-N  bg black
 *                  Meta-P  pilcrow sign
 *                  Meta-R  bg red
 *                  Meta-S  section sign
 *                  Meta-W  bg gray (white)
 *                  Meta-Y  bg yellow
 *                  Meta-a  Same as Ctrl-C
 *                  Meta-b  fg blu
 *                  Meta-c  fg cyan
 *                  Meta-d  Cancella la parola a destra del cursore
 *                  Meta-f  blink
 *                  Meta-g  fg green
 *                  Meta-h  bold
 *                  Meta-i  Insert Text
 *                  Meta-m  fg magenta
 *                  Meta-n  fg black
 *                  Meta-o  ring
 *                  Meta-r  fg red
 *                  Meta-s  sharp s
 *                  Meta-t  tilde
 *                  Meta-u  underscore
 *                  Meta-v  Page Up
 *                  Meta-w  fg gray (white)
 *                  Meta-y  fg yellow
 *                  Meta-'  acute
 *                  Meta-^  circumflex
 *                  Meta-"  diaeresis
 *                  Meta-`  grave
 *                  Meta-$  money
 *                  Meta-(  parent
 *                  Meta-~  tilde
 *                  Meta-,  cedilla
 *                  Meta-/  slash
 *                  Meta-%  division sign
 *                  Meta-!  inv excl mark
 *                  Meta-?  inv quot mark
 *                  Meta-<  left guillem
 *                  Meta->  right guillem
 *                  Meta-|  broken bar
 *                  Meta--  soft hyphen
 *                  Meta-_  macron
 *                  Meta-+  plus/minus
 *                  Meta-.  middle dot
 *                  Meta-*  mult sign
 *                  F1      Visualizza l'help
 *                  F2      Applica colore e attributi correnti
 *                  F3      Applica col e attr fino a spazio successivo
 *                  F4      Applica col e attr a tutta la riga
 *                  Ins     Insert -> Overwrite -> ASCII Art
 *                  Home
 *                  End
 *                  TAB, Enter, INS, BS, DEL, UP, DOWN, RIGHT, LEFT,
 *                  PageUp, PageDown,
 */

/***************************************************************************/
/*
 * get_text_full() : editor interno full screen per testi.
 *
 * Prende in input dall'utente un testo lungo al piu' 'max_linee' righe su
 * 'max_col' colonne e lo inserisce nella struttura 'txt'. Il testo termina
 * quando si lascia una riga vuota oppure si riempono tutte le righe a
 * disposizione.
 * Se abortp == true, immettendo su una riga vuota la stringa "ABORT"
 * l'editing viene interrotto.
 * Il testo viene formattato  automaticamente mandando a capo le parole che
 * non ci stanno in fondo alla riga.
 * Color e` il colore iniziale del cursore. Se color e` zero usa C_DEFAULT.
 *
 * Valori di ritorno: EDIT_ABORT : Abort;
 *                             0 : Nessun testo;
 *                             N : Numero righe scritte.
 */
int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp,
		  int color, Metadata_List *mdlist)
{
	Editor_Text t;
        char prompt_tmp;
        int ret;

#ifdef EDITOR_DEBUG
	test();
#endif

	/* The abortp option is not available (and useless) when the option */
	/* HAVE_CTI is set. The arg is keept only for compatibility with    */
	/* the old editor. Maybe we can consider eliminating it completely  */
	assert(abortp == false);

	/* Inizializza schermo */
	cti_term_init();
	init_window();

	t.max = (max_col > MAX_EDITOR_COL) ? MAX_EDITOR_COL : max_col;
	t.term_nrows = NRIGHE;

	prompt_tmp = prompt_curr;
	prompt_curr = P_EDITOR;

	/* Initialize text to be edited */
	t.text = textbuf_new();
	textbuf_init(t.text);
	t.curr = t.text->first;

	/* Initialize display data */
#define EDITOR_DISPLAY_INITIAL_SIZE 1
	display = display_init(t.term_nrows, EDITOR_DISPLAY_INITIAL_SIZE,
			       t.text->first);

	/* Initialize an empty kill buffer */
	t.killbuf = textbuf_new();
	assert(t.killbuf->first == NULL && t.killbuf->last == NULL);
	assert(t.killbuf->lines_count == 0);

	t.buf_pasted = false;
        t.copy = false;

	t.insert = MODE_INSERT;
	t.curs_col = color ? color : C_DEFAULT;
	t.curr_col = t.curs_col;

        /* Inizializza il metadata */
        md_init(mdlist);
        t.mdlist = mdlist;

	push_color();
	//display_setcolor(&display, t.curr_col);

	/* Inserisce nell'editor il testo del quote o hold */
	text2editor(&t, txt, color, max_col);
	txt_clear(txt);

	/* Visualizza l'header dell'editor ed eventualmente il testo */
	t.curr = t.text->last;
	/*
	display_draw_head(&t, &display);
	display_refresh(&t, 0);
	*/

#ifdef EDITOR_DEBUG
	console_init();
	DEB("t.max = %d", t.max);
#endif

	bool fine = false;
	do {
		ret = get_line_wrap(&t, true);
		pull_color();

		switch(ret) {
		case EDIT_ABORT:
			cti_ll();
			printf(ERASE_TO_EOL "\rAbort.\n");
			fine = true;
			break;

		case EDIT_TIMEOUT:
			cti_ll();
			printf(ERASE_TO_EOL "\rPost timeout.\n");
			fine = true;
			break;

		case EDIT_DONE:
			cti_ll();
			putchar('\n');
			fine = true;
			break;
		}
		/* TODO what is this while for? */
	} while ((t.text->lines_count < max_linee) && (!fine));

	/* TODO: how can ret be EDIT_NEWLINE? it can't... what's the point
         	 of the next block?      */
	if (ret == EDIT_NEWLINE) {
		refresh_line_curs(t.curr->str, t.curr->str + t.curr->pos);

		ret = get_line_wrap(&t, false);
		if (ret == EDIT_DONE) {
			cti_ll();
			putchar('\n');
		}
	}

	Editor2CML(t.text->first, txt, color, t.mdlist);
	Editor_Free(&t);
	prompt_curr = prompt_tmp;

	if (display.reached_full_size) {
		display_window_pop(&display);
	}

	{
		int first, last;
		assert(debug_get_winstack_index() == 0);
		debug_get_current_win(&first, &last);
		assert(first == 0 && last == NRIGHE - 1);
	}

	cti_term_exit();

	return ret;
}

/****************************************************************************/
/*
 * Prende un testo dallo stdinput, lungo 'max'.
 * Il testo viene editato su una singola riga, e se e' piu' lungo dello
 * schermo scrolla.
 */
static int get_line_wrap(Editor_Text *t, bool wrap)
/* TODO why is arg wrap not used here? the two calls to this function have
        wrap = true and wrap = false, why? */
{
        int status = GL_NORMAL;    /* stato dell'editor             */
#ifdef EDITOR_DEBUG
	int last_key = 0;
#endif

	IGNORE_UNUSED_PARAMETER(wrap);

	/*
	line_refresh(&display, t->curr, display.vcurs, 0);
	display_setcolor(&display, t->curr_col);
	*/

        do {
		display_update(t, &display);

#ifdef EDITOR_DEBUG
		textbuf_sanity_check(t->text);
		textbuf_sanity_check(t->killbuf);
		sanity_checks(t);
		console_set_status("[l%p r%2d] stat %d k %3d",
				   (void *)t->curr, t->curr->num, status,
				   last_key);
		console_update(t);

		// bring back the cursor in its editing position
		display_move_curs(&display, display.vcurs, display.hcurs);
#endif

		/* inkey_sc() needs to read display data for now */
		gl_Editor_Pos = display.pos;
		gl_Editor_Hcurs = display.hcurs;
		gl_Editor_Vcurs = display.vcurs;
		int key = inkey_sc(true);
		bool addchar = false;

#ifdef EDITOR_DEBUG
		last_key = key;
		for (int k = 0; k < 20; k++) {
			if (key == Key_F(k)) {
				DEB("F%d pressed [code %d]", k, key);
			}
		}
#endif

		if (key == Key_Window_Changed) {
			display_window_changed(t, &display);
			continue;
		}

		if (key == Key_Timeout) {
			return EDIT_TIMEOUT;
		}

		switch(key) {
		case Key_CR:
		case Key_LF:
			Editor_Key_Enter(t);
			break;

		case Key_TAB:
			Editor_Key_Tab(t);
			break;

			/* Deleting characters */
		case Key_INS:
		case META('I'):
			t->insert = (t->insert + 1) % 3;
			if (t->insert == MODE_ASCII_ART
			    && t->curr->pos == t->max - 1) {
				t->curr->pos--;
			}
			break;
		case Key_BS:
			Editor_Key_Backspace(t);
			break;
		case Key_DEL:
		case Ctrl('D'):
			Editor_Key_Delete(t);
			break;

			/* Deleting Words */
		case Ctrl('W'):
			Editor_Delete_Word(t);
			break;
		case META('d'): /* Cancella la parola a destra del cursore */
			Editor_Delete_Next_Word(t);
			break;

			/* Cursor Motion */
		case Key_UP:
		case Ctrl('P'):
			Editor_Key_Up(t);
			break;
		case Key_DOWN:
		case Ctrl('N'):
			Editor_Key_Down(t);
			break;
		case Key_RIGHT:
		case Ctrl('F'):
			Editor_Key_Right(t);
			break;
		case Key_LEFT:
		case Ctrl('B'):
			Editor_Key_Left(t);
			break;

			/* Extended Motion */
		case Key_PAGEUP:
		case META('v'):
			Editor_PageUp(t);
			break;
		case Key_PAGEDOWN:
		case Ctrl('V'):
			Editor_PageDown(t);
			break;
		case Key_HOME:
		case Key_F(5):
			/* case META('<'): */
			t->curr = t->text->first;
			t->curr->pos = 0;
			break;
		case Key_END:
		case Key_F(6):
			/* case META('>'): */
			t->curr = t->text->last;
			t->curr->pos = t->curr->len;
			if (t->insert == MODE_ASCII_ART
			    && t->curr->pos == t->max - 1) {
				t->curr->pos--;
			}
			break;
		case Ctrl('A'): /* Vai inizio riga */
			t->curr->pos = 0;
			break;
		case Ctrl('E'): /* Vai fine riga */
			t->curr->pos = t->curr->len;
			break;

			/* Abort */
		case Ctrl('C'):
		case META('a'):
			if (Editor_Ask_Abort(t))
				return EDIT_ABORT;
			break;

                case META('i'): /* Insert metadata */
                        Editor_Insert_Metadata(t);
                        break;
                case Ctrl('T'): /* Save text to file */
			if (local_client) {
				Editor_Save_Text(t);
			}
                        break;
		case Ctrl('K'): /* Kill line */
			Editor_Kill_Line(t);
                        break;
		case Ctrl('Y'):
		case Ctrl('G'):
			Editor_Yank(t);
			break;

		case Ctrl('L'): /* Rivisualizza il contenuto dello schermo */
			display.force_redraw = true;
			break;

		case Key_F(1): /* Visualizza l'Help */
			help_edit(t);
			break;

		case Key_F(2): /* Applica colore e attributi correnti */
			editor_apply_attr(t);
			break;

		case Key_F(3): /* Applica colore e attributi correnti fino
                                  allo spazio successivo. */
			editor_apply_attr_word(t);
			break;

		case Key_F(4): /* Applica colore e attributi correnti a
                                  tutta la riga                          */
			editor_apply_attr_line(t);
			break;

#ifdef EDITOR_DEBUG
		case Key_F(12):
			console_toggle();
			console_update(t);
			break;
#endif

		case Ctrl('U'): /* Cancella tutto il testo della riga */
                        Editor_Free_MD(t, t->curr);
			line_truncate_at(t->curr, 0);
			t->curr->pos = 0;
			break;

		case Ctrl('X'):
			return EDIT_DONE;

			/* ANSI Colors : Foreground */
		case META('b'):
			Editor_Fg_Color(t, COL_BLUE);
			break;
		case META('c'):
			Editor_Fg_Color(t, COL_CYAN);
			break;
		case META('g'):
			Editor_Fg_Color(t, COL_GREEN);
			break;
		case META('m'):
			Editor_Fg_Color(t, COL_MAGENTA);
			break;
		case META('n'):
			Editor_Fg_Color(t, COL_BLACK);
			break;
		case META('r'):
			Editor_Fg_Color(t, COL_RED);
			break;
		case META('w'):
			Editor_Fg_Color(t, COL_GRAY);
			break;
		case META('y'):
			Editor_Fg_Color(t, COL_YELLOW);
			break;
			/* ANSI Colors : Background */
		case META('B'):
			Editor_Bg_Color(t, COL_BLUE);
			break;
		case META('C'):
			Editor_Bg_Color(t, COL_CYAN);
			break;
		case META('G'):
			Editor_Bg_Color(t, COL_GREEN);
			break;
		case META('M'):
			Editor_Bg_Color(t, COL_MAGENTA);
			break;
		case META('N'):
			Editor_Bg_Color(t, COL_BLACK);
			break;
		case META('R'):
			Editor_Bg_Color(t, COL_RED);
			break;
		case META('W'):
			Editor_Bg_Color(t, COL_GRAY);
			break;
		case META('Y'):
			Editor_Bg_Color(t, COL_YELLOW);
			break;
			/* ANSI Colors : Attributes */
		case META('f'):
			Editor_Attr_Toggle(t, ATTR_BLINK);
			break;
		case META('h'):
			Editor_Attr_Toggle(t, ATTR_BOLD);
			break;
		case META('u'):
			Editor_Attr_Toggle(t, ATTR_UNDERSCORE);
			break;

		/* Extended charset keybindings */
		case META('\''):
			status = GL_ACUTE;
			break;
		case META('^'):
			status = GL_CIRCUMFLEX;
			break;
		case META('\"'):
			status = GL_DIAERESIS;
			break;
		case META('`'):
			status = GL_GRAVE;
			break;
		case META('$'):
			status = GL_MONEY;
			break;
		case META('('):
			status = GL_PARENT;
			break;
		case META('~'):
			status = GL_TILDE;
			break;
		case META(','):
			status = GL_CEDILLA;
			break;
		case META('/'):
			status = GL_SLASH;
			break;
		case META('o'):
			status = GL_RING;
			break;
		case META('S'):
			key = Key_SECTION_SIGN;
			addchar = true;
			break;
		case META('s'):
			key = Key_SHARP_S;
			addchar = true;
			break;
                case META('t'):
                        key = '~';
                        addchar = true;
                        break;
		case META('P'):
			key = Key_PILCROW_SIGN;
			addchar = true;
			break;
		case META('%'):
			key = Key_DIVISION_SIGN;
			addchar = true;
			break;
		case META('!'):
			key = Key_INV_EXCL_MARK;
			addchar = true;
			break;
		case META('?'):
			key = Key_INV_QUOT_MARK;
			addchar = true;
			break;
		case META('<'):
			key = Key_LEFT_GUILLEM;
			addchar = true;
			break;
		case META('>'):
			key = Key_RIGHT_GUILLEM;
			addchar = true;
			break;
		case META('|'):
			key = Key_BROKEN_BAR;
			addchar = true;
			break;
		case META('-'):
			key = Key_SOFT_HYPHEN;
			addchar = true;
			break;
		case META('_'):
			key = Key_MACRON;
			addchar = true;
			break;
		case META('+'):
			key = Key_PLUS_MINUS;
			addchar = true;
			break;
		case META('.'):
			key = Key_MIDDLE_DOT;
			addchar = true;
			break;
		case META('*'):
			key = Key_MULT_SIGN;
			addchar = true;
			break;

			/* Add a character */
		default:
			IF_ISMETA(key) {
				;
			} else if ((isascii(key) && isprint(key))
				   || is_isoch(key)) {
				addchar = true;
			}
		}
                if (key != Ctrl('K')) {
                        t->copy = false;
		}
		if (status != GL_NORMAL) {
			key = gl_extchar(status);
			DEB("Extended glyph %d key %d", status, key);
			if (key != -1) {
				addchar = true;
			} else {
				Beep();
			}
			status = GL_NORMAL;
		}
		if (addchar) {
			Editor_Putchar(t, key);
		}
	} while (t->curr->len < t->max);

	return EDIT_NULL;
}

/****************************************************************************/
/* Apply current color/attr to the current character */
static void editor_apply_attr(Editor_Text *t)
{
	if ((t->curr->pos == t->curr->len)
	    || (line_get_mdnum(t->curr, t->curr->pos))) {
		Beep();
		return;
	}
	line_replace_attr_idx(t->curr, t->curr->pos, t->curr_col);
	if (t->insert != MODE_ASCII_ART) {
		t->curr->pos++;
	}
}

/* Apply current color/attr up to the next space */
static void editor_apply_attr_word(Editor_Text *t)
{
	if (t->curr->pos == t->curr->len) {
		Beep();
		return;
	}
	int i = t->curr->pos;
	while ((i < t->curr->len) && (t->curr->str[i] != ' ')
	       && (line_get_mdnum(t->curr, i) == 0)) {
		line_replace_attr_idx(t->curr, i, t->curr_col);
		i++;
	}
	if (t->insert != MODE_ASCII_ART) {
		t->curr->pos = i;
	}
}

/* Apply current color/attr to the whole line */
static void editor_apply_attr_line(Editor_Text *t)
{
	if (t->curr->len == 0) {
		Beep();
		return;
	}
	for (int i = 0; i != t->curr->len; i++) {
		if (line_get_mdnum(t->curr, i) == 0) {
			line_replace_attr_idx(t->curr, i, t->curr_col);
		}
	}
}

/* Insert character 'c' at current text position, using the current color */
static void Editor_Putchar(Editor_Text *t, int c)
{
	if (t->insert == MODE_INSERT) {
		line_expand_index(t->curr, t->curr->pos);
	} else {
		if (t->insert == MODE_ASCII_ART
		    && t->curr->pos >= t->max - 1) {
			/* the editor should not allow this state */
			assert(false);
			Beep();
			return;
		}
		/* if cursor past end of line, grow line by one char even in
		 * overwrite and ASCII-Art modes. */
		if (t->curr->pos == t->curr->len) {
			line_expand_back(t->curr);
		}
	}
	line_replace_char_idx(t->curr, t->curr->pos, c, t->curs_col);

	if (t->insert != MODE_ASCII_ART) {
		t->curr->pos++;
	}

	if (t->curr->len >= t->max) {
		Editor_Wrap_Word(t);
	}
}

/* <Enter>: Break the line at current cursor pos. and go to next row start */
static void Editor_Key_Enter(Editor_Text *t)
{
	textbuf_break_line(t->text, t->curr, t->curr->pos);
	t->curr = t->curr->next;
}

/* Effettua un backspace cancellando gli eventuali allegati */
static void Editor_Key_Backspace(Editor_Text *t)
{
	assert(t->curr->pos == 0
	       || t->curr->pos >= t->curr->len
	       || (line_get_mdnum(t->curr, t->curr->pos) == 0)
	       || (line_get_mdnum(t->curr, t->curr->pos)
		   != line_get_mdnum(t->curr, t->curr->pos - 1)));
        int mdnum;
        if ((t->curr->pos)
            && ( (mdnum = line_get_mdnum(t->curr, t->curr->pos - 1)))) {
                do {
                        Editor_Backspace(t);
                } while (t->curr->pos
			 && (mdnum == line_get_mdnum(t->curr, t->curr->pos - 1)));
                mdop_delete(t->mdlist, mdnum);
        } else {
                Editor_Backspace(t);
	}
}

/* Effettua un backspace */
static void Editor_Backspace(Editor_Text *t)
{
	if (t->curr == t->text->first && t->curr->pos == 0) {
		Beep();
		return;
	}

	if (t->curr->pos == 0) {
		/* cursor at line start: merge with line above */
		int above_len = t->curr->prev->len;
		Line *below = t->curr;

		/* move cursor to line above */
		assert(t->curr->prev);
		t->curr = t->curr->prev;

		Merge_Lines_Result result = textbuf_merge_lines(t->text,
					     t->curr, t->curr->next, t->max);
		if (result == MERGE_EXTRA_SPACE) {
			t->curr->pos = above_len + 1;
		} else if (result == MERGE_ABOVE_EMPTY) {
			/* The above line was deleted */
			t->curr->pos = 0;
			t->curr = below;
		} else {
			t->curr->pos = above_len;
		}
		/* Note: MERGE_NOTHING occurs when the starting word below
		   doesn't fit above; move cursor to end of line above.    */
	} else {
		/* delete previous character */
		t->curr->pos--;
		line_remove_index(t->curr, t->curr->pos);
	}
	/* no refresh necessary for MERGE_NOTHING */
	/* line_refresh(t->curr, display.vcurs, t->curr->pos); enough if char
	   deleted without merging */
}

/* Cancella il carattere sottostante al cursore, eliminando gli allegati */
static void Editor_Key_Delete(Editor_Text *t)
{
        int mdnum = line_get_mdnum(t->curr, t->curr->pos);

        if (mdnum) {
                do {
                        Editor_Delete(t);
                } while (mdnum == line_get_mdnum(t->curr, t->curr->pos));
		mdop_delete(t->mdlist, mdnum);
        } else {
                Editor_Delete(t);
	}
}

/* Cancella il carattere sottostante al cursore. */
static void Editor_Delete(Editor_Text *t)
{
	DelCharResult result = textbuf_delete_char(t->text, t->curr,
						  t->curr->pos, t->max);
	if (result == DEL_FAIL) {
		Beep();
		return;
	}
	if (result == DEL_LINE) {
		t->curr = t->curr->next;
		t->curr->pos = 0;
	}

	/* NOTE: refresh useless if result MERGE_NOTHING */
	/* ie. not enough space to move first word below */
	/* also line_refresh(t->curr, display.vcurs, t->curr->pos); is
	 * sufficient if there was no merfe (pos < len ) */
}

/* Cancella la parola precedente al cursore. */
static void Editor_Delete_Word(Editor_Text *t)
{
	if (t->curr->pos == 0) {
		Beep();
		return;
	}
	/* TODO: move in a find_word*() function */
	int tmp = t->curr->pos;
	while ((t->curr->pos != 0) && (t->curr->str[t->curr->pos-1] == ' ')) {
		t->curr->pos--;
	}
	while ((t->curr->pos != 0) && (t->curr->str[t->curr->pos-1] != ' ')) {
		t->curr->pos--;
	}
	line_remove_range(t->curr, t->curr->pos, tmp);
}

/* Cancella la parola successiva al cursore. */
static void Editor_Delete_Next_Word(Editor_Text *t)
{
	/* TODO: move in a find_word*() function */
	int tmp = t->curr->pos;
	while ((t->curr->pos != t->curr->len)
	       && (t->curr->str[t->curr->pos+1] == ' ')) {
		t->curr->pos++;
	}
	while ((t->curr->pos != t->curr->len)
	       && (t->curr->str[t->curr->pos+1] != ' ')) {
		t->curr->pos++;
	}
	while ((t->curr->pos != t->curr->len)
	       && (t->curr->str[t->curr->pos+1] == ' ')) {
		t->curr->pos++;
	}
	line_remove_range(t->curr, tmp, t->curr->pos);
	t->curr->pos = tmp;
}

/*
 * If the current line is not empty, erase its content, otherwise eliminate
 * the line.
 */
static void Editor_Kill_Line(Editor_Text *t)
{
	if ((t->curr->len == 0) && t->curr->next) {
		Line *next_line = t->curr->next;
		textbuf_delete_line(t->text, t->curr);
		t->curr = next_line;
		t->curr->pos = 0;
	} else if (t->curr->pos == t->curr->len) {
		Editor_Delete(t);
	} else {
		Editor_Copy_To_Kill_Buffer(t);
		/* Eliminate the text from the cursor to the end of line */
		line_truncate_at(t->curr, t->curr->pos);
	}
	t->copy = true;
}

/* Process <Y>ank */
static void Editor_Yank(Editor_Text *t)
{
        if (t->killbuf->lines_count == 0) { /* copy buffer is empty */
                return;
	}
	/* unless the cursor sits in an empty line, insert a new empty
	 * at the cursor's location (breaking the current line if needed) */
	if (t->curr->len) {
		bool cursor_at_line_start = (t->curr->pos == 0);
		bool cursor_at_line_end = (t->curr->pos == t->curr->len);

		textbuf_break_line(t->text, t->curr, t->curr->pos);
		t->curr = t->curr->next;

		if (cursor_at_line_start) {
			t->curr = t->curr->prev;
			t->curr->pos = 0;
			t->curr->next->pos = 0;
			DEB("Yank-1");
		} else if (cursor_at_line_end) {
			/* do nothing */
			DEB("Yank-2");
		} else {
			textbuf_insert_line_above(t->text, t->curr);
			t->curr = t->curr->prev;
			t->curr->pos = 0;
			t->curr->next->pos = 0;
			DEB("Yank-3");
		}
	}

	assert(t->curr->len == 0);
	assert(t->curr->pos == 0);
        for (Line *src = t->killbuf->first; src; ) {
		line_copy_all(t->curr, src);
		t->curr->pos = src->len;
                src = src->next;
                if (src) {
			textbuf_insert_line_below(t->text, t->curr);
			t->curr = t->curr->next;
			t->curr->pos = 0;
                }
        }
        t->curr->pos = t->curr->len;
	t->buf_pasted = true;
}

static void Editor_Key_Tab(Editor_Text *t)
{
	if (t->insert == MODE_ASCII_ART) {
		do {
			Editor_Key_Right(t);
		} while (((t->curr->pos) % CLIENT_TABSIZE));
		return;
	}
	if ((t->max - t->curr->pos) < CLIENT_TABSIZE) {
		textbuf_break_line(t->text, t->curr, t->curr->pos);
	        t->curr = t->curr->next;
	} else {
		do {
			Editor_Putchar(t, ' ');
		} while (((t->curr->pos) % TABSIZE));
	}
}

static void Editor_Key_Up(Editor_Text *t)
{
	if (t->curr->prev == NULL) {
		Beep();
		return;
	}
	t->curr = t->curr->prev;
	t->curr->pos = t->curr->next->pos;
	if (t->insert == MODE_ASCII_ART && t->curr->pos >= t->curr->len) {
		line_extend_to_pos(t->curr, t->curr->pos);
	} else if (t->curr->pos > t->curr->len) {
		t->curr->pos = t->curr->len;
	}

        /* Se finisco in un oggetto metadata, vai indietro */
	int mdnum = line_get_mdnum(t->curr, t->curr->pos);
        if (mdnum) {
		do {
                        Editor_Curs_Right(t);
                } while(mdnum == line_get_mdnum(t->curr, t->curr->pos));
	}
}

static void Editor_Key_Down(Editor_Text *t)
{
	if (t->insert == MODE_ASCII_ART) {
		if (!t->curr->next) {
			textbuf_insert_line_below(t->text, t->curr);
		}
		t->curr = t->curr->next;
		t->curr->pos = t->curr->prev->pos;
		if (t->curr->pos >= t->curr->len) {
			line_extend_to_pos(t->curr, t->curr->pos);
		}
	} else if (t->curr->next) {
		t->curr = t->curr->next;
		t->curr->pos = t->curr->prev->pos;
		if (t->curr->pos > t->curr->len) {
			t->curr->pos = t->curr->len;
		}
        } else {
                Beep();
                return;
        }

        /* Se finisco in un oggetto metadata, vai indietro */
	int mdnum = line_get_mdnum(t->curr, t->curr->pos);
        if (mdnum) {
                do {
                        Editor_Curs_Right(t);
                } while(mdnum == line_get_mdnum(t->curr, t->curr->pos));
	}
}

/* Key right: skips metadata objects */
static void Editor_Key_Right(Editor_Text *t)
{
        int mdnum;

        if ((t->curr->pos < t->curr->len)
            && ( (mdnum = line_get_mdnum(t->curr, t->curr->pos)))) {
                do {
                        Editor_Curs_Right(t);
                } while(mdnum == line_get_mdnum(t->curr, t->curr->pos));
	} else {
                Editor_Curs_Right(t);
	}
}

/* Push cursor right one character */
static void Editor_Curs_Right(Editor_Text *t)
{
	if (t->insert == MODE_ASCII_ART) {
		if (t->curr->pos < t->curr->len - 1) {
			t->curr->pos++;
		} else if (t->curr->len >= t->max - 1) {
			if (!t->curr->next) {
				textbuf_insert_line_below(t->text, t->curr);
			}
			t->curr = t->curr->next;
			t->curr->pos = 0;
		} else {
			t->curr->pos++;
			line_extend_to_pos(t->curr, t->curr->pos);
		}
	} else {
		if (t->curr->pos < t->curr->len) {
			++t->curr->pos;
		} else if (t->curr->next) {
			t->curr = t->curr->next;
			t->curr->pos = 0;
		} else {
			Beep();
		}
	}
}

/* Key left: skips metadata objects */
static void Editor_Key_Left(Editor_Text *t)
{
        int mdnum, mdnum1;

        mdnum = line_get_mdnum(t->curr, t->curr->pos);
        if ((t->curr->pos)
            && ( (mdnum1 = line_get_mdnum(t->curr, t->curr->pos - 1))
                 != mdnum) && mdnum1) {
                do {
                        Editor_Curs_Left(t);
                } while ((t->curr->pos) &&
                   (mdnum1 == line_get_mdnum(t->curr, t->curr->pos - 1)));
        } else {
                Editor_Curs_Left(t);
	}
}

/* Push cursor left one character */
static void Editor_Curs_Left(Editor_Text *t)
{
	if (t->curr->pos) {
		--t->curr->pos;
	} else if (t->curr->prev) {
		t->curr = t->curr->prev;
		t->curr->pos = t->curr->len;
		if ((t->insert == MODE_ASCII_ART) && (t->curr->pos > 0)) {
			t->curr->pos--;
		}
	} else {
		Beep();
	}
}

/* Va alla pagina precedente */
static void Editor_PageUp(Editor_Text *t)
{
	t->text->operation = OP_PAGEUP;
}

/* Va alla pagina successiva */
static void Editor_PageDown(Editor_Text *t)
{
	t->text->operation = OP_PAGEDOWN;
}

/*
 * Copies the text in the current line (t->curr) from the cursor position to
 * the end into a new line that is appended to the kill buffer. The current
 * line is not modified.
 */
static void Editor_Copy_To_Kill_Buffer(Editor_Text *t)
{
        if (t->copy == false) {
                Editor_Free_Copy_Buffer(t);
	}

	Line *line = textbuf_append_new_line(t->killbuf);

	/* copy the contents of current line starting from cursor */
	line_copy_from(line, t->curr, t->curr->pos);
}

/*
 * Sposta l'ultima parola della riga corrente alla riga successiva se c'e`
 * posto, altrimenti in una nuova riga che inserisce.
 * Returns true if a line was added below the current line, otherwise false.
 */
static int Editor_Wrap_Word(Editor_Text *t)
{
	/*
	  TODO
	  this function does too many things: decide what to wrap, and
	  wrap. split it into textbuf_wrap*() functions
	*/

	bool line_was_added = false;

	/* Find the starting position of the last word */
	const WordPos last_word = find_last_word(t->curr);
	int first = last_word.first;
	int last = last_word.last;
	int first_blank = last_word.prev_blank;

	if (last - first == t->max - 1) {
		/* monster word; split at cursor and continue below */
		first = t->curr->pos - 1;
		last = t->curr->len - 1;
		first_blank = first;
	}

#if 0
	if (last - first == t->max - 2 && t->curr->len == t->max
		   && t->curr->str[t->curr->pos] == ' ') {
		assert(t->curr->pos == t->curr->len - 1);
		/* insert the space in the line below as if it were a word */
		first = t->curr->pos - 1;
		last = t->curr->pos - 1;
		first_blank = first;
	}
#endif

	if (last + 1 < t->max && t->curr->pos < t->curr->len) {
		/* if the line ends with a space, just eliminate it;
		   there's no need to wrap the word */
		line_truncate_at(t->curr, t->max - 1);
		return line_was_added;
	}

	if (last == -1) {
		/* the line is blank and the user tries to insert one last
		   space: just move that space on the line below */
		first = t->curr->len - 1;
		last = first;
		assert(t->curr->str[first] == ' ');
		first_blank = 0;
	}

	if (t->curr->next == NULL) {
		textbuf_append_new_line(t->text);
	        line_was_added = true;
	}
	Line *below = t->curr->next;
	/* if the line below begins with a non-blank char, add extra space */
	bool need_extra_space = below->len && below->str[0] != ' ';

	const int word_len = last - first + 1;
	const int total_len = word_len + need_extra_space;
	if ((below->len + total_len) >= t->max) {
		/* The word to be wrapped does not fit the next line,
		   insert an extra empty line below. */
		textbuf_insert_line_below(t->text, t->curr);
		line_was_added = true;
		below = t->curr->next;

		need_extra_space = false;
	}

	int space_attributes = C_DEFAULT;
	if (need_extra_space) {
		/* If the chars adjacent to the space have the same BG color,
		   keep it, otherwise set it to C_DEFAULT */
		if (CC_BG(t->curr->col[last])
		    == CC_BG(below->col[0])) {
			space_attributes = attr_get_color(below->col[0]);
		}
	}

	/* if the wrap was triggered by the entry of a space at the end of the
	   line and the line below is empty, make sure the space stays. */
#if 0
	if (below->len == 0
	    && (t->curr->len > last + 1) && (t->curr->str[last + 1] == ' ')) {
		last += 1;
	}
#endif

	line_insert_range_front(below, t->curr, first, word_len,
				need_extra_space, space_attributes);

	if (t->curr->pos >= first) {
		/* the cursor was in the part of the line that was wrapped */
		line_truncate_at(t->curr, first_blank);
		below->pos = t->curr->pos - first;
		if (below->pos > below->len) {
			below->pos = below->len;
		}
		t->curr = below;
		assert(below->pos <= below->len);
	} else {
		line_truncate_at(t->curr, first);
	}

	return line_was_added;
}

/* Modifica il colore del cursore */
static void Editor_Set_Color(Editor_Text *t, int c)
{
	t->curs_col = c;
	if ((t->curr_col & 0xffff) != (t->curs_col & 0xffff)) {
		t->curr_col = t->curs_col;
		display_setcolor(&display, t->curs_col & 0xffff);
		fflush(stdout);
	}
}

/* Inserisce del metadata */
static void Editor_Insert_Metadata(Editor_Text *t)
{
        int c;

	/*
	  NOTE: it is responsibility of the Editor_Insert_* functions called
	        below to execute window_pop() _before_ the text buffer is
	        modified, and anyways before returning.
	*/
        if (display.reached_full_size) {
		display_window_push(&display, 0, NRIGHE - 1);
	}
	fill_line(&display, display.pos, COL_HEAD_MD);
        make_cursor_invisible();
        cml_printf(_("<b>--- Scegli ---</b> \\<<b>f</b>>ile upload \\<<b>l</b>>ink \\<<b>p</b>>ost \\<<b>r</b>>oom \\<<b>t</b>>ext file \\<<b>u</b>>ser \\<<b>a</b>>bort"));
        do {
		/* inkey_sc() needs to read display data for now */
		gl_Editor_Pos = display.pos;
		gl_Editor_Hcurs = display.hcurs;
		gl_Editor_Vcurs = display.vcurs;
                c = inkey_sc(true);
        } while ((c == 0) || !index("aflprtu\x1b", c));

        switch (c) {
        case 'f':
                Editor_Insert_File(t);
                break;
        case 'l':
                Editor_Insert_Link(t);
                break;
        case 'p':
                Editor_Insert_PostRef(t);
                break;
        case 'r':
                Editor_Insert_Room(t);
                break;
        case 't':
                Editor_Insert_Text(t);
                break;
        case 'u':
                Editor_Insert_User(t);
                break;
        }
        make_cursor_visible();
	display.force_redraw_header = true;
}

/* Inserisce il riferimento a un post */
static void Editor_Insert_PostRef(Editor_Text *t)
{
        char roomname[LBUF], locstr[LBUF];
        char *ptr, tmpcol;
        int id, col, offset = 0;
        long local_number;

	erase_current_line();
        cml_printf("<b>--- Room [%s]:</b> ", postref_room);
        get_roomname("", roomname, true);
        if (roomname[0] == 0) {
                strcpy(roomname, postref_room);
        }
	if (roomname[0]) {
		if (roomname[0] == ':') {
			offset++;
		}
		/* TODO display */
		cti_mv(40, display.pos);
		local_number = new_long_def(" <b>msg #</b>", postref_locnum);
	}

	if (display.reached_full_size) {
		display_window_pop(&display);
	}

	if (roomname[0] == 0) {
		return;
	}

        id = md_insert_post(t->mdlist, roomname, local_number);

        tmpcol = t->curs_col;
        col = COLOR_ROOM;
        ATTR_SET_MDNUM(col, id);
        Editor_Set_Color(t, col);
        for (ptr = roomname+offset; *ptr; ptr++) {
                Editor_Putchar(t, *ptr);
	}

        col = COLOR_ROOMTYPE;
        ATTR_SET_MDNUM(col, id);
        Editor_Set_Color(t, col);
        if (offset) {
                Editor_Putchar(t, ':');
	} else {
                Editor_Putchar(t, '>');
	}

        if (local_number) {
                col = COLOR_LOCNUM;
                ATTR_SET_MDNUM(col, id);
                Editor_Set_Color(t, col);
                Editor_Putchar(t, ' ');
                Editor_Putchar(t, '#');
                sprintf(locstr, "%ld", local_number);
                for (ptr = locstr; *ptr; ptr++) {
                        Editor_Putchar(t, *ptr);
		}
        }

        Editor_Set_Color(t, tmpcol);
}

/* Inserisce il riferimento a una room */
static void Editor_Insert_Room(Editor_Text *t)
{
        char roomname[LBUF];
        const char *ptr;
	char tmpcol;
        int id, col;

	erase_current_line();
        cml_printf("<b>--- Room [%s]:</b> ", postref_room);
        get_roomname("", roomname, true);
        if (roomname[0] == 0) {
                strcpy(roomname, postref_room);
        }
        if (display.reached_full_size) {
		display_window_pop(&display);
	}
        if (roomname[0] == 0) {
		return;
	}

        id = md_insert_room(t->mdlist, roomname);

        tmpcol = t->curs_col;

	int offset = 0;
	if (roomname[0] == ':') { /* Blog room */
		col = COLOR_ROOMTYPE;
		ATTR_SET_MDNUM(col, id);
		Editor_Set_Color(t, col);
		for (ptr = blog_display_pre; *ptr; ptr++) {
			Editor_Putchar(t, *ptr);
		}
		offset = 1;
        }

        col = COLOR_ROOM;
        ATTR_SET_MDNUM(col, id);
        Editor_Set_Color(t, col);
        for (ptr = roomname + offset; *ptr; ptr++) {
                Editor_Putchar(t, *ptr);
	}

        col = COLOR_ROOMTYPE;
        ATTR_SET_MDNUM(col, id);
        Editor_Set_Color(t, col);
        Editor_Putchar(t, '>');

        Editor_Set_Color(t, tmpcol);
}

/* Inserisce il riferimento a utente */
static void Editor_Insert_User(Editor_Text *t)
{
        char username[LBUF];
        char *ptr, tmpcol;
        int id, col;

	erase_current_line();
        cml_printf("<b>--- Utente [%s]: </b> ", last_profile);
        get_username("", username);
        if (username[0] == 0) {
                strncpy(username, last_profile, MAXLEN_UTNAME);
        }

	if (display.reached_full_size) {
		display_window_pop(&display);
	}

	if (username[0] == 0) {
		return;
	}

        id = md_insert_user(t->mdlist, username);

        tmpcol = t->curs_col;
        col = COLOR_USER;
        ATTR_SET_MDNUM(col, id);
        Editor_Set_Color(t, col);
        for (ptr = username; *ptr; ptr++) {
                Editor_Putchar(t, *ptr);
	}

        Editor_Set_Color(t, tmpcol);
}

/* Upload di un file */
static void Editor_Insert_File(Editor_Text *t)
{
        char buf[LBUF], path[LBUF], filename[LBUF];
        int id, tmpcol, filecol, c=0;
        char *ptr, *fullpath;
        size_t pathlen;
        FILE *fp;
        struct stat filestat;
        unsigned long filenum, len, flags;

	if (local_client) {
		erase_current_line();
        } else {

                display_setcolor(&display, COL_HEAD_ERROR);
		erase_current_line();
                cml_printf(_(
" *** Server il client locale per l'upload dei file.    -- Premi un tasto --"
			     ));
                while (c == 0) {
                        c = getchar();
		}
		if (display.reached_full_size) {
			display_window_pop(&display);
		}
                return;
        }

	erase_current_line();
	path[0] = 0;
        if ( (pathlen = getline_scroll("<b>File name:</b> ", COL_HEAD_MD, path,
                                       LBUF-1, 0, 0, display.pos) > 0)) {
	        find_filename(path, filename, sizeof(filename));
                if (filename[0] == 0) {
			if (display.reached_full_size) {
				display_window_pop(&display);
			}
                        return;
		}
                fullpath = interpreta_tilde_dir(path);

                fp = fopen(fullpath, "r");
                if (fp == NULL) {
			fill_line(&display, display.pos, COL_HEAD_ERROR);
                        cml_printf(_(
" *** File inesistente.            -- Premi un tasto per continuare --"
				     ));
                        while (c == 0) {
                                c = getchar();
			}
                        if (fullpath) {
                                Free(fullpath);
			}
			if (display.reached_full_size) {
				display_window_pop(&display);
			}
                        return;
                }
                fstat(fileno(fp), &filestat);
                len = (unsigned long)filestat.st_size;
                if (len == 0) {
			fill_line(&display, display.pos, COL_HEAD_ERROR);
                        cml_printf(_(" *** Mi rifiuto di allegare file vuoti!     -- Premi un tasto per continuare --"));
                        while (c == 0)
                                c = getchar();
                        if (fullpath)
                                Free(fullpath);
                        fclose(fp);
			if (display.reached_full_size) {
				display_window_pop(&display);
			}
                        return;
                }

                serv_putf("FUPB %s|%lu", filename, len);
                serv_gets(buf);

                if (buf[0] == '1') {
			fill_line(&display, display.pos, COL_HEAD_ERROR);
                        switch (buf[1]) {
                        case '3':
                                cml_printf(_(" *** Non si possono allegare file in questa room.     -- Premi un tasto --"));
                                break;
                        case '4':
                                cml_printf(_(" *** Questo file &egrave; troppo grande per essere allegato!   --- Premi un tasto ---"));
                                break;
                        case '5':
                                cml_printf(_(" *** Non hai spazio personale sufficiente! -- Premi un tasto per continuare --"));
                                break;
                        case '6':
                                cml_printf(_(" *** Hai gia uploadato troppi file!    --- Premi un tasto per continuare ---  "));
                                break;
                        case '7':
                                cml_printf(_(" *** Non c'&egrave; spazio a sufficienza nel server!       --- Premi un tasto ---  "));
                                break;
                        }
                        while (c == 0) {
                                c = getchar();
			}
                        if (fullpath) {
                                Free(fullpath);
			}
			if (display.reached_full_size) {
				display_window_pop(&display);
			}
                        return;
                }
                filenum = extract_ulong(buf+4, 0); /* no. prenotazione */

                // TODO check the flags
                flags = 0;

		if (display.reached_full_size) {
			display_window_pop(&display);
		}

                id = md_insert_file(t->mdlist, filename, fullpath, filenum,
				    len, flags);

                tmpcol = t->curs_col;
                filecol = COLOR_FILE;

                ATTR_SET_MDNUM(filecol, id);
                Editor_Set_Color(t, filecol);
                for (ptr = (t->mdlist->md[id-1])->content; *ptr; ptr++) {
                        if ((isascii(*ptr) && isprint(*ptr))
                            || is_isoch(*ptr)) {
                                Editor_Putchar(t, *ptr);
			}
                }
                Editor_Set_Color(t, tmpcol);
        }
}

/* Inserisce un testo da un file */
static void Editor_Insert_Link(Editor_Text *t)
{
        char buf[LBUF], label[LBUF];
        char *ptr, tmpcol;
        int id, linkcol;

	erase_current_line();
	buf[0] = 0;

        if (getline_scroll("<b>Insert Link:</b> ", COL_HEAD_MD, buf,
                           LBUF-8, 0, 0, display.pos) > 0) {
		erase_current_line();
                label[0] = 0;
                getline_scroll("<b>Etichetta (opzionale):</b> ", COL_HEAD_MD,
                               label, NCOL - 2, 0, 0, display.pos);
                id = md_insert_link(t->mdlist, buf, label);
                if (label[0] == 0) {
                        strncpy(label, buf, 75);
                        if (strlen(buf) > 75)
                                strcpy(label+75, "...");
                }

		if (display.reached_full_size) {
			display_window_pop(&display);
		}

                tmpcol = t->curs_col;
                linkcol = COLOR_LINK;
                ATTR_SET_MDNUM(linkcol, id);
                Editor_Set_Color(t, linkcol);
                for (ptr = label; *ptr; ptr++) {
                        if ((isascii(*ptr) && isprint(*ptr))
                            || is_isoch(*ptr))
                                Editor_Putchar(t, *ptr);
                }
                Editor_Set_Color(t, tmpcol);
        } else {
		if (display.reached_full_size) {
			display_window_pop(&display);
		}
	}
}

/* Inserisce un testo da un file */
static void Editor_Insert_Text(Editor_Text *t)
{
        char file_path[LBUF], *filename, buf[LBUF];
        FILE *fp;
        Line *nl;
	int len, wlen, color, i;
        int c = 0;

	fill_line(&display, display.pos, COL_HEAD_ERROR);
        file_path[0] = 0;
        if (getline_scroll("<b>Inserisci file:</b> ", COL_HEAD_MD, file_path,
                           LBUF-1, 0, 0, display.pos) > 0) {
                filename = interpreta_tilde_dir(file_path);
                fp = fopen(filename, "r");
                if (filename) {
                        Free(filename);
		}
                if (fp != NULL) {
			if (display.reached_full_size) {
				display_window_pop(&display);
			}
                        fgets(buf, 6, fp);
                        if (!strncmp(buf, "<cml>", 5)) {
                                color = C_DEFAULT;
                                if (t->curr->len) {
                                        if (t->curr->pos)
                                                Editor_Key_Enter(t);
                                        Editor_Key_Enter(t);
                                        Editor_Key_Up(t);
                                }
                                while (fgets(buf, LBUF, fp) != NULL) {
                                        for (i=0; buf[i]; i++) {
                                                if (buf[i]=='\n')
                                                        buf[i] = 0;
                                        }
                                        t->curr->len = cml2editor(buf, t->curr->str, t->curr->col, &len, t->max, &color, t->mdlist);
                                        t->curr->pos = 0;
                                        t->curr->flag = 1;
                                        if (t->curr->len >= t->max) {
						/* wrap word */
                                                //Editor_Key_Enter(t);
						textbuf_insert_line_below(
						            t->text, t->curr);

                                                nl = t->curr->next;
                                                wlen = 0;
                                                len = t->curr->len;
                                                while ((wlen <= len) && (t->curr->str[len-wlen]==' '))
                                                        wlen++;
                                                while ((wlen <= len) && (t->curr->str[len-wlen]!=' '))
                                                        wlen++;
                                                if (wlen >= (len-2))
                                                        wlen = len - t->max + 1;
                                                len -= wlen;
                                                nl->len = wlen + 2;
                                                memcpy(nl->str+2,
						       t->curr->str+len,
						       wlen*sizeof(int));
                                                memcpy(nl->col+2,
						       t->curr->col+len,
						       wlen*sizeof(int));
                                                t->curr->len = len;
                                                nl->str[0] = ' ';
                                                nl->str[1] = ' ';
                                                nl->col[0] = C_DEFAULT;
                                                nl->col[1] = C_DEFAULT;
                                                nl->pos = 0;
                                                nl->flag = 1;

						t->curr = t->curr->next;
					}
					textbuf_insert_line_below(t->text,
								  t->curr);
					t->curr = t->curr->next;
                                }
                        } else {
                                rewind(fp);
                                while( (c = fgetc(fp)) != EOF) {
                                        if ((c == Key_CR) || (c == Key_LF)) {
                                                Editor_Key_Enter(t);
					} else if ((isascii(c) && isprint(c))
						   || is_isoch(c)) {
                                                Editor_Putchar(t, c);
                                                if (t->curr->len >= t->max) {

                                                }
                                        }
                                }
                        }
                        fclose(fp);
                        return;
                } else {
			/* TODO display */
                        cti_mv(0, display.pos);
                        display_setcolor(&display, COL_HEAD_ERROR);
                        printf("%-80s\r", " *** File non trovato o non leggibile!! -- Premi un tasto per continuare --");
                        while (c == 0) {
                                c = getchar();
			}
			if (display.reached_full_size) {
				display_window_pop(&display);
			}
                }
        } else {
		if (display.reached_full_size) {
			display_window_pop(&display);
		}
	}
}



/* Save the text to a file */
static void Editor_Save_Text(Editor_Text *t)
{
        char file_path[LBUF], *filename, *out;
        FILE *fp;
        Line *line;
        int col;
        int c = 0;

        if (display.reached_full_size) {
		display_window_push(&display, 0, NRIGHE - 1);
	}
	fill_line(&display, display.pos, COL_HEAD_MD);
        file_path[0] = 0;
        if (getline_scroll("<b>Nome file:</b> ", COL_HEAD_MD, file_path,
                           LBUF-1, 0, 0, display.pos) > 0) {
                filename = interpreta_tilde_dir(file_path);
                fp = fopen(filename, "w");
                if (filename) {
                        Free(filename);
		}
                if (fp != NULL) {
                        col = C_DEFAULT;
                        fprintf(fp, "<cml>\n");
                        for (line = t->text->first; line; line = line->next) {
                                out = editor2cml(line->str, line->col, &col, line->len, t->mdlist);
                                fprintf(fp, "%s\n", out);
                                Free(out);
                        }
                        fclose(fp);
                } else {
			/* TODO display */
                        cti_mv(0, display.pos);
                        display_setcolor(&display, COL_HEAD_ERROR);
                        printf("%-80s\r", " *** Non posso creare o modificare il file! -- Premi un tasto per continuare --");
                        while (c == 0) {
                                c = getchar();
			}
                }
        }
	if (display.reached_full_size) {
		display_window_pop(&display);
	}
}

/* Libera la memoria allocata al testo. */
static void Editor_Free(Editor_Text *t)
{
	textbuf_free(t->text);
        Editor_Free_Copy_Buffer(t);
}

/* Elimina il metadata associato alla linea l */
static void Editor_Free_MD(Editor_Text *t, Line *l)
{
        for (int i = 0; i < l->len; i++) {
                int mdnum = line_get_mdnum(l, i);
                if (mdnum) {
                        mdop_delete(t->mdlist, mdnum);
                        do
                                i++;
                        while (line_get_mdnum(l, i) == mdnum);
                }
        }
}

/* Libera la memoria allocata al buffer di copia. */
static void Editor_Free_Copy_Buffer(Editor_Text *t)
{
	/*
	  TODO

	  Replace this with textbuf_free, but to do that first buf_pasted
	  should be moved to the TextBuf structure.

	*/

	Line *tmp;

	for (Line *line = t->killbuf->first; line; line = tmp) {
		tmp = line->next;
                if (!t->buf_pasted) {
                        Editor_Free_MD(t, line);
		}
		free(line);
	}
        t->killbuf->lines_count = 0;
        t->buf_pasted = false;
        t->killbuf->first = NULL;
        t->killbuf->last = NULL;
}

static int Editor_Ask_Abort(Editor_Text *t)
{
	if (display.reached_full_size) {
		display_window_push(&display, 0, NRIGHE - 1);
	}
	/* TODO display */
	cti_mv(0, display.pos - 2);
	display_setcolor(&display, C_EDITOR_DEBUG);
	printf(sesso
	       ? _("\nSei sicura di voler lasciar perdere il testo (s/n)? ")
	       : _("\nSei sicuro di voler lasciar perdere il testo (s/n)? "));
	if (si_no() == 'n')
		return false;
	if (display.reached_full_size) {
		display_window_pop(&display);
	}
	return true;
}

static void Editor2CML(Line *line, struct text *txt, int col,
                       Metadata_List *mdlist)
{
	char *out;

	if (col == 0)
		col = C_DEFAULT;
	for (; line; line = line->next) {
		out = editor2cml(line->str, line->col, &col, line->len, mdlist);
		txt_puts(txt, out);
	}
}

/* Inserisce il testo nella struttura txt nell'editor. */
static void text2editor(Editor_Text *t, struct text *txt, int color,
                        int max_col)
{
        Metadata_List *mdlist;
	Line *nl;
	char *str;
	int len, wlen;

	if (color == 0)
		color = C_DEFAULT;

	if (txt == NULL)
		return;
	txt_rewind(txt);

        mdlist = t->mdlist;
	while( (str = txt_get(txt))) {
		t->curr->len = cml2editor(str, t->curr->str, t->curr->col,
					  &len, t->max, &color, mdlist);
		t->curr->pos = 0;
		t->curr->flag = 1;
                if (t->curr->len >= max_col) { /* wrap word */
			textbuf_insert_line_below(t->text, t->curr);
                        nl = t->curr->next;
                        wlen = 0;
                        len = t->curr->len;
                        while ((wlen <= len) && (t->curr->str[len-wlen]==' '))
                                wlen++;
                        while ((wlen <= len) && (t->curr->str[len-wlen]!=' '))
                                wlen++;
                        if (wlen >= (len-2))
                                wlen = len - max_col + 1;
                        len -= wlen;
                        nl->len = wlen + 2;
                        memcpy(nl->str+2, t->curr->str+len, wlen*sizeof(int));
                        memcpy(nl->col+2, t->curr->col+len, wlen*sizeof(int));
                        t->curr->len = len;
                        nl->str[0] = ' ';
                        nl->str[1] = ' ';
                        nl->col[0] = C_DEFAULT;
                        nl->col[1] = C_DEFAULT;
                        nl->pos = 0;
                        nl->flag = 1;
                        t->curr = nl;
                }
		textbuf_insert_line_below(t->text, t->curr);
		t->curr = t->curr->next;
	}
	/* qui devo eliminare l'ultima riga */
}

/****************************************************************************/
/*
 * Visualizza l'help per l'editor interno.
 */
static void help_edit(Editor_Text *t)
{
	display_setcolor(&display, C_DEFAULT);
	cti_clear_screen();
	if (display.reached_full_size) {
		display_window_push(&display, 0, NRIGHE - 1);
	}
	/* TODO: define the help strings as constants, possibly in another
	 *       file. Unify with the help strings in edit.c .             */
	cml_printf(_(
//"        <b>o---O Help Editor Interno O---o</b>\n\n"
"<b>Attenzione</b>: le combinazioni di tasti Alt- vanno composte su alcuni terminali\n"
"            con la pressione del tasto Escape seguita dal tasto dato dalla\n"
"            combinazione.\n\n"
"<b>Movimento del cursore:</b> Utililizzare i tasti di cursore oppure\n"
"  Ctrl-B  Sposta il cursore di un carattere a sinistra (Backward).\n"
"  Ctrl-F  Sposta il cursore di un carattere a destra   (Forward).\n"
"  Ctrl-N  Vai alla riga successiva (Next).\n"
"  Ctrl-P  Vai alla riga precedente (Previous).\n"
"  Ctrl-A  Vai all'inizio della riga.\n"
"  Ctrl-E  Vai alla fine della riga.\n"
"  Ctrl-V, \\<Page Down\\>  Vai alla pagina successiva.\n"
"  Alt-v , \\<Page Up\\>    Vai alla pagina precedente.\n"
"  \\<Home\\>                Vai in cima al testo.\n"
"  \\<End\\>                 Vai in fondo al testo.\n\n"
"<b>Comandi di cancellazione:</b>\n"
"  Ctrl-D, \\<Del\\>  Cancella il carattere sotto al cursore.\n"
"  Ctrl-G  Incolla il testo tagliato con Ctrl-K.\n"
"  Ctrl-K  Cancella dal cursore fino alla fine della riga.\n"
"  Ctrl-U  Cancella tutto il testo.\n"
"  Ctrl-W  Cancella una parola all'indietro.\n"
"  Alt-d   Cancella la parola a destra del cursore.\n"
		     ));
	hit_any_key();
	cti_clear_screen();
        cml_printf(_(
"<b>Inserimento allegati:</b>\n\n"
"  Premendo Alt-i &egrave; possibile inserire in allegato al post\n"
"  dei file, dei link a pagine web, o riferimenti a post, room e utenti.\n\n"
"<b>Salvataggio post su file e inserimento file testo:</b> (solo client locale)\n\n"
"  Alt-i \\<t>ext  Inserisce nel post un file di testo.\n"
"  Ctrl-T        Salva il post in un file di testo.\n"
		     ));
        cml_printf(_(
"\n<b>Altri Comandi:</b>\n\n"
"  Alt-a   Abort: lascia perdere (chiede conferma).\n"
"  Ctrl-L  Rinfresca la schermata.\n"
"  Ctrl-X  Termina l'immissione del testo.\n"
		     ));
	cml_printf(_(
"\n<b>Modalit&agrave; di funzionamento dell'editor:</b>\n\n"
"Le modalit&agrave; disponibili sono <b>Insert</b> per l'inserimento del testo,\n"
"<b>Overwrite</b> per la sovrascrizione e <b>ASCII-Art</b> per creare disegni.\n"
"La selezione avviene mediante la pressione ripetuta del tasto \\<<b>Ins</b>\\>.\n\n"
		     ));
	hit_any_key();
	cti_clear_screen();
	cml_print(help_colors);
	cml_print(_(
"  \\<<b>F2</b>\\>  Applica colore e attributi correnti al carattere sotto al cursore.\n"
"  \\<<b>F3</b>\\>  Applica colore e attributi correnti fino al prossimo spazio.\n"
"  \\<<b>F4</b>\\>  Applica colore e attributi correnti a tutta la riga.\n\n"
		    ));
	hit_any_key();

	cti_clear_screen();
	cml_print(help_8bit);
	hit_any_key();

	if (display.reached_full_size) {
		display_window_pop(&display);
	}
	display.force_redraw = true;
}

#else
   /* Se non ho l'accesso alle capacita' del terminale editor interno... */
# include "edit.h"
# include "text.h"

int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp)
{
	txt_clear(txt);
	return get_text(txt, max_linee, max_col, abortp);
}

#endif /* HAVE_CTI */

/**************************************************************************/
/* Editor debugging utilities.                                            */
/**************************************************************************/
#ifdef EDITOR_DEBUG

#define CONSOLE_ROWS 5

static char console[CONSOLE_ROWS][LBUF];
static int console_line = 0;
static char console_status[LBUF];
static bool console_display_on = true;
static bool console_dirty = false;
static bool console_force_refresh = false;

static void console_init(void)
{
	console_line = 0;
	for (int i = 0; i < CONSOLE_ROWS; i++) {
		console[i][0] = 0;
	}
	console_status[0] = 0;
	console_printf("Editor debug console started.");
}

static void console_toggle(void)
{
	console_display_on = !console_display_on;
	console_printf("Console display: %s", console_display_on ? "On":"Off");
	console_force_refresh = true;
}

static void console_scroll(void)
{
	for (int i = 0; i < CONSOLE_ROWS - 1; i++) {
		strcpy(console[i], console[i + 1]);
	}
}

static void console_printf(const char *fmt, ...)
{
	va_list ap;

	console_scroll();
	va_start(ap, fmt);
	int len = snprintf(console[CONSOLE_ROWS - 1], sizeof(console[0]),
			   "%d.", console_line);
	vsnprintf(console[CONSOLE_ROWS - 1] + len, sizeof(console[0]), fmt, ap);
	va_end(ap);
	console_line++;
	console_dirty = true;
}

static void console_set_status(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(console_status, sizeof(console_status), fmt, ap);
	va_end(ap);
	console_dirty = true;
}

static void console_show_copy_buffer(Editor_Text *t)
{
	static int num_rows = 0;
	int row = CONSOLE_ROWS;

	if (t->killbuf->lines_count) {
		display_move_curs(&display, row++, 0);
		display_setcolor(&display, YELLOW);
		erase_current_line();
		printf("----- copy buffer -----");
		display_setcolor(&display, L_YELLOW);
		for (Line *line = t->killbuf->first; line; line=line->next) {
			if (row == display.pos - 1) {
				break;
			}
			display_move_curs(&display, row++, 0);
			erase_current_line();
			for (int i = 0; (i<NCOL-2) && (line->str[i]!=0); i++) {
				putchar(line->str[i]);
			}
		}
		display_setcolor(&display, YELLOW);
		if (row < display.pos - 1) {
			display_move_curs(&display, row++, 0);
			erase_current_line();
			printf("--- end copy buffer ---");
		}
	}
	int rows_written = row;
	while(row < num_rows) {
		display_move_curs(&display, row++, 0);
		erase_current_line();
	}
	num_rows = rows_written;
}

static void console_update(Editor_Text *t)
{
	if (!(console_force_refresh
	      || (console_display_on && console_dirty))) {
		return;
	}
	console_force_refresh = false;

	if (display.reached_full_size) {
		display_window_push(&display, 0, NRIGHE - 1);
	}

	display_setcolor(&display, COL_RED);
	for (int i = 0; i != CONSOLE_ROWS; i++) {
		display_move_curs(&display, i, 0);
		erase_current_line();
		if (i == CONSOLE_ROWS - 1) {
			display_setcolor(&display, L_RED);
		}
		printf("%s", console[i]);
	}

	console_show_copy_buffer(t);

	display_move_curs(&display, display.pos - 2, 0);
	display_setcolor(&display, COLOR(COL_GRAY, COL_RED, ATTR_BOLD));
	int ws_count = debug_get_winstack_index();
	int first, last;
	debug_get_current_win(&first, &last);
	printf("%s FR %d [fs %c rows %d] W(%d,%d)$%d", console_status,
	       display.top_line_num,
	       display.reached_full_size ? 'y' : 'n', t->term_nrows, first,
	       last, debug_get_winstack_index());
	for (int i = 0; i != ws_count; i++) {
		debug_get_winstack(i, &first, &last);
		printf("(%d,%d)", first, last);
	}
	erase_to_eol();

	if (display.reached_full_size) {
		display_window_pop(&display);
	}

	console_dirty = false;
}

static void sanity_checks(Editor_Text *t)
{
	if (t->text->first == NULL) {
		assert(t->text->last == NULL);
		assert(t->curr == NULL);
		goto SANITY_COPY_BUFFER;
	}
	assert(t->text->first->prev == NULL);
	assert(t->text->last->next == NULL);

	{
		int num = 1;
		Line *line = t->text->first;
		for (;;) {
			assert(line->num == num);
			if (line->next == NULL) {
				assert(line == t->text->last);
				break;
			}
			if (line->prev) {
				assert(line->prev->next == line);
			} else {
				assert(line == t->text->first);
			}
			num++;
			line = line->next;
		}
		assert(t->text->lines_count == num);
	}
	{
		Line *line = t->text->last;
		for (;;) {
			if (line->prev == NULL) {
				assert(line == t->text->first);
				break;
			}
			if (line->next) {
				assert(line->next->prev == line);
			} else {
				assert(line == t->text->last);
			}
			line = line->prev;
		}
	}

	/* Test the copy buffer sanity */
 SANITY_COPY_BUFFER:

	if (t->killbuf->first == NULL) {
		assert(t->killbuf->last == NULL);
		return;
	}
	assert(t->killbuf->first->prev == NULL);
	assert(t->killbuf->last->next == NULL);

	{
		int num = 1;
		Line *line = t->killbuf->first;
		for (;;) {
			//assert(line->num == num);
			if (line->next == NULL) {
				assert(line == t->killbuf->last);
				break;
			}
			if (line->prev) {
				assert(line->prev->next == line);
			} else {
				assert(line == t->killbuf->first);
			}
			num++;
			line = line->next;
		}
		assert(t->killbuf->lines_count == num);
	}
	{
		Line *line = t->killbuf->last;
		for (;;) {
			if (line->prev == NULL) {
				assert(line == t->killbuf->first);
				break;
			}
			if (line->next) {
				assert(line->next->prev == line);
			} else {
				assert(line == t->killbuf->last);
			}
			line = line->prev;
		}
	}
}
#endif
