#include <stdio.h>
#include <string.h>

#include "myna/catalogue.h"
#include "myna/search.h"

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,      \
                    __LINE__, #cond);                                   \
            failures++;                                                 \
        }                                                               \
    } while (0)

static const char *const SAMPLE[] = {
    "flags,Feature flags,Turn features on or off.,feature toggle,release a feature;turn off a feature,rollout",
    "rollout,Percentage rollout,Release to a share of users.,staged rollout,% of users;some users,flags;targeting",
    "targeting,Feature targeting,Choose who sees a feature.,,specific users;of users,flags",
    "click,Outside-click dismissal,Close when clicking outside.,light dismiss,click outside;close it when i click,modal;escape",
    "modal,Modal dialog,A window on top.,popup,popup;pop up,click;escape;focus",
    "escape,Escape-key handling,Close with Esc.,,press escape,modal",
    "focus,Focus management,Keep focus in the dialog.,focus trap,keyboard,modal",
};

static void load_sample(myna_catalogue *cat)
{
    myna_catalogue_init(cat);
    for (size_t i = 0; i < sizeof SAMPLE / sizeof SAMPLE[0]; i++) {
        myna_concept c;
        CHECK(myna_concept_parse(SAMPLE[i], &c) == 0);
        CHECK(myna_catalogue_add(cat, &c) == 0);
    }
}

static int has_id(const myna_match *matches, size_t n, const char *id)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(matches[i].concept->id, id) == 0)
            return 1;
    }
    return 0;
}

/* ---- parsing ---- */

static void test_parse_fields_and_lists(void)
{
    myna_concept c;
    CHECK(myna_concept_parse("modal, Modal dialog ,\"A window, on top\","
                             "popup; dialog ;;lightbox,pop up;click,backdrop;focus",
                             &c) == 0);
    CHECK(strcmp(c.id, "modal") == 0);
    CHECK(strcmp(c.name, "Modal dialog") == 0);
    CHECK(strcmp(c.explanation, "A window, on top") == 0);
    CHECK(c.synonym_count == 3);
    CHECK(strcmp(c.synonyms[1], "dialog") == 0);
    CHECK(strcmp(c.synonyms[2], "lightbox") == 0);
    CHECK(c.trigger_count == 2);
    CHECK(c.related_count == 2);
    CHECK(strcmp(c.related[1], "focus") == 0);
}

static void test_parse_checklist_columns(void)
{
    myna_concept c;
    CHECK(myna_concept_parse("modal,Modal,A window.,popup,pop up,,"
                             "\"Close with Escape; Trap focus, then restore it\",Esc closes it",
                             &c) == 0);
    CHECK(c.requirement_count == 2);
    CHECK(strcmp(c.requirements[1], "Trap focus, then restore it") == 0);
    CHECK(c.done_when_count == 1);
    CHECK(strcmp(c.done_when[0], "Esc closes it") == 0);

    /* The checklist columns are optional. */
    CHECK(myna_concept_parse("modal,Modal,A window.,popup,pop up,", &c) == 0);
    CHECK(c.requirement_count == 0 && c.done_when_count == 0);
    CHECK(myna_concept_parse("modal,Modal,A window.,popup,pop up,,Close with Escape", &c) == 0);
    CHECK(c.requirement_count == 1 && c.done_when_count == 0);
}

static void test_parse_escaped_quote_and_empty_lists(void)
{
    myna_concept c;
    CHECK(myna_concept_parse("q,Quote,\"Say \"\"hi\"\"\",,,", &c) == 0);
    CHECK(strcmp(c.explanation, "Say \"hi\"") == 0);
    CHECK(c.synonym_count == 0 && c.trigger_count == 0 && c.related_count == 0);
}

static void test_parse_rejects_malformed(void)
{
    myna_concept c;
    CHECK(myna_concept_parse("a,b,c", &c) != 0);               /* too few fields */
    CHECK(myna_concept_parse("a,b,c,d,e,f,g,h,i", &c) != 0);   /* too many fields */
    CHECK(myna_concept_parse("a,b,\"oops,d,e,f", &c) != 0);    /* unterminated quote */
    CHECK(myna_concept_parse("a,b,\"x\"y,d,e,f", &c) != 0);    /* text after quote */
    CHECK(myna_concept_parse(",Name,x,,,", &c) != 0);          /* missing id */
    CHECK(myna_concept_parse("", &c) != 0);
}

static void test_parse_caps_list_length(void)
{
    char line[1024] = "id,Name,Explain,";
    for (int i = 0; i < 20; i++)
        strcat(line, "syn;");
    strcat(line, ",,");

    myna_concept c;
    CHECK(myna_concept_parse(line, &c) == 0);
    CHECK(c.synonym_count == MYNA_MAX_SYNONYMS);
}

/* ---- search ---- */

static void test_empty_and_meaningless_queries(const myna_catalogue *cat)
{
    myna_match m[5];
    CHECK(myna_search(cat, "", m, 5) == 0);
    CHECK(myna_search(cat, "   ?!  ", m, 5) == 0);
    CHECK(myna_search(cat, "make it the", m, 5) == 0); /* stop words only */
    CHECK(myna_search(cat, NULL, m, 5) == 0);
    CHECK(myna_search(cat, "quantum banana", m, 5) == 0);
}

static void test_case_and_punctuation_ignored(const myna_catalogue *cat)
{
    myna_match m[5];
    size_t n = myna_search(cat, "CLICK... OUTSIDE!!!", m, 5);
    CHECK(n == 1);
    CHECK(n == 1 && strcmp(m[0].concept->id, "click") == 0);
    CHECK(n == 1 && strcmp(m[0].matched[0], "click outside") == 0);
}

static void test_word_variants_match(const myna_catalogue *cat)
{
    myna_match m[5];
    CHECK(has_id(m, myna_search(cat, "clicking outside", m, 5), "click"));
    CHECK(has_id(m, myna_search(cat, "turning off a features", m, 5), "flags"));
}

static void test_percent_example_ranks_phrases_first(const myna_catalogue *cat)
{
    myna_match m[5];
    size_t n = myna_search(cat, "Release a feature to only 10% of users", m, 5);
    CHECK(n == 3);
    CHECK(n == 3 && strcmp(m[0].concept->id, "flags") == 0);
    CHECK(n == 3 && strcmp(m[1].concept->id, "rollout") == 0);
    CHECK(n == 3 && strcmp(m[2].concept->id, "targeting") == 0);
    CHECK(n == 3 && m[1].score > m[2].score);
}

static void test_duplicate_terms_count_once(const myna_catalogue *cat)
{
    myna_match m[5];
    size_t n = myna_search(cat, "close the popup when I click outside", m, 5);
    CHECK(n == 2);
    CHECK(n == 2 && strcmp(m[0].concept->id, "click") == 0);
    CHECK(n == 2 && strcmp(m[1].concept->id, "modal") == 0);
    /* "popup" is both a synonym and a trigger of modal but scores once. */
    CHECK(n == 2 && m[1].matched_count == 1);
}

static void test_max_results_keeps_best(const myna_catalogue *cat)
{
    myna_match m[1];
    size_t n = myna_search(cat, "Release a feature to only 10% of users", m, 1);
    CHECK(n == 1);
    CHECK(n == 1 && strcmp(m[0].concept->id, "flags") == 0);
}

static void test_very_long_query(const myna_catalogue *cat)
{
    static char query[10000];
    size_t len = 0;
    while (len + 15 < sizeof query) {
        memcpy(query + len, "click outside ", 14);
        len += 14;
    }
    query[len] = '\0';

    myna_match m[5];
    CHECK(has_id(m, myna_search(cat, query, m, 5), "click"));
}

static void test_suggest_related_skips_results(const myna_catalogue *cat)
{
    myna_match m[5];
    size_t n = myna_search(cat, "close the popup when I click outside", m, 5);

    const myna_concept *rel[5];
    size_t r = myna_suggest_related(cat, m, n, rel, 5);
    CHECK(r == 2);
    CHECK(r == 2 && strcmp(rel[0]->id, "escape") == 0);
    CHECK(r == 2 && strcmp(rel[1]->id, "focus") == 0);
}

static void test_find_by_id(const myna_catalogue *cat)
{
    const myna_concept *c = myna_find_by_id(cat, "modal");
    CHECK(c != NULL && strcmp(c->name, "Modal dialog") == 0);
    CHECK(myna_find_by_id(cat, "missing") == NULL);
}

/* ---- bundled dataset ---- */

static void test_dataset(const char *path)
{
    myna_catalogue cat;
    myna_catalogue_init(&cat);
    int loaded = myna_catalogue_load_csv(&cat, path);
    if (loaded < 0) {
        fprintf(stderr, "Could not read %s\n", path);
        failures++;
        return;
    }
    CHECK(loaded >= 30);
    CHECK(cat.skipped == 0);

    for (size_t i = 0; i < cat.count; i++) {
        for (size_t r = 0; r < cat.items[i].related_count; r++) {
            if (!myna_find_by_id(&cat, cat.items[i].related[r])) {
                fprintf(stderr, "%s: unknown related id '%s'\n",
                        cat.items[i].id, cat.items[i].related[r]);
                failures++;
            }
        }
    }

    myna_match m[5];
    size_t n = myna_search(&cat, "Make this popup modern, blur the background, "
                                 "animate it, and close it when I click outside.",
                           m, 5);
    CHECK(has_id(m, n, "modal-dialog"));
    CHECK(has_id(m, n, "backdrop-overlay"));
    CHECK(has_id(m, n, "entry-exit-transition"));
    CHECK(has_id(m, n, "outside-click-dismissal"));

    const myna_concept *rel[3];
    size_t r = myna_suggest_related(&cat, m, n, rel, 3);
    int focus = 0, escape = 0;
    for (size_t i = 0; i < r; i++) {
        focus |= strcmp(rel[i]->id, "focus-management") == 0;
        escape |= strcmp(rel[i]->id, "escape-key") == 0;
    }
    CHECK(focus && escape);

    n = myna_search(&cat, "i wanna implement a modal size larger", m, 5);
    CHECK(has_id(m, n, "modal-dialog"));
    CHECK(has_id(m, n, "element-sizing"));

    n = myna_search(&cat, "Release a feature to only 10% of users", m, 5);
    CHECK(has_id(m, n, "feature-flags"));
    CHECK(has_id(m, n, "percentage-rollout"));
    CHECK(has_id(m, n, "feature-targeting"));
    CHECK(has_id(m, n, "gradual-deployment"));

    /* A word that starts another only matches with a real ending:
     * "check" is not "checkbox", and "graph" is not "graphql". */
    n = myna_search(&cat, "check the npm packages for vulnerabilities", m, 5);
    CHECK(has_id(m, n, "dependency-security"));
    CHECK(!has_id(m, n, "checkbox"));
    n = myna_search(&cat, "show a graph of sales per month", m, 5);
    CHECK(has_id(m, n, "data-visualization"));
    CHECK(!has_id(m, n, "graphql-api"));

    myna_catalogue_free(&cat);
}

int main(int argc, char **argv)
{
    const char *dataset = argc > 1 ? argv[1] : "data/concepts.csv";

    test_parse_fields_and_lists();
    test_parse_checklist_columns();
    test_parse_escaped_quote_and_empty_lists();
    test_parse_rejects_malformed();
    test_parse_caps_list_length();

    myna_catalogue cat;
    load_sample(&cat);
    test_empty_and_meaningless_queries(&cat);
    test_case_and_punctuation_ignored(&cat);
    test_word_variants_match(&cat);
    test_percent_example_ranks_phrases_first(&cat);
    test_duplicate_terms_count_once(&cat);
    test_max_results_keeps_best(&cat);
    test_very_long_query(&cat);
    test_suggest_related_skips_results(&cat);
    test_find_by_id(&cat);
    myna_catalogue_free(&cat);

    test_dataset(dataset);

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
