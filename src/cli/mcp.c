#include "mcp.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "myna/prompt.h"
#include "myna/search.h"
#include "myna/version.h"

#include "json.h"
#include "report.h"

#define MODERN_VERSION "2026-07-28"
#define MAX_REPORT_RESULTS 100
#define MAX_REPORT_TEXT 500
#define MAX_PATH_TEXT 1000
#define MAX_RESULTS 5
#define MAX_SUGGESTIONS 3
#define MAX_IDS 10
#define MAX_DESCRIPTION 2000
#define MAX_KEYWORDS_LEN 500
#define MAX_CATEGORIES 64
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
    "Myna knows what a good implementation of software features needs: "
    "interface, forms, product features, APIs, backend, system design, "
    "security, mobile, AI features, releases and more. "
    "Before building such a feature, call find_concepts with the user's request "
    "in plain words, plus in keywords any technical names you think apply. If "
    "nothing fits, browse with list_concepts. Then call get_checklist with the "
    "ids you will implement, and "
    "follow its requirements. After writing the code, call review_checklist "
    "with the same ids and check the code against every item. Then call "
    "report_review with your verdicts, so the user can see the review in "
    "their editor.";

#define ID_LIST_SCHEMA                                                          \
    "\"concept_ids\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},"      \
    "\"maxItems\":10,\"description\":\"Concept ids from find_concepts, such as "\
    "modal-dialog.\"},"
#define READ_ONLY                                                               \
    "\"annotations\":{\"readOnlyHint\":true,\"idempotentHint\":true,"           \
    "\"openWorldHint\":false}"

/* One JSON object per tool, joined into the tools/list array. They are
 * separate strings because a single literal this long is more than C
 * compilers are required to support. */
static const char *const TOOLS[] = {
    "{\"name\":\"find_concepts\",\"title\":\"Find concepts\","
    "\"description\":\"Find the software concepts behind a plain-language "
    "request, such as 'close the popup when I click outside'. Returns concept "
    "ids, names, one-line explanations and related concepts to consider.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"description\":"
    "{\"type\":\"string\",\"description\":\"What the user wants, in their own "
    "words.\"},\"keywords\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},"
    "\"maxItems\":10,\"description\":\"Technical names you think apply, such as "
    "'target size' or 'debounce'. Myna searches them together with the "
    "description, which helps when the user's words don't name the concept.\"}},"
    "\"required\":[\"description\"],\"additionalProperties\":false},"
    READ_ONLY "}",
    "{\"name\":\"list_concepts\",\"title\":\"List concepts\","
    "\"description\":\"Browse the catalogue. With no category, lists the "
    "categories, such as ui, forms, security or architecture, and how many "
    "concepts each has. With a category, lists the id and name of every concept "
    "in it. Use it when find_concepts doesn't find what you need.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"category\":"
    "{\"type\":\"string\",\"description\":\"A category name from list_concepts, "
    "such as ui.\"}},\"additionalProperties\":false},"
    READ_ONLY "}",
    "{\"name\":\"get_checklist\",\"title\":\"Get implementation checklist\","
    "\"description\":\"Get what a good implementation must do (requirements) "
    "and how to check it works (done when) for the chosen concepts, as a "
    "ready-to-follow brief. Call it before writing code. Pass concept_ids from "
    "find_concepts, or a description to use the best matches.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{" ID_LIST_SCHEMA
    "\"description\":{\"type\":\"string\",\"description\":\"The user's request "
    "in plain words. Used to find concepts when concept_ids is empty, and "
    "quoted in the brief.\"}},\"additionalProperties\":false},"
    READ_ONLY "}",
    "{\"name\":\"review_checklist\",\"title\":\"Review against checklist\","
    "\"description\":\"Get a numbered review rubric for finished code: every "
    "requirement and done-when check for the chosen concepts, to mark PASS, "
    "FAIL or NOT APPLICABLE. Call it after writing code, then read the code "
    "and answer each item.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{" ID_LIST_SCHEMA
    "\"description\":{\"type\":\"string\",\"description\":\"What was built, in "
    "plain words. Used to find concepts when concept_ids is empty.\"}},"
    "\"additionalProperties\":false},"
    READ_ONLY "}",
    "{\"name\":\"report_review\",\"title\":\"Report review results\","
    "\"description\":\"After checking code against review_checklist, report "
    "your verdict for each item. Myna saves the review in the project, in "
    ".myna/review.json, so the user can see it in their editor.\","
    "\"inputSchema\":{\"type\":\"object\",\"properties\":{"
    "\"project_dir\":{\"type\":\"string\",\"description\":\"Absolute path of "
    "the project's root folder.\"},"
    "\"summary\":{\"type\":\"string\",\"description\":\"What was reviewed, in "
    "a few words, such as Settings page.\"}," ID_LIST_SCHEMA
    "\"results\":{\"type\":\"array\",\"minItems\":1,\"maxItems\":100,"
    "\"description\":\"One entry per rubric item.\",\"items\":{\"type\":"
    "\"object\",\"properties\":{"
    "\"item\":{\"type\":\"string\",\"description\":\"The checklist item, as "
    "written in the rubric.\"},"
    "\"number\":{\"type\":\"integer\",\"minimum\":1,\"description\":\"The "
    "item's number in the rubric.\"},"
    "\"verdict\":{\"type\":\"string\",\"enum\":[\"pass\",\"fail\","
    "\"not_applicable\"]},"
    "\"file\":{\"type\":\"string\",\"description\":\"File that shows it, "
    "relative to project_dir.\"},"
    "\"line\":{\"type\":\"integer\",\"minimum\":1},"
    "\"note\":{\"type\":\"string\",\"description\":\"For a fail: what is "
    "wrong and how to fix it.\"}},"
    "\"required\":[\"item\",\"verdict\"],\"additionalProperties\":false}}},"
    "\"required\":[\"project_dir\",\"results\"],\"additionalProperties\":false},"
    "\"annotations\":{\"readOnlyHint\":false,\"destructiveHint\":false,"
    "\"idempotentHint\":true,\"openWorldHint\":false}}",
};

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
    const json_value *keywords = json_get(args, "keywords");
    myna_match matches[MAX_RESULTS];
    char query[MAX_DESCRIPTION + MAX_KEYWORDS_LEN + 1];

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
    if (keywords && keywords->type != JSON_ARRAY) {
        textf(text, "keywords must be an array of strings, such as [\"debounce\"].");
        *is_error = 1;
        return;
    }

    /* The keywords are appended to the description and searched with it. */
    size_t len = strlen(desc);
    memcpy(query, desc, len + 1);
    for (const json_value *v = keywords ? keywords->child : NULL; v; v = v->next) {
        const char *word = json_string(v);
        size_t wlen = word ? strlen(word) : 0;
        if (!wlen)
            continue;
        if (len + 2 + wlen >= sizeof query) {
            textf(text, "The keywords are too long (max %d characters in all).",
                  MAX_KEYWORDS_LEN);
            *is_error = 1;
            return;
        }
        memcpy(query + len, ". ", 2);
        memcpy(query + len + 2, word, wlen + 1);
        len += 2 + wlen;
    }

    size_t n = myna_search(cat, query, matches, MAX_RESULTS);
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

/* `list_concepts`: the categories with their sizes, or the concepts in one. */
static void tool_list(json_buf *text, int *is_error, const myna_catalogue *cat,
                      const json_value *args)
{
    const char *want = json_string(json_get(args, "category"));
    const char *names[MAX_CATEGORIES];
    size_t counts[MAX_CATEGORIES];
    size_t ncat = 0;

    for (size_t i = 0; i < cat->count; i++) {
        const char *c = cat->items[i].category;
        size_t k = 0;
        while (k < ncat && strcmp(names[k], c) != 0)
            k++;
        if (k == ncat && ncat < MAX_CATEGORIES) {
            names[ncat] = c;
            counts[ncat++] = 0;
        }
        if (k < ncat)
            counts[k]++;
    }

    if (want && *want) {
        size_t shown = 0;
        for (size_t i = 0; i < cat->count; i++) {
            const myna_concept *c = &cat->items[i];
            if (strcmp(c->category, want) != 0)
                continue;
            if (shown == 0)
                textf(text, "Concepts in %s:\n", want);
            textf(text, "\n- %s (id: %s)", c->name, c->id);
            shown++;
        }
        if (shown) {
            textf(text, "\n\nNext: call get_checklist with the ids that fit the request.");
            return;
        }
        textf(text, "Unknown category: %s.", want);
        *is_error = 1;
    }

    if (ncat == 1 && !names[0][0]) {
        textf(text, "%sThis catalogue has no categories. Use find_concepts.",
              want ? " " : "");
        return;
    }
    textf(text, "%sCategories:\n", (want && *want) ? " " : "");
    for (size_t k = 0; k < ncat; k++)
        textf(text, "\n- %s (%d concepts)", names[k][0] ? names[k] : "uncategorised",
              (int)counts[k]);
    textf(text, "\n\nCall list_concepts with a category to see its concepts.");
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
        textf(text, "\nWhen you have checked every item, call report_review with "
                    "project_dir, the concept_ids and one result per numbered item.\n");
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

/* A whole number of at least 1, or 0 if `v` is not one. */
static long positive_int(const json_value *v)
{
    if (!v || v->type != JSON_NUMBER)
        return 0;
    char *end;
    long n = strtol(v->text, &end, 10);
    return (*end == '\0' && n > 0) ? n : 0;
}

/* An optional string member: returns 1 and sets *out (NULL if absent) when
 * valid, 0 when it is not a string or longer than `max`. */
static int optional_text(const json_value *obj, const char *key, size_t max, const char **out)
{
    const json_value *v = json_get(obj, key);
    *out = NULL;
    if (!v)
        return 1;
    if (v->type != JSON_STRING || strlen(v->text) > max)
        return 0;
    *out = v->text;
    return 1;
}

static const char *const VERDICTS[] = {"pass", "fail", "not_applicable"};

static int verdict_index(const char *s)
{
    for (int i = 0; s && i < 3; i++) {
        if (strcmp(s, VERDICTS[i]) == 0)
            return i;
    }
    return -1;
}

/* Check one entry of report_review's results. Writes the reason to `text`
 * and returns 0 if it is invalid. */
static int check_result(json_buf *text, const json_value *r, int index)
{
    const char *item, *file, *note;
    const json_value *line = json_get(r, "line"), *number = json_get(r, "number");

    if (r->type != JSON_OBJECT) {
        textf(text, "results[%d] must be an object with item and verdict.", index);
        return 0;
    }
    if (!optional_text(r, "item", MAX_REPORT_TEXT, &item) || !item || !*item) {
        textf(text, "results[%d] needs the checklist item as text (max %d characters).",
              index, MAX_REPORT_TEXT);
        return 0;
    }
    if (verdict_index(json_string(json_get(r, "verdict"))) < 0) {
        textf(text, "results[%d] needs a verdict: pass, fail or not_applicable.", index);
        return 0;
    }
    if (!optional_text(r, "file", MAX_PATH_TEXT, &file) ||
        !optional_text(r, "note", MAX_REPORT_TEXT, &note) ||
        (line && !positive_int(line)) || (number && !positive_int(number))) {
        textf(text, "results[%d]: file and note must be short strings, and line and "
                    "number whole numbers of at least 1.", index);
        return 0;
    }
    return 1;
}

static void tool_report(json_buf *text, int *is_error, const json_value *args)
{
    const char *dir, *summary;
    const json_value *ids = json_get(args, "concept_ids");
    const json_value *results = json_get(args, "results");
    int counts[3] = {0, 0, 0}, n = 0;

    *is_error = 1;
    if (!optional_text(args, "project_dir", MAX_PATH_TEXT, &dir) || !dir || !*dir) {
        textf(text, "Pass project_dir: the absolute path of the project's root folder.");
        return;
    }
    if (!optional_text(args, "summary", MAX_REPORT_TEXT, &summary)) {
        textf(text, "summary must be a short string (max %d characters).", MAX_REPORT_TEXT);
        return;
    }
    if (ids && ids->type != JSON_ARRAY) {
        textf(text, "concept_ids must be an array of strings.");
        return;
    }
    if (!results || results->type != JSON_ARRAY || !results->child) {
        textf(text, "Pass results: one entry per rubric item, each with item and verdict.");
        return;
    }
    for (const json_value *r = results->child; r; r = r->next, n++) {
        if (n == MAX_REPORT_RESULTS) {
            textf(text, "Too many results (max %d).", MAX_REPORT_RESULTS);
            return;
        }
        if (!check_result(text, r, n))
            return;
        counts[verdict_index(json_string(json_get(r, "verdict")))]++;
    }

    char stamp[32];
    time_t now = time(NULL);
    strftime(stamp, sizeof stamp, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    /* The saved review: one line of JSON. */
    json_buf out;
    char num[24];
    json_buf_init(&out);
    json_raw(&out, "{\"tool\":\"myna\",\"version\":\"" MYNA_VERSION "\",\"created\":");
    json_str(&out, stamp);
    json_raw(&out, ",\"summary\":");
    json_str(&out, summary ? summary : "");
    json_raw(&out, ",\"concepts\":[");
    int first = 1;
    for (const json_value *v = ids ? ids->child : NULL; v; v = v->next) {
        if (v->type != JSON_STRING)
            continue;
        json_raw(&out, first ? "" : ",");
        json_str(&out, v->text);
        first = 0;
    }
    snprintf(num, sizeof num, "%d", counts[0]);
    json_raw(&out, "],\"counts\":{\"pass\":");
    json_raw(&out, num);
    snprintf(num, sizeof num, "%d", counts[1]);
    json_raw(&out, ",\"fail\":");
    json_raw(&out, num);
    snprintf(num, sizeof num, "%d", counts[2]);
    json_raw(&out, ",\"not_applicable\":");
    json_raw(&out, num);
    json_raw(&out, "},\"results\":[");
    for (const json_value *r = results->child; r; r = r->next) {
        const char *keys[] = {"item", "verdict", "file", "note"};
        json_raw(&out, r == results->child ? "{" : ",{");
        for (int k = 0; k < 4; k++) {
            const char *s = json_string(json_get(r, keys[k]));
            if (!s)
                continue;
            json_raw(&out, k ? "," : "");
            json_str(&out, keys[k]);
            json_raw(&out, ":");
            json_str(&out, s);
        }
        const char *numkeys[] = {"number", "line"};
        for (int k = 0; k < 2; k++) {
            long v = positive_int(json_get(r, numkeys[k]));
            if (!v)
                continue;
            snprintf(num, sizeof num, ",\"%s\":%ld", numkeys[k], v);
            json_raw(&out, num);
        }
        json_raw(&out, "}");
    }
    json_raw(&out, "]}");

    char saved[1100], err[1200];
    if (out.failed) {
        textf(text, "Out of memory while saving the review.");
        json_buf_free(&out);
        return;
    }
    if (myna_save_review(dir, out.data, saved, sizeof saved, err, sizeof err) != 0) {
        textf(text, "Could not save the review: %s", err);
        json_buf_free(&out);
        return;
    }
    json_buf_free(&out);

    *is_error = 0;
    textf(text, "Saved the review%s%s: %d passed, %d failed, %d not applicable.\n"
                "Saved to: %s\n", summary ? " of " : "", summary ? summary : "",
          counts[0], counts[1], counts[2], saved);
    if (counts[1] == 0) {
        textf(text, "\nEverything that applies passed.\n");
        return;
    }
    textf(text, "\nFailed:\n");
    for (const json_value *r = results->child; r; r = r->next) {
        if (verdict_index(json_string(json_get(r, "verdict"))) != 1)
            continue;
        const char *file = json_string(json_get(r, "file"));
        const char *note = json_string(json_get(r, "note"));
        long line = positive_int(json_get(r, "line"));
        textf(text, "- %s", json_string(json_get(r, "item")));
        if (file && line)
            textf(text, " (%s:%ld)", file, line);
        else if (file)
            textf(text, " (%s)", file);
        textf(text, "%s%s\n", note ? ": " : "", note ? note : "");
    }
    textf(text, "\nFix the failed items, check them again, and call report_review with "
                "the new results.\n");
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
    } else if (strcmp(name, "list_concepts") == 0) {
        tool_list(&text, &is_error, cat, args);
    } else if (strcmp(name, "get_checklist") == 0) {
        tool_checklist(&text, &is_error, cat, args, 0);
    } else if (strcmp(name, "review_checklist") == 0) {
        tool_checklist(&text, &is_error, cat, args, 1);
    } else if (strcmp(name, "report_review") == 0) {
        tool_report(&text, &is_error, args);
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
        json_raw(b, "[");
        for (size_t i = 0; i < sizeof TOOLS / sizeof TOOLS[0]; i++) {
            json_raw(b, i ? "," : "");
            json_raw(b, TOOLS[i]);
        }
        json_raw(b, "]");
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
