#ifndef MYNA_JSON_H
#define MYNA_JSON_H

#include <stddef.h>

/*
 * A small, strict JSON reader and writer for the MCP server. Values form a
 * tree: arrays and objects hold their elements as a linked list of children.
 */

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type;

typedef struct json_value json_value;
struct json_value {
    json_type type;
    char *key;          /* member name, when inside an object */
    char *text;         /* decoded string, or a number's original text */
    int boolean;
    json_value *child;  /* first element or member */
    json_value *next;   /* next sibling */
};

/*
 * Parse a complete JSON document. Returns NULL if it is malformed, nested too
 * deeply, or contains a \u0000 escape. Free the result with json_free().
 */
json_value *json_parse(const char *text);
void json_free(json_value *v);

/* The member `key` of an object, or NULL. */
const json_value *json_get(const json_value *obj, const char *key);

/* The text of a string value, or NULL if `v` is not a string. */
const char *json_string(const json_value *v);

/* A growable output buffer. On allocation failure `failed` is set and
 * further writes are ignored. */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
    int failed;
} json_buf;

void json_buf_init(json_buf *b);
void json_buf_free(json_buf *b);
/* Append text as-is. */
void json_raw(json_buf *b, const char *text);
/* Append `text` as a quoted, escaped JSON string. */
void json_str(json_buf *b, const char *text);
/* Append an existing value as JSON (used to echo request ids). */
void json_write(json_buf *b, const json_value *v);

#endif /* MYNA_JSON_H */
