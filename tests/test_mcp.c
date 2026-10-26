#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myna/catalogue.h"

#include "json.h"
#include "mcp.h"
#include "report.h"

#ifdef _WIN32
#include <direct.h>
#define remove_dir(path) _rmdir(path)
#else
#include <unistd.h>
#define remove_dir(path) rmdir(path)
#endif

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,      \
                    __LINE__, #cond);                                   \
            failures++;                                                 \
        }                                                               \
    } while (0)

#define META                                                            \
    "\"_meta\":{\"io.modelcontextprotocol/protocolVersion\":\"2026-07-28\"," \
    "\"io.modelcontextprotocol/clientCapabilities\":{}}"

/* ---- JSON ---- */

static void test_json_parse(void)
{
    json_value *v = json_parse(" {\"a\": [1, -2.5e3, true, null], \"s\": \"x\\\"\\n\\u00e9\\ud83d\\ude00\"} ");
    CHECK(v != NULL);
    const json_value *a = json_get(v, "a");
    CHECK(a && a->type == JSON_ARRAY);
    CHECK(a && a->child && strcmp(a->child->text, "1") == 0);
    CHECK(a && strcmp(a->child->next->text, "-2.5e3") == 0);
    CHECK(a && a->child->next->next->type == JSON_BOOL && a->child->next->next->boolean);
    CHECK(strcmp(json_string(json_get(v, "s")), "x\"\n\xc3\xa9\xf0\x9f\x98\x80") == 0);
    CHECK(json_get(v, "missing") == NULL);
    json_free(v);

    const char *bad[] = {
        "", "{", "{\"a\":}", "[1,]", "{\"a\" 1}", "01", "1.", "\"open",
        "\"\\x\"", "\"\\u12\"", "\"\\ud83d\"", "\"\\u0000\"", "tru", "{} extra",
        "\"tab\there\"",
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        json_value *b = json_parse(bad[i]);
        if (b)
            fprintf(stderr, "accepted bad JSON: %s\n", bad[i]);
        CHECK(b == NULL);
        json_free(b);
    }

    /* Deep nesting is refused rather than overflowing the stack. */
    char deep[200];
    memset(deep, '[', 100);
    memset(deep + 100, ']', 99);
    deep[199] = '\0';
    CHECK(json_parse(deep) == NULL);
}

static void test_json_write_escapes(void)
{
    json_buf b;
    json_buf_init(&b);
    json_str(&b, "a\"b\\c\nd\x01");
    CHECK(strcmp(b.data, "\"a\\\"b\\\\c\\nd\\u0001\"") == 0);
    json_buf_free(&b);
}

/* ---- MCP ---- */

static char *call(myna_mcp_state *st, const myna_catalogue *cat, const char *line)
{
    char *reply = myna_mcp_handle(st, cat, line);
    if (reply) {
        /* Every reply must be one line of valid JSON. */
        CHECK(strchr(reply, '\n') == NULL);
        json_value *v = json_parse(reply);
        CHECK(v != NULL);
        json_free(v);
    }
    return reply;
}

static int has(const char *reply, const char *text)
{
    return reply && strstr(reply, text) != NULL;
}

static void test_legacy_session(const myna_catalogue *cat)
{
    myna_mcp_state st;
    myna_mcp_init(&st);
    char *r;

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
                       "\"params\":{\"protocolVersion\":\"2025-06-18\",\"capabilities\":{}}}");
    CHECK(has(r, "\"protocolVersion\":\"2025-06-18\""));
    CHECK(has(r, "\"tools\":{}"));
    CHECK(has(r, "\"serverInfo\":{\"name\":\"myna\""));
    free(r);

    CHECK(call(&st, cat, "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}") == NULL);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}");
    CHECK(has(r, "\"name\":\"find_concepts\""));
    CHECK(has(r, "\"name\":\"get_checklist\""));
    CHECK(has(r, "\"name\":\"review_checklist\""));
    CHECK(has(r, "\"name\":\"list_concepts\""));
    free(r);

    /* Keywords from the assistant find a concept the user's words don't name. */
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"find_concepts\",\"arguments\":{\"description\":"
                       "\"people keep tapping the wrong thing on phones\","
                       "\"keywords\":[\"target size\"]}}}");
    CHECK(has(r, "(id: target-size)"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"list_concepts\",\"arguments\":{}}}");
    CHECK(has(r, "- ui (") && has(r, "- security ("));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"list_concepts\",\"arguments\":{\"category\":\"ui\"}}}");
    CHECK(has(r, "(id: modal-dialog)") && !has(r, "(id: rest-api)"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"list_concepts\",\"arguments\":{\"category\":\"nope\"}}}");
    CHECK(has(r, "Unknown category: nope") && has(r, "\"isError\":true"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":\"x\",\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"find_concepts\",\"arguments\":{\"description\":"
                       "\"close the popup when I click outside\"}}}");
    CHECK(has(r, "\"id\":\"x\""));
    CHECK(has(r, "(id: outside-click-dismissal)"));
    CHECK(has(r, "\"isError\":false"));
    free(r);

    /* An unknown version falls back to the newest legacy one. */
    myna_mcp_init(&st);
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
                       "\"params\":{\"protocolVersion\":\"2099-01-01\"}}");
    CHECK(has(r, "\"protocolVersion\":\"2025-11-25\""));
    free(r);
}

static void test_modern_requests(const myna_catalogue *cat)
{
    myna_mcp_state st;
    myna_mcp_init(&st);
    char *r;

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"server/discover\","
                       "\"params\":{" META "}}");
    CHECK(has(r, "\"supportedVersions\":[\"2026-07-28\""));
    CHECK(has(r, "\"resultType\":\"complete\""));
    CHECK(has(r, "\"ttlMs\":") && has(r, "\"cacheScope\":\"public\""));
    free(r);

    /* List results are cacheable and must say so. */
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\","
                       "\"params\":{" META "}}");
    CHECK(has(r, "\"tools\":[") && has(r, "\"ttlMs\":") && has(r, "\"cacheScope\":\"public\""));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\",\"params\":{}}");
    CHECK(has(r, "\"code\":-32602"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/list\",\"params\":"
                       "{\"_meta\":{\"io.modelcontextprotocol/protocolVersion\":\"1900-01-01\","
                       "\"io.modelcontextprotocol/clientCapabilities\":{}}}}");
    CHECK(has(r, "\"code\":-32022"));
    CHECK(has(r, "\"requested\":\"1900-01-01\""));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"get_checklist\",\"arguments\":{\"concept_ids\":"
                       "[\"modal-dialog\",\"escape-key\",\"modal-dialog\"]}," META "}}");
    CHECK(has(r, "Requirements:"));
    CHECK(has(r, "review_checklist with concept_ids [\\\"modal-dialog\\\", \\\"escape-key\\\"]"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"review_checklist\",\"arguments\":{\"concept_ids\":"
                       "[\"modal-dialog\"]}," META "}}");
    CHECK(has(r, "PASS, FAIL or NOT APPLICABLE"));
    CHECK(has(r, "\\n1. Close it with the Escape key"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"ping\",\"params\":{" META "}}");
    CHECK(has(r, "\"result\":{\"resultType\":\"complete\""));
    free(r);
}

static void test_errors(const myna_catalogue *cat)
{
    myna_mcp_state st;
    myna_mcp_init(&st);
    st.legacy = 1;
    char *r;

    r = call(&st, cat, "not json");
    CHECK(has(r, "\"id\":null") && has(r, "-32700"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":[1],\"method\":\"ping\"}");
    CHECK(has(r, "-32600"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"nope\"}");
    CHECK(has(r, "-32601"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\","
                       "\"params\":{\"name\":\"nope\"}}");
    CHECK(has(r, "-32602") && has(r, "Unknown tool: nope"));
    free(r);

    /* Mistakes the model can fix come back as tool results with isError. */
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"get_checklist\",\"arguments\":{\"concept_ids\":[\"nope\"]}}}");
    CHECK(has(r, "\"isError\":true") && has(r, "Unknown concept id: nope"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"find_concepts\",\"arguments\":{}}}");
    CHECK(has(r, "\"isError\":true"));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"find_concepts\",\"arguments\":{\"description\":\"quantum banana\"}}}");
    CHECK(has(r, "No matching concepts") && has(r, "\"isError\":false"));
    free(r);
}

static char *read_text_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;
    static char buf[8192];
    size_t n = fread(buf, 1, sizeof buf - 1, fp);
    fclose(fp);
    buf[n] = '\0';
    return buf;
}

static void test_report_review(const myna_catalogue *cat)
{
    const char *dir = "myna-report-test";
    myna_mcp_state st;
    myna_mcp_init(&st);
    st.legacy = 1;
    char *r;

    CHECK(myna_ensure_dir(dir) == 0);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    CHECK(has(r, "\"name\":\"report_review\""));
    free(r);

    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"report_review\",\"arguments\":{\"project_dir\":\"myna-report-test\","
                       "\"summary\":\"Menu\",\"concept_ids\":[\"dropdown-menu\"],\"results\":["
                       "{\"number\":1,\"item\":\"Opens with Enter\",\"verdict\":\"pass\"},"
                       "{\"number\":2,\"item\":\"Stays on screen\",\"verdict\":\"fail\","
                       "\"file\":\"app.js\",\"line\":9,\"note\":\"No positioning\"}]}}}");
    CHECK(has(r, "\"isError\":false"));
    CHECK(has(r, "1 passed, 1 failed, 0 not applicable"));
    CHECK(has(r, "Stays on screen (app.js:9): No positioning"));
    free(r);

    /* The saved review is one line of valid JSON with the counts and results. */
    char *saved = read_text_file("myna-report-test/.myna/review.json");
    CHECK(saved != NULL);
    if (saved) {
        CHECK(strchr(saved, '\n') == strrchr(saved, '\n')); /* only the final newline */
        json_value *v = json_parse(saved);
        CHECK(v != NULL);
        const json_value *counts = json_get(v, "counts");
        CHECK(counts && strcmp(json_get(counts, "fail")->text, "1") == 0);
        CHECK(strcmp(json_string(json_get(v, "summary")), "Menu") == 0);
        json_free(v);
    }
    CHECK(read_text_file("myna-report-test/.myna/history.jsonl") != NULL);

    /* Mistakes the model can fix come back as tool errors. */
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"report_review\",\"arguments\":{\"project_dir\":\"no-such-folder-x\","
                       "\"results\":[{\"item\":\"x\",\"verdict\":\"pass\"}]}}}");
    CHECK(has(r, "\"isError\":true") && has(r, "not an existing folder"));
    free(r);
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"report_review\",\"arguments\":{\"project_dir\":\"myna-report-test\","
                       "\"results\":[{\"item\":\"x\",\"verdict\":\"maybe\"}]}}}");
    CHECK(has(r, "\"isError\":true") && has(r, "needs a verdict"));
    free(r);
    r = call(&st, cat, "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\",\"params\":"
                       "{\"name\":\"report_review\",\"arguments\":{\"project_dir\":\"myna-report-test\","
                       "\"results\":[{\"item\":\"x\",\"verdict\":\"fail\",\"line\":0}]}}}");
    CHECK(has(r, "\"isError\":true"));
    free(r);

    remove("myna-report-test/.myna/review.json");
    remove("myna-report-test/.myna/history.jsonl");
    remove_dir("myna-report-test/.myna");
    remove_dir(dir);
}

int main(int argc, char **argv)
{
    const char *dataset = argc > 1 ? argv[1] : "data/concepts";
    myna_catalogue cat;

    test_json_parse();
    test_json_write_escapes();

    myna_catalogue_init(&cat);
    if (myna_catalogue_load(&cat, dataset) < 0) {
        fprintf(stderr, "cannot load %s\n", dataset);
        return 1;
    }
    test_legacy_session(&cat);
    test_modern_requests(&cat);
    test_errors(&cat);
    test_report_review(&cat);
    myna_catalogue_free(&cat);

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("All MCP tests passed\n");
    return 0;
}
