#ifndef JSON_H_
#define JSON_H_

enum {
	JSON_NULL,
	JSON_STR,	/* "foo" */
	JSON_NUM,	/* 3.141 */
	JSON_BOOL,	/* true */
	JSON_OBJ,	/* { ... } */
	JSON_ARR	/* [ ... ] */
};

struct json_value;

/* objects are collections of items { "foo": 1, "bar": 2 } */
struct json_obj {
	struct json_item *items;
	int num_items, max_items;
};

/* arrays are collections of values [ "foo", "bar", 3.14, false, "xyzzy" ] */
struct json_arr {
	struct json_value *val;
	int size, maxsize;
};

struct json_value {
	int type;

	char *str;				/* JSON_STR */
	double num;				/* JSON_NUM */
	int boolean;			/* JSON_BOOL */
	struct json_obj obj;	/* JSON_OBJ */
	struct json_arr arr;	/* JSON_ARR */
};

/* items are key-value pairs "foo": "xyzzy" */
struct json_item {
	char *name;
	struct json_value val;

	struct json_item *next;
};


struct json_obj *json_alloc_obj(void);		/* malloc + json_init_obj */
void json_free_obj(struct json_obj *obj);	/* json_destroy_obj + free */
void json_init_obj(struct json_obj *obj);
void json_destroy_obj(struct json_obj *obj);

struct json_arr *json_alloc_arr(void);		/* malloc + json_init_arr */
void json_free_arr(struct json_arr *arr);	/* json_destroy_arr + free */
void json_init_arr(struct json_arr *arr);
void json_destroy_arr(struct json_arr *arr);

/* initialize item */
int json_item(struct json_item *item, const char *name);
void json_destroy_item(struct json_item *item);

/* pointer values for str, obj, arr can be null */
int json_value_str(struct json_value *jv, const char *str);
void json_value_num(struct json_value *jv, double num);
void json_value_bool(struct json_value *jv, int bval);
void json_value_obj(struct json_value *jv, struct json_obj *obj);	/* shallow copy obj */
void json_value_arr(struct json_value *jv, struct json_arr *arr);	/* shallow copy arr */

void json_destroy_value(struct json_value *jv);

/* item can be null, in which case only space is allocated
 * if not null, contents are shallow-copied (moved), do not destroy or use
 */
int json_obj_append(struct json_obj *obj, const struct json_item *item);

/* val can be null in which case only space is allocated
 * if not null, contents are shallow-copied (moved), do not destroy or use
 */
int json_arr_append(struct json_arr *arr, const struct json_value *val);

/* find named item in an object (direct descendant name only, not a path) */
struct json_item *json_find_item(const struct json_obj *obj, const char *name);

/* json_lookup returns 0 if the requested value is not found */
struct json_value *json_lookup(struct json_obj *root, const char *path);

/* typed lookup functions return their default argument if the requested value
 * is not found, or if it has the wrong type.
 */
const char *json_lookup_str(struct json_obj *obj, const char *path, const char *def);
double json_lookup_num(struct json_obj *obj, const char *path, double def);
int json_lookup_bool(struct json_obj *obj, const char *path, int def);
/* these next two are probably not very useful */
struct json_obj *json_lookup_obj(struct json_obj *obj, const char *path, struct json_obj *def);
struct json_arr *json_lookup_arr(struct json_obj *obj, const char *path, struct json_arr *def);


int json_parse(struct json_obj *root, const char *text);

/* mostly useful for debugging */
void json_print(FILE *fp, struct json_obj *root);
void json_print_obj(FILE *fp, struct json_obj *obj, int ind);
void json_print_arr(FILE *fp, struct json_arr *arr, int ind);
void json_print_item(FILE *fp, struct json_item *item, int ind);
void json_print_value(FILE *fp, struct json_value *val, int ind);

#endif	/* JSON_H_ */
