#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dynarr.h"

#if defined(_MSC_VER) || defined(__WATCOMC__)
#include <malloc.h>
#else
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
#include <alloca.h>
#endif
#endif

#include "json.h"

#ifdef _MSC_VER
#define strncasecmp strnicmp
#endif

struct json_obj *json_alloc_obj(void)
{
	struct json_obj *obj;

	if(!(obj = malloc(sizeof *obj))) {
		fprintf(stderr, "json_alloc_obj: failed to allocate object\n");
		return 0;
	}
	json_init_obj(obj);
	return obj;
}

void json_free_obj(struct json_obj *obj)
{
	json_destroy_obj(obj);
	free(obj);
}

void json_init_obj(struct json_obj *obj)
{
	memset(obj, 0, sizeof *obj);
}

void json_destroy_obj(struct json_obj *obj)
{
	int i;

	for(i=0; i<obj->num_items; i++) {
		json_destroy_item(obj->items + i);
	}
	free(obj->items);
}

struct json_arr *json_alloc_arr(void)
{
	struct json_arr *arr;

	if(!(arr = malloc(sizeof *arr))) {
		fprintf(stderr, "json_alloc_arr: failed to allocate array\n");
		return 0;
	}
	json_init_arr(arr);
	return arr;
}

void json_free_arr(struct json_arr *arr)
{
	json_destroy_arr(arr);
	free(arr);
}

void json_init_arr(struct json_arr *arr)
{
	memset(arr, 0, sizeof *arr);
}

void json_destroy_arr(struct json_arr *arr)
{
	int i;

	for(i=0; i<arr->size; i++) {
		json_destroy_value(arr->val + i);
	}
	free(arr->val);
}


int json_item(struct json_item *item, const char *name)
{
	memset(item, 0, sizeof *item);

	if(!(item->name = strdup(name))) {
		fprintf(stderr, "json_item: failed to allocate name\n");
		return -1;
	}
	return 0;
}

void json_destroy_item(struct json_item *item)
{
	free(item->name);
	json_destroy_value(&item->val);
}

int json_value_str(struct json_value *jv, const char *str)
{
	jv->type = JSON_STR;

	jv->str = 0;
	if(str && !(jv->str = strdup(str))) {
		fprintf(stderr, "json_value_str: failed to duplicate string\n");
		return -1;
	}
	return 0;
}

void json_value_num(struct json_value *jv, double num)
{
	jv->type = JSON_NUM;
	jv->num = num;
}

void json_value_bool(struct json_value *jv, int bval)
{
	jv->type = JSON_BOOL;
	jv->boolean = bval;
}

void json_value_obj(struct json_value *jv, struct json_obj *obj)
{
	jv->type = JSON_OBJ;
	if(obj) {
		jv->obj = *obj;
	} else {
		json_init_obj(&jv->obj);
	}
}

void json_value_arr(struct json_value *jv, struct json_arr *arr)
{
	jv->type = JSON_ARR;
	if(arr) {
		jv->arr = *arr;
	} else {
		json_init_arr(&jv->arr);
	}
}

void json_destroy_value(struct json_value *jv)
{
	switch(jv->type) {
	case JSON_STR:
		free(jv->str);
		break;

	case JSON_OBJ:
		json_destroy_obj(&jv->obj);
		break;

	case JSON_ARR:
		json_destroy_arr(&jv->arr);
		break;

	default:
		break;
	}
}

int json_obj_append(struct json_obj *obj, const struct json_item *item)
{
	if(obj->num_items >= obj->max_items) {
		int newsz = obj->max_items ? (obj->max_items << 1) : 8;
		void *tmp = realloc(obj->items, newsz * sizeof *obj->items);
		if(!tmp) {
			fprintf(stderr, "json_obj_append: failed to grow items array (%d)\n", newsz);
			return -1;
		}
		obj->items = tmp;
		obj->max_items = newsz;
	}

	obj->items[obj->num_items++] = *item;
	return 0;
}

int json_arr_append(struct json_arr *arr, const struct json_value *val)
{
	if(arr->size >= arr->maxsize) {
		int newsz = arr->maxsize ? (arr->maxsize << 1) : 8;
		void *tmp = realloc(arr->val, newsz * sizeof *arr->val);
		if(!tmp) {
			fprintf(stderr, "json_arr_append: failed to grow array (%d)\n", newsz);
			return -1;
		}
		arr->val = tmp;
		arr->maxsize = newsz;
	}

	arr->val[arr->size++] = *val;
	return 0;
}

struct json_item *json_find_item(const struct json_obj *obj, const char *name)
{
	int i;
	for(i=0; i<obj->num_items; i++) {
		if(strcmp(obj->items[i].name, name) == 0) {
			return obj->items + i;
		}
	}
	return 0;
}

/* lookup functions */

struct json_value *json_lookup(struct json_obj *obj, const char *path)
{
	int len;
	char *pelem;
	const char *endp;
	struct json_item *it = 0;

	pelem = alloca(strlen(path) + 1);
	while(*path) {
		endp = path;
		while(*endp && *endp != '.') endp++;
		if(!(len = endp - path)) break;

		memcpy(pelem, path, len);
		pelem[len] = 0;

		/* continue after the . or point at the terminator */
		path = endp;
		if(*path == '.') path++;

		if(!(it = json_find_item(obj, pelem))) {
			return 0;
		}

		if(it->val.type != JSON_OBJ) {
			/* we hit a leaf. If the path continues we failed */
			if(*path) return 0;
		}

		/* item is an object, we can continue the lookup if necessary */
		obj = &it->val.obj;
	}

	return it ? &it->val : 0;
}

const char *json_lookup_str(struct json_obj *obj, const char *path, const char *def)
{
	struct json_value *val;

	if(!(val = json_lookup(obj, path)) || val->type != JSON_STR) {
		return def;
	}
	return val->str;
}

double json_lookup_num(struct json_obj *obj, const char *path, double def)
{
	struct json_value *val;

	if(!(val = json_lookup(obj, path)) || val->type != JSON_NUM) {
		return def;
	}
	return val->num;
}

int json_lookup_bool(struct json_obj *obj, const char *path, int def)
{
	struct json_value *val;

	if(!(val = json_lookup(obj, path)) || val->type != JSON_BOOL) {
		return def;
	}
	return val->boolean;
}

struct json_obj *json_lookup_obj(struct json_obj *obj, const char *path, struct json_obj *def)
{
	struct json_value *val;

	if(!(val = json_lookup(obj, path)) || val->type != JSON_OBJ) {
		return def;
	}
	return &val->obj;
}

struct json_arr *json_lookup_arr(struct json_obj *obj, const char *path, struct json_arr *def)
{
	struct json_value *val;

	if(!(val = json_lookup(obj, path)) || val->type != JSON_ARR) {
		return def;
	}
	return &val->arr;
}


/* ---- parser ---- */

#define MAX_TOKEN_SIZE	512
struct parser {
	char *text;
	char nextc;
	char *token;
};

enum { TOKEN_NUM, TOKEN_STR, TOKEN_BOOL };	/* plus all the single-char tokens */


static int item(struct parser *p, struct json_item *it);
static int array(struct parser *p, struct json_arr *arr);
static int object(struct parser *p, struct json_obj *obj);


static int next_char(struct parser *p)
{
	while(*p->text) {
		p->nextc = *p->text++;
		if(!isspace(p->nextc)) break;
	}
	return p->nextc;
}

#define SET_TOKEN(token, str, len) \
	do { \
		DYNARR_RESIZE(token, (len) + 1); \
		memcpy(token, str, len); \
		token[len] = 0; \
	} while(0)

static int next_token(struct parser *p)
{
	int len;
	char *ptr, *s;

	if(!p->nextc) return -1;

	switch(p->nextc) {
	case '{':
	case '}':
	case ',':
	case '[':
	case ']':
	case ':':
		SET_TOKEN(p->token, &p->nextc, 1);
		next_char(p);
		return p->token[0];

	case '"':
		DYNARR_CLEAR(p->token);
		next_char(p);
		while(p->nextc && p->nextc != '"') {
			DYNARR_STRPUSH(p->token, p->nextc);
			next_char(p);
		}
		next_char(p);
		return TOKEN_STR;

	default:
		break;
	}

	s = p->text - 1;

	strtod(s, &ptr);
	if(ptr != s) {
		len = ptr - s;
		SET_TOKEN(p->token, s, len);
		p->text = ptr;
		next_char(p);
		return TOKEN_NUM;
	}

	if(strncasecmp(s, "true", 4) == 0) {
		SET_TOKEN(p->token, "true", 4);
		p->text += 3;
		next_char(p);
		return TOKEN_BOOL;
	}
	if(strncasecmp(s, "false", 5) == 0) {
		SET_TOKEN(p->token, "false", 5);
		p->text += 4;
		next_char(p);
		return TOKEN_BOOL;
	}

	SET_TOKEN(p->token, &p->nextc, 1);
	fprintf(stderr, "json_parse: unexpected character: %c\n", p->nextc);
	return -1;
}

static int expect(struct parser *p, int tok)
{
	return next_token(p) == tok ? 1 : 0;
}

static const char *toktypestr(int tok)
{
	static char buf[] = "' '";
	switch(tok) {
	case TOKEN_NUM: return "number";
	case TOKEN_STR: return "string";
	case TOKEN_BOOL: return "boolean";
	default:
		break;
	}
	buf[1] = tok;
	return buf;
}

#define EXPECT(p, x) \
	do { \
		if(!expect(p, x)) { \
			fprintf(stderr, "json_parse: expected: %s, found: %s\n", toktypestr(x), (p)->token); \
			return -1; \
		} \
	} while(0)

static int value(struct parser *p, struct json_value *val)
{
	int toktype;
	struct json_obj obj;
	struct json_arr arr;

	toktype = next_token(p);
	switch(toktype) {
	case TOKEN_STR:
		if(json_value_str(val, p->token) == -1) {
			return -1;
		}
		break;

	case TOKEN_NUM:
		json_value_num(val, atof(p->token));
		break;

	case TOKEN_BOOL:
		json_value_bool(val, *p->token == 't' ? 1 : 0);
		break;

	case '{':	/* object */
		if(object(p, &obj) == -1) {
			return -1;
		}
		json_value_obj(val, &obj);
		break;

	case '[':	/* array */
		if(array(p, &arr) == -1) {
			return -1;
		}
		json_value_arr(val, &arr);
		break;

	default:
		fprintf(stderr, "json_parse: unexpected token %s while parsing item\n", p->text);
		return -1;
	}
	return 0;
}

static int item(struct parser *p, struct json_item *it)
{
	EXPECT(p, TOKEN_STR);
	if(json_item(it, p->token) == -1) {
		return -1;
	}
	EXPECT(p, ':');
	if(value(p, &it->val) == -1) {
		free(it->name);
		return -1;
	}
	return 0;
}

static int array(struct parser *p, struct json_arr *arr)
{
	struct json_value val;

	json_init_arr(arr);

	while(p->nextc != -1 && p->nextc != ']') {
		if(value(p, &val) == -1) {
			fprintf(stderr, "json_parse: expected value in array\n");
			json_destroy_arr(arr);
			return -1;
		}

		if(json_arr_append(arr, &val) == -1) {
			json_destroy_value(&val);
			json_destroy_arr(arr);
			return -1;
		}

		if(p->nextc == ',') expect(p, ',');	/* eat up comma separator */
	}
	EXPECT(p, ']');
	return 0;
}

static int object(struct parser *p, struct json_obj *obj)
{
	struct json_item it;

	json_init_obj(obj);

	while(p->nextc != -1 && p->nextc != '}') {
		if(item(p, &it) == -1) {
			fprintf(stderr, "json_parse: expected item in object\n");
			json_destroy_obj(obj);
			return -1;
		}

		if(json_obj_append(obj, &it) == -1) {
			json_destroy_item(&it);
			json_destroy_obj(obj);
			return -1;
		}

		if(p->nextc == ',') expect(p, ',');	/* eat up comma separator */
	}
	EXPECT(p, '}');
	return 0;
}

int json_parse(struct json_obj *root, const char *text)
{
	struct parser p;

	if(!text || !*text) return -1;

	p.nextc = *text;
	p.text = (char*)(text + 1);
	if(!(p.token = dynarr_alloc(0, 1))) {
		fprintf(stderr, "json_parse: failed to allocate token dynamic array\n");
		return -1;
	}

	EXPECT(&p, '{');
	if(object(&p, root) == -1) {
		dynarr_free(p.token);
		return -1;
	}
	dynarr_free(p.token);
	return 0;
}


static void putind(FILE *fp, int ind)
{
	int i;
	for(i=0; i<ind; i++) {
		fputs("  ", fp);
	}
}

void json_print(FILE *fp, struct json_obj *root)
{
	json_print_obj(fp, root, 0);
	fputc('\n', fp);
}

void json_print_obj(FILE *fp, struct json_obj *obj, int ind)
{
	int i;

	fputs("{\n", fp);

	for(i=0; i<obj->num_items; i++) {
		putind(fp, ind + 1);
		json_print_item(fp, obj->items + i, ind + 1);
		if(i < obj->num_items - 1) {
			fputs(",\n", fp);
		} else {
			fputc('\n', fp);
		}
	}

	putind(fp, ind);
	fputs("}", fp);
}

void json_print_arr(FILE *fp, struct json_arr *arr, int ind)
{
	int i;

	fputs("[\n", fp);

	for(i=0; i<arr->size; i++) {
		putind(fp, ind + 1);
		json_print_value(fp, arr->val + i, ind + 1);
		if(i < arr->size - 1) {
			fputs(",\n", fp);
		} else {
			fputc('\n', fp);
		}
	}

	putind(fp, ind);
	fputs("]", fp);
}

void json_print_item(FILE *fp, struct json_item *item, int ind)
{
	fprintf(fp, "\"%s\": ", item->name);
	json_print_value(fp, &item->val, ind);
}

void json_print_value(FILE *fp, struct json_value *val, int ind)
{
	switch(val->type) {
	case JSON_STR:
		fprintf(fp, "\"%s\"", val->str);
		break;
	case JSON_NUM:
		fprintf(fp, "%g", val->num);
		break;
	case JSON_BOOL:
		fprintf(fp, "%s", val->boolean ? "true" : "false");
		break;
	case JSON_OBJ:
		json_print_obj(fp, &val->obj, ind);
		break;
	case JSON_ARR:
		json_print_arr(fp, &val->arr, ind);
		break;
	default:
		fputs("<UNKNOWN>", fp);
	}
}
