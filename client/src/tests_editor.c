/* tests */

#ifdef PERFORM_EDITOR_TESTS

static
void assert_str_eq(const char *str1, const char *str2)
{
	if (strcmp(str1, str2)) {
		printf("Error \'%s\' != \'%s\'", str1, str2);
		assert(false);
	}
}

static
void test_assert(void)
{
	assert_str_eq("", "");
	assert_str_eq("a", "a");
	assert_str_eq("\0a", "\0b");
	assert_str_eq("a\0a", "a\0b");
}

static
Line * line_set_str(Line *line, const char *str, const char *col)
{
	assert(strlen(str) == strlen(col));
	assert(line);
	for (size_t i = 0; i < strlen(str); i++) {
		line->str[i] = (int)str[i];
		line->col[i] = (int)col[i];
	}
	line->len = strlen(str);
	return line;
}

static
Line * line_from_str(const char *str, const char *col)
{
	assert(strlen(str) == strlen(col));
	Line *line = line_new();
	return line_set_str(line, str, col);
}

static
void line_extract_str(Line *line, char *str)
{
	for (int i = 0; i < line->len; i++) {
		str[i] = (char)line->str[i];
	}
	str[line->len] = 0;
}

static
void line_extract_col(Line *line, char *col)
{
	for (int i = 0; i < line->len; i++) {
		col[i] = (char)line->col[i];
	}
	col[line->len] = 0;
}

static int ints_to_str(const int *ints, ssize_t len, char *buf,
		       ssize_t bufsize)
{
	ssize_t written = 0;
	const char hexdigit[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
				 '9',
				 'a', 'b', 'c', 'd', 'e', 'f'};
	for (ssize_t i = 0; i != len; ++i) {
		if (isprint(ints[i])) {
			if (written + 1 >= bufsize) {
				break;
			}
			buf[written++] = ints[i];
		} else {
			if (written + 6 >= bufsize) {
				break;
			}
			buf[written++] = '{';
			buf[written++] = '0';
			buf[written++] = 'x';
			assert(ints[i] <= 0xffff);
			if (ints[i] > 0xff) {
				buf[written++] = hexdigit[(ints[i] >> 12)
							  & 0x0f];
				buf[written++] = hexdigit[(ints[i] >> 8)
							  & 0x0f];
			}
			buf[written++] = hexdigit[(ints[i] >> 4) & 0x0f];
			buf[written++] = hexdigit[ints[i] & 0x0f];
			buf[written++] = '}';
		}
	}
	assert(written < bufsize);
	buf[written++] = 0;

	return written;
}

static
void line_print(Line *line) {
	char str[256];

	ints_to_str(line->str, line->len, str, sizeof(str));
	printf("S '%s'\n", str);
	ints_to_str(line->col, line->len, str, sizeof(str));
	printf("C '%s'\n", str);
}

static
void textbuf_print(TextBuf *buf) {
	printf("TextBuf (%d lines)\n", buf->lines_count);
	Line *line = buf->first;
	while (line) {
		line_print(line);
		line = line->next;
	}
}

static
bool line_str_eq(Line *line, const char *str)
{
	char buf[256];
	line_extract_str(line, buf);
	if (strcmp(buf, str) != 0) {
		printf("line->str is '%s' != '%s'\n", buf, str);
		return false;
	}
	return true;
}

/* Returns true if the first n characters of the two string are the same */
static
bool line_eqn(Line *l1, Line *l2, int n)
{
	if (n > l1->len || n > l2->len) {
		return false;
	}
	for (int i = 0; i < n; i++) {
		if (l1->str[i] != l2->str[i] || l1->col[i] != l2->col[i]) {
			return false;
		}
	}
	return true;
}

static
bool line_eq(Line *l1, Line *l2)
{
	if (l1->len != l2->len) {
		return false;
	}
	return line_eqn(l1, l2, l1->len);
}

static
bool line_col_eq(Line *line, const char *col)
{
	char buf[256];
	line_extract_col(line, buf);
	if (strcmp(buf, col) != 0) {
		printf("line->col is '%s' != '%s'\n", buf, col);
		return false;
	}
	return true;
}

static
bool line_str_col_eq(Line *line, const char *str, const char *col)
{
	return line_str_eq(line, str) && line_col_eq(line, col);
}

static
bool line_str_col_eqn(Line *line, const char *str, const char *col)
{
	return line_str_eq(line, str) && line_col_eq(line, col);
}

static
void test_line_eq(void)
{
	Line *l1 = line_from_str("123", "abc");
	Line *l2 = line_from_str("123", "abc");
	Line *l3 = line_from_str("1234", "abcd");
	Line *l4 = line_from_str("12_", "abc");
	Line *l5 = line_from_str("123", "ab_");

	assert(line_eq(l1, l1));
	assert(line_eq(l1, l2));
	assert(!line_eq(l1, l3));
	assert(!line_eq(l1, l4));
	assert(!line_eq(l1, l5));

	Free(l1);
	Free(l2);
	Free(l3);
	Free(l4);
	Free(l5);
}

static
void test_line_copy_all(void)
{
	Line *src = line_from_str("123", "abc");
	Line *dest = line_new();
	line_copy_all(dest, src);
	assert(line_eq(src, dest));

	Free(src);
	src = line_from_str("", "");
	line_copy_all(dest, src);
	Line *result = line_from_str("123", "abc");
	assert(line_eq(result, dest));
	Free(result);

	Free(src);
	src = line_from_str("7", "d");
	line_copy_all(dest, src);
	result = line_from_str("723", "dbc");
	assert(line_eq(result, dest));
	Free(result);

	Free(src);
	src = line_from_str("aaaaa", "11111");
	line_copy_all(dest, src);
	assert(line_eqn(src, dest, src->len));

	Free(src);
	Free(dest);
}

static
void test_line_copy_from_t(char *str, char *col)
{
	Line *src = line_from_str(str, col);
	Line *dest = line_new();
	for (size_t offset = 0; offset <= strlen(str); offset++) {
		line_copy_from(dest, src, offset);
		if (!line_str_col_eq(dest, str + offset, col + offset)) {
			printf("test_line_copy_from(\"%s\", %zu) - Failed",
			       str, offset);
			assert(false);
		}
	}
	Free(src);
	Free(dest);
}

static
void test_line_copy_from(void)
{
	test_line_copy_from_t("", "");
	test_line_copy_from_t("0", "1");
	test_line_copy_from_t("0123456789", "abcdefghij");
}

static
void test_line_copy_t(char *dst_str, char *dst_col, char *src_str,
		      char *src_col)
{
	//printf("test line_copy: dst ('%s', '%s') - ('%s', '%s')\n", dst_str, dst_col, src_str, src_col);
	char rstr[256], rcol[256];
	Line *src = line_from_str(src_str, src_col);
	for (size_t doff = 0; doff <= strlen(dst_str); doff++) {
		for (size_t soff = 0; soff <= strlen(src_str); soff++) {
			Line *dest = line_from_str(dst_str, dst_col);
			line_copy(dest, doff, src, soff);
			line_extract_str(dest, rstr);
			line_extract_col(dest, rcol);
			if (strncmp(rstr + doff, src_str + soff,
				    strlen(src_str) - soff)
			    || strncmp(rcol + doff, src_col + soff,
				       strlen(src_str) - soff)) {
				printf(
				 "test_line_copy(\"%s\", %zu, \"%s\", %zu) - ",
				 dst_str, doff, src_str, soff);
				printf("failed (got \"%s\", \"%s\")\n", rstr,
				       rcol);
				assert(false);
			}
			Free(dest);
		}
	}
	Free(src);
}

static
void test_line_copy(void)
{
	test_line_copy_t("", "", "", "");
	test_line_copy_t("0", "a", "1", "b");
	test_line_copy_t("0123456789", "abcdefghij", "ABCDEFGHIJ", "KLMNOPQRST");
}

static
void test_line_remove_index_eq(char *str, int pos, char *result)
{
	Line *line;

	line = line_from_str(str, str);
	line_remove_index(line, pos);
	assert(line_str_col_eq(line, result, result));
	Free(line);
}

static
void test_line_insert_index_eq(char *str, int pos, char ch, char *result)
{
	Line *line;

	line = line_from_str(str, str);
	line_expand_index(line, pos);
	line->str[pos] = ch;
	line->col[pos] = ch;
	assert(line_str_col_eq(line, result, result));
	Free(line);
}

static
void test_line_remove_range_eq(char *str, int begin, int end, char *result)
{
	char buf[256];
	Line *line;

	line = line_from_str(str, str);
	line_remove_range(line, begin, end);

	line_extract_str(line, buf);
	if (!line_str_col_eq(line, result, result)) {
		printf("line_remove_range [%d,%d) from '%s' -> ", begin,
		       end, str);
		printf("'%s' ", buf);
		printf("instead of '%s' - Failed\n", result);
	}
	assert(line_str_col_eq(line, result, result));
	Free(line);
}

static
void test_line_extend_to_pos_eq(char *str, char *col)
{
	Line *line;
	char rstr[256], rcol[256];

	for (size_t pos = 0; pos != strlen(str) + 3; ++pos) {
		bool failed = false;
		line = line_from_str(str, col);
		int initial_len = line->len;
		bool will_extend = (size_t)initial_len <= pos;
		line_extend_to_pos(line, pos);
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		if (will_extend) {
			if ((size_t)line->len != pos + 1) {
				failed = true;
			}
		} else {
			if (line->len != initial_len) {
				failed = true;
			}
		}
		if (strncmp(rstr, str, strlen(str))
		    || strncmp(rcol, col, strlen(col))) {
			failed = true;
		}
		for (size_t i = strlen(str); i < strlen(rstr); ++i) {
			if (rstr[i] != ' ' || rcol[i] != C_DEFAULT) {
				failed = true;
			}
		}
		if (failed) {
			printf("line_extend_to_pos(\"%s\", \"%s\", %lu) failed"
			       "\n (got \"%s\", \"%s\")\n", str, col, pos,
			       rstr, rcol);
			assert(false);
		}
		Free(line);
	}
}

static
void test_line_from_str(void)
{
	Line *line;

	line = line_from_str("123", "abc");
	assert(line_str_col_eq(line, "123", "abc"));
	Free(line);
}

static
void test_line_remove_index(void)
{
	test_line_remove_index_eq("a", 0, "");
	test_line_remove_index_eq("abc", 0, "bc");
	test_line_remove_index_eq("abc", 1, "ac");
	test_line_remove_index_eq("abc", 2, "ab");
}

static
void test_line_expand_index(void)
{
	test_line_insert_index_eq("", 0, '+', "+");
	test_line_insert_index_eq("abc", 0, '+', "+abc");
	test_line_insert_index_eq("abc", 1, '+', "a+bc");
	test_line_insert_index_eq("abc", 2, '+', "ab+c");
	test_line_insert_index_eq("abc", 3, '+', "abc+");
}

void test_line_expand_front(void)
{
	char str[] = "123";
	char col[] = "abc";
	char rstr[64], rcol[64];

	for (int nchars = 0; nchars != 10; ++nchars) {
		Line *line = line_from_str(str, col);
		line->dirty = false;
		line_expand_front(line, nchars);
		assert(line->dirty);
		assert(line->len == (int)strlen(str) + nchars);
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		assert(!strcmp(rstr + nchars, str));
		assert(!strcmp(rcol + nchars, col));
		Free(line);
	}
}

void test_line_insert_range_front(void)
{
	char src_str[] = "12345 67890 1234 56 7890";
	char src_col[] = "abcdefghijklmnopqrstuvwx";
	char dst_str[] = "The quick brown fox jumps over the lazy dog!";
	char dst_col[] = "00011122233344455566677788899900011122233344";
	char rstr[256], rcol[256];

	int src_len = strlen(src_str);
	for (int space = 0; space != 2; space++) {
		for(int offset = 0; offset < src_len; ++offset) {
			for(int count = 0; count <= src_len - offset;
			    ++count) {
		Line *src = line_from_str(src_str, src_col);
		Line *dst = line_from_str(dst_str, dst_col);
		src->dirty = false;
		dst->dirty = false;
		line_insert_range_front(dst, src, offset, count, (bool)space, 0);
		assert(!src->dirty);
		assert(dst->dirty);
		assert(dst->len == (int)strlen(dst_str) + count + space);
		line_extract_str(dst, rstr);
		line_extract_col(dst, rcol);
		//line_print(dst);
		assert(!strncmp(rstr, src_str + offset, count));
		assert(!strncmp(rcol, src_col + offset, count));
		assert(!strncmp(rstr + count + space, dst_str,
				strlen(dst_str)));
		assert(!strncmp(rcol + count + space, dst_col,
				strlen(dst_str)));
		if (space) {
			assert(rstr[count] == ' ');
			assert(rcol[count] == 0);
		}
		Free(src);
		Free(dst);
			}
		}
	}
}

static
void test_line_remove_range(void)
{
	//test_line_remove_range_eq("", 0, 0, "");
	test_line_remove_range_eq("a", 0, 1, "");
	test_line_remove_range_eq("abcde", 0, 2, "cde");
	test_line_remove_range_eq("abcde", 1, 2, "acde");
	//test_line_remove_range_eq("abcde", 2, 2, "abcde");
	test_line_remove_range_eq("abcde", 3, 4, "abce");
	test_line_remove_range_eq("abcde", 3, 5, "abc");
	test_line_remove_range_eq("abcde", 0, 5, "");
	test_line_remove_range_eq("abcde", 0, 4, "e");
	test_line_remove_range_eq("abcde", 1, 5, "a");
}

static
void test_line_extend_to_pos(void)
{
	test_line_extend_to_pos_eq("", "");
	test_line_extend_to_pos_eq("a", "1");
	test_line_extend_to_pos_eq("qwerty", "123456");
}

static
void test_line_strip_trailing_space(void)
{
	char str[] = "123";
	char col[] = "abc";
	char str1[] = "123 ";
	char col1[] = "abcd";
	char str3[] = "123   ";
	char col3[] = "abcdef";
	char rstr[64], rcol[64];

	{
		Line *line = line_from_str(str, col);
		line->dirty = false;
		line_strip_trailing_space(line);
		assert(line->dirty);
		assert(line->len == (int)strlen(str));
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		assert(!strncmp(rstr, str, 3));
		assert(!strncmp(rcol, col, 3));
		Free(line);
	}

	{
		Line *line = line_from_str(str1, col1);
		line->dirty = false;
		line_strip_trailing_space(line);
		assert(line->dirty);
		assert(line->len == (int)strlen(str));
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		assert(!strncmp(rstr, str, 3));
		assert(!strncmp(rcol, col, 3));
		Free(line);
	}

	{
		Line *line = line_from_str(str3, col3);
		line->dirty = false;
		line_strip_trailing_space(line);
		assert(line->dirty);
		assert(line->len == (int)strlen(str));
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		assert(!strncmp(rstr, str, 3));
		assert(!strncmp(rcol, col, 3));
		Free(line);
	}
}

static
void test_line_truncate_at(void)
{
	char str[] = "The quick brown fox jumps over the lazy dog!";
	char col[] = "00001111112222223333444444555556666777778888";
	char rstr[256], rcol[256];

	for (int len = 0; len != (int)strlen(str); ++len) {
		Line *line = line_from_str(str, col);
		line->dirty = false;
		line_truncate_at(line, len);
		assert(line->dirty);
		assert(line->len == len);
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		assert(!strncmp(rstr, str, len));
		assert(!strncmp(rcol, col, len));
		Free(line);
	}

	{
		size_t len = strlen(str);
		Line *line = line_from_str(str, col);
		line->dirty = false;
		line_truncate_at(line, len);
		assert(!line->dirty);
		assert(line->len == (int)strlen(str));
		line_extract_str(line, rstr);
		line_extract_col(line, rcol);
		assert(!strncmp(rstr, str, strlen(str)));
		assert(!strncmp(rcol, col, strlen(str)));
		Free(line);
	}
}

static
void test_find_last_word(void)
{
	typedef struct {
		char str[64];
		char col[64];
		WordPos wp;
	} test_data;
	test_data data[] = {
		{.str = "cat",
		 .col = "012",
		 .wp = {.first = 0, .last = 2, .prev_blank = 0},
		},
		{.str = " cat",
		 .col = "0123",
		 .wp = {.first = 1, .last = 3, .prev_blank = 0},
		},
		{.str = "  cat",
		 .col = "01234",
		 .wp = {.first = 2, .last = 4, .prev_blank = 0},
		},
		{.str = "cat ",
		 .col = "0123",
		 .wp = {.first = 0, .last = 2, .prev_blank = 0},
		},
		{.str = "cat  ",
		 .col = "01234",
		 .wp = {.first = 0, .last = 2, .prev_blank = 0},
		},
		{.str = " cat ",
		 .col = "01234",
		 .wp = {.first = 1, .last = 3, .prev_blank = 0},
		},
		{.str = "cat dog",
		 .col = "0123456",
		 .wp = {.first = 4, .last = 6, .prev_blank = 3},
		},
		{.str = "cat   dog",
		 .col = "012345678",
		 .wp = {.first = 6, .last = 8, .prev_blank = 3},
		},
		{.str = "cat   dog   ",
		 .col = "012345678901",
		 .wp = {.first = 6, .last = 8, .prev_blank = 3},
		},
	};
	for (int i = 0; i != sizeof(data)/sizeof(*data); ++i) {
		Line *line = line_from_str(data[i].str, data[i].col);
		line->dirty = false;
		WordPos wp = find_last_word(line);
		assert(!line->dirty);
		assert(wp.first == data[i].wp.first);
		assert(wp.last == data[i].wp.last);
		assert(wp.prev_blank == data[i].wp.prev_blank);
		Free(line);
	}
}

static
void test_line_next_word_idx(void)
{
	char str[] = "abc defg    ghij    ";
	char col[] = "01234567890123456789";

	for (int offset = 0; offset <= (int)strlen(str); ++offset) {
		Line *line = line_from_str(str, col);
		line->dirty = false;
		int pos = line_next_word_idx(line, offset);
		assert(!line->dirty);
		if (offset < 3) {
			assert(pos == offset);
		} else if (offset < 4) {
			assert(pos == 4);
		} else if (offset < 8) {
			assert(pos == offset);
		} else if (offset < 12) {
			assert(pos == 12);
		} else if (offset < 16) {
			assert(pos == offset);
		} else {
			assert(pos == (int)strlen(str));
		}
		Free(line);
	}
}


/* Creates a new TextBuf with a single line containing (str, col). */
/* The resulting buffer must be freed. */
static
TextBuf * test_make_textbuf1(char *str, char *col)
{
	TextBuf *buf = textbuf_new();
	textbuf_append_new_line(buf);
	assert(buf->lines_count == 1);
	assert(buf->first == buf->last);
	assert(buf->first->len == 0);
	line_set_str(buf->first, str, col);
	return buf;
}

bool textbuf_eq(TextBuf *buf1, TextBuf *buf2)
{
	if (buf1->lines_count != buf2->lines_count) {
		return false;
	}
	Line *line1 = buf1->first;
	Line *line2 = buf2->first;
	while (line1) {
		assert(line2);
		if (!line_eq(line1, line2)) {
			return false;
		}
		line1 = line1->next;
		line2 = line2->next;
	}
	assert(line2 == NULL);
	return true;
}

/* Creates a new TextBuf with a lines filled with the conetents of (str,
 * col).
 * str and col must be two NULL terminated arrays of strings, with the same
 * number of elements. For each of those elements a new line is inserted in
 * the buf, filled with the contents of the element.
 * The resulting buffer must be freed. */
static
TextBuf * test_make_textbufn(char **str, char **col)
{
	TextBuf *buf = textbuf_new();
	while (*str) {
		assert(*col);
		assert(strlen(*col) == strlen(*str));
		Line *line = textbuf_append_new_line(buf);
		line_set_str(line, *str, *col);
		++str;
		++col;
	}
	assert(*col == NULL);
	return buf;
}

static
void test_make_textbuf(void)
{
	{
		TextBuf *buf = test_make_textbuf1("", "");
		assert(buf->lines_count == 1);
		assert(buf->first->len == 0);
		textbuf_free(buf);
	}
	{
		TextBuf *buf = test_make_textbuf1("abc", "123");
		assert(buf->lines_count == 1);
		assert(buf->first->len == 3);
		assert(line_str_col_eq(buf->first, "abc", "123"));
		textbuf_free(buf);
	}
	{
		char *str[3] = {"line1", "line2", NULL};
		char *col[3] = {"colr1", "colr2", NULL};
		TextBuf *buf = test_make_textbufn(str, col);
		assert(buf->lines_count == 2);
		assert(line_str_col_eq(buf->first, str[0], col[0]));
		assert(line_str_col_eq(buf->first->next, str[1], col[1]));
		textbuf_free(buf);
	}
	{
		char *str[3] = {"line1", "line2", NULL};
		char *col[3] = {"colr1", "colr2", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		TextBuf *buf2 = test_make_textbufn(str, col);
		TextBuf *buf3 = test_make_textbufn(col, str);
		assert(textbuf_eq(buf1, buf2));
		assert(!textbuf_eq(buf1, buf3));
		textbuf_append_new_line(buf2);
		assert(!textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
		textbuf_free(buf3);
	}
}

void test_textbuf_append_new_line(void)
{
	{
		TextBuf *buf1 = textbuf_new();
		textbuf_append_new_line(buf1);
		TextBuf *buf2 = test_make_textbuf1("", "");
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[3] = {"line1", "", NULL};
		char *col[3] = {"colr1", "", NULL};
		TextBuf *buf1 = test_make_textbuf1(str[0], col[0]);
		textbuf_append_new_line(buf1);
		TextBuf *buf2 = test_make_textbufn(str, col);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
}

static
void test_textbuf_insert_line_below(void)
{
	{
		char *str[3] = {"line1", "line3", NULL};
		char *col[3] = {"colr1", "colr3", NULL};
		char *rstr[4] = {"line1", "", "line3", NULL};
		char *rcol[4] = {"colr1", "", "colr3", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_insert_line_below(buf1, buf1->first);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[3] = {"line1", "line2", NULL};
		char *col[3] = {"colr1", "colr2", NULL};
		char *rstr[4] = {"line1", "line2", "", NULL};
		char *rcol[4] = {"colr1", "colr2", "",NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_insert_line_below(buf1, buf1->first->next);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}

}

static
void test_textbuf_insert_line_above(void)
{
	{
		char *str[3] = {"line1", "line3", NULL};
		char *col[3] = {"colr1", "colr3", NULL};
		char *rstr[4] = {"line1", "", "line3", NULL};
		char *rcol[4] = {"colr1", "", "colr3", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_insert_line_above(buf1, buf1->last);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[3] = {"line2", "line3", NULL};
		char *col[3] = {"colr2", "colr3", NULL};
		char *rstr[4] = {"", "line2", "line3", NULL};
		char *rcol[4] = {"", "colr2", "colr3",NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_insert_line_above(buf1, buf1->last->prev);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}

}

static
void test_textbuf_delete_line(void)
{
	{
		char *str[] = {"line1", "line2", "line3", NULL};
		char *col[] = {"colr1", "colr2", "colr3", NULL};
		char *rstr[] = {"line2", "line3", NULL};
		char *rcol[] = {"colr2", "colr3", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_delete_line(buf1, buf1->first);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[] = {"line1", "line2", "line3", NULL};
		char *col[] = {"colr1", "colr2", "colr3", NULL};
		char *rstr[] = {"line1", "line3", NULL};
		char *rcol[] = {"colr1", "colr3", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_delete_line(buf1, buf1->first->next);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[] = {"line1", "line2", "line3", NULL};
		char *col[] = {"colr1", "colr2", "colr3", NULL};
		char *rstr[] = {"line1", "line2", NULL};
		char *rcol[] = {"colr1", "colr2", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_delete_line(buf1, buf1->last);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
}

static
void test_textbuf_break_line()
{
	/* single line */
	{
		char *str[] = {"abc", NULL};
		char *col[] = {"123", NULL};
		char *rstr[] = {"", "abc", NULL};
		char *rcol[] = {"", "123", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first, 0);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[] = {"abc", NULL};
		char *col[] = {"123", NULL};
		char *rstr[] = {"a", "bc", NULL};
		char *rcol[] = {"1", "23", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first, 1);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[] = {"abc", NULL};
		char *col[] = {"123", NULL};
		char *rstr[] = {"abc", "", NULL};
		char *rcol[] = {"123", "", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first, 3);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	/* more lines */
	{
		int pos = 1;
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"123", "456", NULL};
		char *rstr[] = {"a", "bc", "def", NULL};
		char *rcol[] = {"1", "23", "456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first, pos);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		int pos = 3;
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"123", "456", NULL};
		char *rstr[] = {"abc", "", "def", NULL};
		char *rcol[] = {"123", "", "456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first, pos);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		int pos = 1;
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"123", "456", NULL};
		char *rstr[] = {"abc", "d", "ef", NULL};
		char *rcol[] = {"123", "4", "56", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first->next, pos);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		int pos = 3;
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"123", "456", NULL};
		char *rstr[] = {"abc", "def", "", NULL};
		char *rcol[] = {"123", "456", "", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_break_line(buf1, buf1->first->next, pos);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
}

static
void test_textbuf_merge_lines(void)
{
	/* above line empty */
	{
		char *str[] = {"", "", NULL};
		char *col[] = {"", "", NULL};
		char *rstr[] = {"", NULL};
		char *rcol[] = {"", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		Merge_Lines_Result result = textbuf_merge_lines(buf1,
								buf1->first, buf1->first->next, 80);
		assert(result == MERGE_ABOVE_EMPTY);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	/* above line empty */
	{
		char *str[] = {"", "def", NULL};
		char *col[] = {"", "456", NULL};
		char *rstr[] = {"def", NULL};
		char *rcol[] = {"456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		Merge_Lines_Result result = textbuf_merge_lines(buf1,
								buf1->first, buf1->first->next, 80);
		assert(result == MERGE_ABOVE_EMPTY);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	/* below line empty */
	{
		char *str[] = {"abc", "", NULL};
		char *col[] = {"123", "", NULL};
		char *rstr[] = {"abc", NULL};
		char *rcol[] = {"123", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		Merge_Lines_Result result = textbuf_merge_lines(buf1,
								buf1->first, buf1->first->next, 80);
		assert(result == MERGE_BELOW_EMPTY);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	// Merge with below that fits in above
	// no extra space needed
	{
		/* NOTE 0x07 is C_DEFAULT */
		char *str[] = {"abc ", "def", NULL};
		char *col[] = {"123\x07", "456", NULL};
		char *rstr[] = {"abc def", NULL};
		char *rcol[] = {"123\x07" "456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_merge_lines(buf1, buf1->first, buf1->first->next, 80);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);

		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		char *str[] = {"abc", " def", NULL};
		char *col[] = {"123", "\x07" "456", NULL};
		char *rstr[] = {"abc def", NULL};
		char *rcol[] = {"123\x07" "456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_merge_lines(buf1, buf1->first, buf1->first->next, 80);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);

		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	// Note that extra spaces are not collapsed.
	// TODO Should we consider doing that?
	{
		char *str[] = {"abc ", " def", NULL};
		char *col[] = {"123\x07", "\x07" "456", NULL};
		char *rstr[] = {"abc  def", NULL};
		char *rcol[] = {"123\x07\x07" "456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_merge_lines(buf1, buf1->first, buf1->first->next, 80);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);

		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		// NOTE since no space was added, its color is unchanged
		char *str[] = {"abc   ", "def", NULL};
		char *col[] = {"123456", "789", NULL};
		char *rstr[] = {"abc   def", NULL};
		char *rcol[] = {"123456789", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_merge_lines(buf1, buf1->first, buf1->first->next, 80);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);

		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	// Extra space needed
	{
		// Same colors
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"12\xf0", "\x0f" "56", NULL};
		char *rstr[] = {"abc def", NULL};
		char *rcol[] = {"12\xf0\x07" "\x0f""56", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_merge_lines(buf1, buf1->first, buf1->first->next, 80);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		// Different BG colors -> C_DEFAULT
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"12\xf0", "\x0f""56", NULL};
		char *rstr[] = {"abc def", NULL};
		char *rcol[] = {"12\xf0\x07\x0f""56", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		textbuf_merge_lines(buf1, buf1->first, buf1->first->next, 80);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	// The below string doesn't fit
	{
		// and can't be split
		char *str[] = {"abc", "def", NULL};
		char *col[] = {"123", "456", NULL};
		char *rstr[] = {"abc", "def", NULL};
		char *rcol[] = {"123", "456", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		Merge_Lines_Result result =
			textbuf_merge_lines(buf1, buf1->first,
					    buf1->first->next, 7);
	        assert(result == MERGE_NOTHING);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		// first word fits
		char *str[] = {"abc", "def ghi", NULL};
		char *col[] = {"12\xf0", "\x0f""56 789", NULL};
		char *rstr[] = {"abc def", "ghi", NULL};
		char *rcol[] = {"12\xf0\x07\x0f""56", "789", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		Merge_Lines_Result result =
			textbuf_merge_lines(buf1, buf1->first,
					    buf1->first->next, 8);
	        assert(result == MERGE_EXTRA_SPACE);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		// first word fits
		char *str[] = {"abc", "def ghi", NULL};
		char *col[] = {"12\xf0", "\x0f""56 789", NULL};
		char *rstr[] = {"abc def", "ghi", NULL};
		char *rcol[] = {"12\xf0\x07\x0f""56", "789", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		Merge_Lines_Result result =
			textbuf_merge_lines(buf1, buf1->first,
					    buf1->first->next, 9);
	        assert(result == MERGE_EXTRA_SPACE);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
	{
		// first word fits
		char *str[] = {"abc", "def ghi", NULL};
		char *col[] = {"12\xf0", "\x0f""56 789", NULL};
		char *rstr[] = {"abc def", "ghi", NULL};
		char *rcol[] = {"12\xf0\x07\x0f""56", "789", NULL};
		TextBuf *buf1 = test_make_textbufn(str, col);
		//textbuf_print(buf1);
		Merge_Lines_Result result =
			textbuf_merge_lines(buf1, buf1->first,
					    buf1->first->next, 11);
	        assert(result == MERGE_EXTRA_SPACE);
		TextBuf *buf2 = test_make_textbufn(rstr, rcol);
		//textbuf_print(buf1);
		//textbuf_print(buf2);
		assert(textbuf_eq(buf1, buf2));
		textbuf_free(buf1);
		textbuf_free(buf2);
	}
}

static
void test_textbuf_delete_char(void)
{
	// TODo
}

void test(void)
{
	test_assert();
	test_line_from_str();
	test_line_eq();
	test_line_copy();
	test_line_copy_all();
	//test_line_copy_from();
	test_line_remove_index();
	test_line_expand_index();
	test_line_expand_front();
	test_line_insert_range_front();
	test_line_remove_range();
	test_line_extend_to_pos();
	test_line_strip_trailing_space();
	test_line_truncate_at();
	test_find_last_word();
	test_line_next_word_idx();
	test_make_textbuf();
	test_textbuf_append_new_line();
	test_textbuf_insert_line_below();
	test_textbuf_insert_line_above();
	test_textbuf_delete_line();
	test_textbuf_break_line();
	test_textbuf_merge_lines();
	test_textbuf_delete_char();
	printf("*** Editor tests passed.\n\n\n\n\n");
}

#endif
