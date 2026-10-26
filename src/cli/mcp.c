#include "mcp.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "myna/prompt.h"
#include "myna/search.h"
#include "myna/version.h"

#include "json.h"

#define MODERN_VERSION "2026-07-28"
#define MAX_RESULTS 5
#define MAX_SUGGESTIONS 3
#define MAX_IDS 10
#define MAX_DESCRIPTION 2000
#define MAX_LINE (1024 * 1024)

/* JSON-RPC and MCP error codes. */
#define PARSE_ERROR -32700
#define INVALID_REQUEST -32600
#define METHOD_NOT_FOUND -32601
#define INVALID_PARAMS -32602
#define UNSUPPORTED_VERSION -32022

/* Legacy versions, newest first, served after an `initialize` handshake. */
static const char *const LEGACY_VERSIONS[] = {
    "2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05",
};
#define LEGACY_COUNT (sizeof LEGACY_VERSIONS / sizeof LEGACY_VERSIONS[0])

static const char INSTRUCTIONS[] =
    "Myna knows what a good implementation of common app features needs: "
    "dialogs, menus, forms, loading states, navigation, sign-in and more. "
    "Before building such a feature, call find_concepts with the user's request "
    "in plain words, then get_checklist with the ids you will implement, and "
    "follow its requirements. After writing the code, call review_checklist "
    "with the same ids and check the code against every item.";

#define ID_LIST_SCHEMA                                                          \
    "\"concept_ids\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},"      \
    "\"maxItems\":10,\"description\":\"Concept ids from find_concepts, such as "\
    "modal-dialog.\"},"
#define READ_ONLY                                                               \
    "\"annotations\":{\"readOnlyHint\":true,\"idempotentHint\":true,"           \
    "\"openWorldHint\":false}"

static const char TOOLS[] =
    "["
    "{\"name\":\"find_concepts\",\"title\":\"Find concepts\","
    "\"description\":\"Find the software concepts behind a plain-language "
    "request, such as 'close the popup when I click outside'. Returns concept "
    "ids, names, one-line explanations and related concepts to consider.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"description\":"
    "{\"type\":\"string\",\"description\":\"What the user wants, in their own "
    "words.\"}},\"required\":[\"description\"],\"additionalProperties\":false},"
    READ_ONLY "},"
    "{\"name\":\"get_checklist\",\"title\":\"Get implementation checklist\","
    "\"description\":\"Get what a good implementation must do (requirements) "
    "and how to check it works (done when) for the chosen concepts, as a "
    "ready-to-follow brief. Call it before writing code. Pass concept_ids from "
    "find_concepts, or a description to use the best matches.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{" ID_LIST_SCHEMA
    "\"description\":{\"type\":\"string\",\"description\":\"The user's request "
    "in plain words. Used to find concepts when concept_ids is empty, and "
    "quoted in the brief.\"}},\"additionalProperties\":false},"
    READ_ONLY "},"
    "{\"name\":\"review_checklist\",\"title\":\"Review against checklist\","
    "\"description\":\"Get a numbered review rubric for finished code: every "
    "requirement and done-when check for the chosen concepts, to mark PASS, "
    "FAIL or NOT APPLICABLE. Call it after writing code, then read the code "
    "and answer each item.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{" ID_LIST_SCHEMA
    "\"description\":{\"type\":\"string\",\"description\":\"What was built, in "
    "plain words. Used to find concepts when concept_ids is empty.\"}},"
    "\"additionalProperties\":false},"
    READ_ONLY "}"
    "]";

void myna_mcp_init(myna_mcp_state *st)
{
    st->legacy = 0;
}

/* printf-style append to a buffer used for plain text. */
static void textf(json_buf *b, const char *fmt, ...)
{
    char small[512];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(small, sizeof small, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if ((size_t)n < sizeof small) {
        json_raw(b, small);
        return;
    }
    char *big = malloc((size_t)n + 1);
    if (!big) {
        b->failed = 1;
        return;
    }
    va_start(ap, fmt);
    vsnprintf(big, (size_t)n + 1, fmt, ap);
    va_end(ap);
    json_raw(b, big);
    free(big);
}

static void put_server_info(json_buf *b)
{
    json_raw(b, "{\"name\":\"myna\",\"title\":\"Myna\",\"version\":\"" MYNA_VERSION "\"}");
}

static void put_versions(json_buf *b)
{
    json_raw(b, "[\"" MODERN_VERSION "\"");
    for (size_t i = 0; i < LEGACY_COUNT; i++) {
        json_raw(b, ",");
        json_str(b, LEGACY_VERSIONS[i]);
    }
    json_raw(b, "]");
}

/* `id` may be NULL when the request's id could not be read. */
static void begin_reply(json_buf *b, const json_value *id)
{
    json_raw(b, "{\"jsonrpc\":\"2.0\",\"id\":");
    if (id)
        json_write(b, id);
    else
        json_raw(b, "null");
}

static void begin_result(json_buf *b, const json_value *id)
{
    begin_reply(b, id);
    json_raw(b, ",\"result\":{\"resultType\":\"complete\","
                "\"_meta\":{\"io.modelcontextprotocol/serverInfo\":");
    put_server_info(b);
    json_raw(b, "}");
}

static void end_result(json_buf *b)
{
    json_raw(b, "}}");
}

/* Cache hints, required on list and discovery results in 2026-07-28. Myna's
 * tools never change while it runs and hold nothing user-specific. */
static void put_cache_hints(json_buf *b)
{
    json_raw(b, ",\"ttlMs\":3600000,\"cacheScope\":\"public\"");
}

/* `data` is raw JSON or NULL. */
static void put_error(json_buf *b, const json_value *id, int code, const char *message,
                      const char *data)
{
    char num[16];
    snprintf(num, sizeof num, "%d", code);
    begin_reply(b, id);
    json_raw(b, ",\"error\":{\"code\":");
    json_raw(b, num);
    json_raw(b, ",\"message\":");
    json_str(b, message);
    if (data) {
        json_raw(b, ",\"data\":");
        json_raw(b, data);
    }
    json_raw(b, "}}");
}

static void handle_initialize(json_buf *b, const json_value *id, const json_value *params)
{
    const char *requested = json_string(json_get(params, "protocolVersion"));
    const char *version = LEGACY_VERSIONS[0];

    for (size_t i = 0; requested && i < LEGACY_COUNT; i++) {
        if (strcmp(requested, LEGACY_VERSIONS[i]) == 0)
            version = LEGACY_VERSIONS[i];
    }

    begin_result(b, id);
    json_raw(b, ",\"protocolVersion\":");
    json_str(b, version);
    json_raw(b, ",\"capabilities\":{\"tools\":{}},\"serverInfo\":");
    put_server_info(b);
    json_raw(b, ",\"instructions\":");
    json_str(b, INSTRUCTIONS);
    end_result(b);
}

static void handle_discover(json_buf *b, const json_value *id)
{
    begin_result(b, id);
    json_raw(b, ",\"supportedVersions\":");
    put_versions(b);
    json_raw(b, ",\"capabilities\":{\"tools\":{}},\"instructions\":");
    json_str(b, INSTRUCTIONS);
    put_cache_hints(b);
    end_result(b);
}

/* Modern requests must name a supported version in `_meta`. Writes an error
 * and returns 0 if they don't. */
static int check_modern_meta(json_buf *b, const json_value *id, const json_value *params)
{
    const json_value *meta = json_get(params, "_meta");
    const char *version = json_string(json_get(meta, "io.modelcontextprotocol/protocolVersion"));

    if (!version || !json_get(meta, "io.modelcontextprotocol/clientCapabilities")) {
        put_error(b, id, INVALID_PARAMS,
                  "Missing required _meta fields io.modelcontextprotocol/protocolVersion "
                  "and io.modelcontextprotocol/clientCapabilities (or send initialize "
                  "first for protocol versions up to 2025-11-25)",
                  NULL);
        return 0;
    }
    if (strcmp(version, MODERN_VERSION) != 0) {
        json_buf data;
        json_buf_init(&data);
        json_raw(&data, "{\"supported\":");
        put_versions(&data);
        json_raw(&data, ",\"requested\":");
        json_str(&data, version);
        json_raw(&data, "}");
        put_error(b, id, UNSUPPORTED_VERSION, "Unsupported protocol version",
                  data.failed ? NULL : data.data);
        json_buf_free(&data);
        return 0;
    }
    return 1;
}

/* ---- tools ---- */

static void tool_find(json_buf *text, int *is_error, const myna_catalogue *cat,
                      const json_value *args)
{
    const char *desc = json_string(json_get(args, "description"));
    myna_match matches[MAX_RESULTS];

    if (!desc || !*desc) {
        textf(text, "Pass a plain-language description of what the user wants, "
                    "for example \"close the popup when I click outside\".");
        *is_error = 1;
        return;
    }
    if (strlen(desc) > MAX_DESCRIPTION) {
        textf(text, "The description is too long (max %d characters). "
                    "Describe one feature at a time.", MAX_DESCRIPTION);
        *is_error = 1;
        return;
    }

    size_t n = myna_search(cat, desc, matches, MAX_RESULTS);
    if (n == 0) {
        textf(text, "No matching concepts found for \"%s\". Myna covers common "
                    "app features such as dialogs, menus, forms, loading states, "
                    "navigation, sign-in, data and releases. Try describing what "
                    "the user should see or do in different words.", desc);
        return;
    }

    textf(text, "Concepts matching \"%s\":\n", desc);
    for (size_t i = 0; i < n; i++) {
        const myna_concept *c = matches[i].concept;
        textf(text, "\n%d. %s (id: %s)\n   %s\n", (int)(i + 1), c->name, c->id,
              c->explanation);
        if (c->requirement_count || c->done_when_count)
            textf(text, "   Checklist: %d requirements, %d done-when checks\n",
                  (int)c->requirement_count, (int)c->done_when_count);
        else
            textf(text, "   Checklist: none yet\n");
    }

    const myna_concept *related[MAX_SUGGESTIONS];
    size_t r = myna_suggest_related(cat, matches, n, related, MAX_SUGGESTIONS);
    for (size_t i = 0; i < r; i++)
        textf(text, "%s%s (id: %s)", i ? ", " : "\nAlso consider: ", related[i]->name,
              related[i]->id);
    if (r)
        textf(text, "\n");

    textf(text, "\nNext: call get_checklist with the ids of the concepts you will "
                "implement (skip any that don't fit the request).");
}

/*
 * Resolve the concepts for get_checklist and review_checklist: concept_ids
 * when given, otherwise the best matches for description. Returns the number
 * found; on failure writes a message to `text`, sets *is_error and returns 0.
 */
static size_t resolve_concepts(json_buf *text, int *is_error, const myna_catalogue *cat,
                               const json_value *args, const myna_concept **out)
{
    const json_value *ids = json_get(args, "concept_ids");
    const char *desc = json_string(json_get(args, "description"));
    size_t n = 0;

    if (ids && ids->type != JSON_ARRAY) {
        textf(text, "concept_ids must be an array of strings, such as [\"modal-dialog\"].");
        *is_error = 1;
        return 0;
    }

    if (ids && ids->child) {
        int unknown = 0;
        for (const json_value *v = ids->child; v; v = v->next) {
            const char *id = json_string(v);
            const myna_concept *c = id ? myna_find_by_id(cat, id) : NULL;
            if (!c) {
                textf(text, unknown++ ? ", %s" : "Unknown concept id: %s",
                      id ? id : "(not a string)");
                continue;
            }
            int dup = 0;
            for (size_t k = 0; k < n; k++)
                dup |= out[k] == c;
            if (dup)
                continue;
            if (n == MAX_IDS) {
                textf(text, "Too many concept ids (max %d).", MAX_IDS);
                *is_error = 1;
                return 0;
            }
            out[n++] = c;
        }
        if (unknown) {
            textf(text, ". Call find_concepts to get valid ids.");
            *is_error = 1;
            return 0;
        }
        return n;
    }

    if (!desc || !*desc) {
        textf(text, "Pass concept_ids from find_concepts, or a description of the "
                    "feature in plain words.");
        *is_error = 1;
        return 0;
    }
    if (strlen(desc) > MAX_DESCRIPTION) {
        textf(text, "The description is too long (max %d characters).", MAX_DESCRIPTION);
        *is_error = 1;
        return 0;
    }

    myna_match matches[MAX_RESULTS];
    n = myna_search(cat, desc, matches, MAX_RESULTS);
    if (n == 0) {
        textf(text, "No matching concepts found for \"%s\". Call find_concepts with "
                    "different words, then pass concept_ids.", desc);
        *is_error = 1;
        return 0;
    }
    for (size_t i = 0; i < n; i++)
        out[i] = matches[i].concept;
    return n;
}

static void append_built(json_buf *text, size_t len, int review, const myna_catalogue *cat,
                         const char *desc, const myna_concept *const *chosen, size_t n)
{
    char *s = malloc(len + 1);
    if (!s) {
        text->failed = 1;
        return;
    }
    if (review)
        myna_build_review(chosen, n, s, len + 1);
    else
        myna_build_prompt(cat, desc, chosen, n, s, len + 1);
    json_raw(text, s);
    free(s);
}

static void tool_checklist(json_buf *text, int *is_error, const myna_catalogue *cat,
                           const json_value *args, int review)
{
    const myna_concept *chosen[MAX_IDS];
    const char *desc = json_string(json_get(args, "description"));
    size_t n = resolve_concepts(text, is_error, cat, args, chosen);
    if (n == 0)
        return;

    if (review) {
        append_built(text, myna_build_review(chosen, n, NULL, 0), 1, cat, NULL, chosen, n);
        return;
    }

    if (desc && strlen(desc) > MAX_DESCRIPTION)
        desc = NULL;
    append_built(text, myna_build_prompt(cat, desc, chosen, n, NULL, 0), 0, cat, desc,
                 chosen, n);
    textf(text, "\nAfter implementing, call review_checklist with concept_ids [");
    for (size_t i = 0; i < n; i++)
        textf(text, "%s\"%s\"", i ? ", " : "", chosen[i]->id);
    textf(text, "] and check the code against every item.\n");
}

static void handle_tools_call(json_buf *b, const json_value *id, const myna_catalogue *cat,
                              const json_value *params)
{
    const char *name = json_string(json_get(params, "name"));
    const json_value *args = json_get(params, "arguments");
    json_buf text;
    int is_error = 0;

    if (!name) {
        put_error(b, id, INVALID_PARAMS, "Missing tool name", NULL);
        return;
    }
    if (args && args->type != JSON_OBJECT) {
        put_error(b, id, INVALID_PARAMS, "Tool arguments must be an object", NULL);
        return;
    }

    json_buf_init(&text);
    if (strcmp(name, "find_concepts") == 0) {
        tool_find(&text, &is_error, cat, args);
    } else if (strcmp(name, "get_checklist") == 0) {
        tool_checklist(&text, &is_error, cat, args, 0);
    } else if (strcmp(name, "review_checklist") == 0) {
        tool_checklist(&text, &is_error, cat, args, 1);
    } else {
        json_buf msg;
        json_buf_init(&msg);
        textf(&msg, "Unknown tool: %s", name);
        put_error(b, id, INVALID_PARAMS, msg.failed ? "Unknown tool" : msg.data, NULL);
        json_buf_free(&msg);
        json_buf_free(&text);
        return;
    }

    begin_result(b, id);
    json_raw(b, ",\"content\":[{\"type\":\"text\",\"text\":");
    json_str(b, text.data ? text.data : "");
    json_raw(b, is_error ? "}],\"isError\":true" : "}],\"isError\":false");
    end_result(b);
    if (text.failed)
        b->failed = 1;
    json_buf_free(&text);
}

static void dispatch(json_buf *b, myna_mcp_state *st, const myna_catalogue *cat,
                     const json_value *id, const char *method, const json_value *params)
{
    if (strcmp(method, "initialize") == 0) {
        st->legacy = 1;
        handle_initialize(b, id, params);
        return;
    }
    if (!st->legacy && !check_modern_meta(b, id, params))
        return;

    if (strcmp(method, "server/discover") == 0) {
        handle_discover(b, id);
    } else if (strcmp(method, "ping") == 0) {
        begin_result(b, id);
        end_result(b);
    } else if (strcmp(method, "tools/list") == 0) {
        begin_result(b, id);
        json_raw(b, ",\"tools\":");
        json_raw(b, TOOLS);
        put_cache_hints(b);
        end_result(b);
    } else if (strcmp(method, "tools/call") == 0) {
        handle_tools_call(b, id, cat, params);
    } else {
        put_error(b, id, METHOD_NOT_FOUND, "Method not found", NULL);
    }
}

char *myna_mcp_handle(myna_mcp_state *st, const myna_catalogue *cat, const char *line)
{
    json_buf b;
    json_value *msg = json_parse(line);

    json_buf_init(&b);
    if (!msg) {
        put_error(&b, NULL, PARSE_ERROR, "Parse error", NULL);
    } else {
        const json_value *id = json_get(msg, "id");
        const char *method = json_string(json_get(msg, "method"));
        const json_value *params = json_get(msg, "params");

        if (msg->type == JSON_OBJECT && !id && method) {
            /* A notification, such as notifications/initialized: no reply. */
            json_free(msg);
            return NULL;
        }
        if (msg->type != JSON_OBJECT || !method ||
            (id->type != JSON_STRING && id->type != JSON_NUMBER)) {
            int valid_id = id && (id->type == JSON_STRING || id->type == JSON_NUMBER);
            put_error(&b, valid_id ? id : NULL, INVALID_REQUEST, "Invalid request", NULL);
        } else if (params && params->type != JSON_OBJECT) {
            put_error(&b, id, INVALID_PARAMS, "params must be an object", NULL);
        } else {
            dispatch(&b, st, cat, id, method, params);
        }
    }
    json_free(msg);

    if (b.failed) {
        json_buf_free(&b);
        json_raw(&b, "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,"
                     "\"message\":\"Out of memory\"}}");
    }
    return b.data;
}

/* Read one line of any length up to MAX_LINE into *buf. Returns its length,
 * -1 at end of input, or -2 if the line was too long (it is skipped). */
static long read_message(FILE *in, char **buf, size_t *cap)
{
    size_t len = 0;
    int ch;

    while ((ch = fgetc(in)) != EOF && ch != '\n') {
        if (len + 2 > *cap) {
            if (*cap >= MAX_LINE) {
                while ((ch = fgetc(in)) != EOF && ch != '\n')
                    ;
                return -2;
            }
            size_t grown_cap = *cap ? *cap * 2 : 4096;
            char *grown = realloc(*buf, grown_cap);
            if (!grown)
                return -2;
            *buf = grown;
            *cap = grown_cap;
        }
        (*buf)[len++] = (char)ch;
    }
    if (ch == EOF && len == 0)
        return -1;
    if (len > 0 && (*buf)[len - 1] == '\r')
        len--;
    if (*cap == 0) {
        *buf = malloc(1);
        if (!*buf)
            return -2;
        *cap = 1;
    }
    (*buf)[len] = '\0';
    return (long)len;
}

int myna_mcp_serve(const myna_catalogue *cat, FILE *in, FILE *out)
{
    myna_mcp_state st;
    char *line = NULL;
    size_t cap = 0;
    long len;

#ifdef _WIN32
    /* Messages end in "\n" exactly; stop Windows from writing "\r\n". */
    _setmode(_fileno(in), _O_BINARY);
    _setmode(_fileno(out), _O_BINARY);
#endif

    myna_mcp_init(&st);
    while ((len = read_message(in, &line, &cap)) != -1) {
        char *reply;
        if (len == -2) {
            reply = myna_mcp_handle(&st, cat, ""); /* answers with a parse error */
        } else if (len == 0) {
            continue;
        } else {
            reply = myna_mcp_handle(&st, cat, line);
        }
        if (reply) {
            fputs(reply, out);
            fputc('\n', out);
            fflush(out);
            free(reply);
        }
    }
    free(line);
    return 0;
}
