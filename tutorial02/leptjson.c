#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <ctype.h>
#include <math.h>
#include <errno.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
	const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

//static int lept_parse_true(lept_context* c, lept_value* v) {
//    EXPECT(c, 't');
//    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 3;
//    v->type = LEPT_TRUE;
//    return LEPT_PARSE_OK;
//}
//
//static int lept_parse_false(lept_context* c, lept_value* v) {
//    EXPECT(c, 'f');
//    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 4;
//    v->type = LEPT_FALSE;
//    return LEPT_PARSE_OK;
//}
//
//static int lept_parse_null(lept_context* c, lept_value* v) {
//    EXPECT(c, 'n');
//    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//        return LEPT_PARSE_INVALID_VALUE;
//    c->json += 3;
//    v->type = LEPT_NULL;
//    return LEPT_PARSE_OK;
//}

static int lept_parse_literal_inner(const char** ptr, lept_value* v, char* pattern, int type) {
	assert(pattern != NULL && ptr != NULL && *ptr != NULL);
	const char* cptr = *ptr;
	//int cnt = 0;
	while (*cptr != '\0' && *pattern != '\0') {
		//cnt++;
		cptr++;
		pattern++;
	}
	if (*pattern == '\0') {
		v->type = type;
		*ptr = cptr;
		return LEPT_PARSE_OK;
	}
	return LEPT_PARSE_INVALID_VALUE;
}
static int lept_parse_literal(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 'n':
		return lept_parse_literal_inner(&c->json, v, "null", LEPT_NULL);
	case 't':
		return lept_parse_literal_inner(&c->json, v, "true", LEPT_TRUE);
	case 'f':
		return lept_parse_literal_inner(&c->json, v, "false", LEPT_FALSE);
	default:
		return LEPT_PARSE_INVALID_VALUE;
	}

}

static int check(const char* ptr) {
	if (ptr == '\0') { return -1; }
	if (*ptr == '-') { ptr++; }
	//if (*ptr == '0') { ptr++; }
	//else {
	//	if (!isdigit(*ptr)) { return LEPT_PARSE_INVALID_VALUE; }
	//	while (isdigit(*ptr)) { ptr++; }
	//}
	if (!isdigit(*ptr)) { return LEPT_PARSE_INVALID_VALUE; }
	if (*ptr == '0' && *(ptr + 1) != '\0' && *(ptr + 1) != '.') { /* number begins with zero*/return LEPT_PARSE_ROOT_NOT_SINGULAR; }
	while (isdigit(*ptr)) {
		ptr++;
	}

	if (*ptr == '.') {
		ptr++;
		if (!isdigit(*ptr)) { return LEPT_PARSE_INVALID_VALUE; }
		while (isdigit(*ptr)) { ptr++; }
	}
	if (*ptr == 'e' || *ptr == 'E') {
		ptr++;
		if (*ptr == '-' || *ptr == '+') {
			ptr++;
		}
		if (!isdigit(*ptr)) { return LEPT_PARSE_INVALID_VALUE; }
		while (isdigit(*ptr)) { ptr++; }
	}

	return LEPT_PARSE_OK;
}

int checkERROR(const char* ptr) {
	if (!isdigit(*ptr) && *ptr != '-') {
		return LEPT_PARSE_INVALID_VALUE;
	}
	if (*ptr == '+') {
		//begin with +
		return LEPT_PARSE_INVALID_VALUE;
	}
	if (*ptr == '-') { ptr++; }
	if (!isdigit(*ptr)) { return LEPT_PARSE_INVALID_VALUE; }
	if (*ptr == '0' && *(ptr + 1) != '\0' && *(ptr + 1) != '.') { /* number begins with zero*/return LEPT_PARSE_ROOT_NOT_SINGULAR; }
	while (isdigit(*ptr)) {
		ptr++;
	}
	if (*ptr == '.' && !isdigit(*(ptr + 1))) { return LEPT_PARSE_INVALID_VALUE; }
	return LEPT_PARSE_OK;
}
static int lept_parse_number(lept_context* c, lept_value* v) {
	char* end;
	/* \TODO validate number */
	int r = check(c->json);
	if (r) { return r; }

	v->n = strtod(c->json, &end);
	if((v->n == HUGE_VAL || v->n == -1 * HUGE_VAL) && errno == ERANGE ) { return LEPT_PARSE_NUMBER_TOO_BIG; }
	if (c->json == end)
		return LEPT_PARSE_INVALID_VALUE;

	c->json = end;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 't':  /*return lept_parse_true(c, v);*/
	case 'f':  /*return lept_parse_false(c, v);*/
	case 'n':  /*return lept_parse_null(c, v);*/
		return lept_parse_literal(c, v);
	default:   return lept_parse_number(c, v);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}
