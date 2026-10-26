#include <stdio.h>
#include <string.h>

#include "myna/catalogue.h"
#include "myna/prompt.h"
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
    "modal,Modal dialog,A window on top.,popup,pop up,escape;focus;click,"
    "Close it with the Escape key;Trap focus inside,Escape closes it",
    "click,Outside-click dismissal,Close when clicking outside.,,click outside,modal,"
    "Ignore the opening click;close it with the escape key,Clicking outside closes it",
    "escape,Escape-key handling,Close with Esc.,,press escape,modal,,",
    "focus,Focus management,Keep focus in the dialog.,focus trap,keyboard,modal,,",
    "plain,Plain concept,No checklist.,plain,plain thing,,,",
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

static size_t count_of(const char *haystack, const char *needle)
{
    size_t n = 0;
    for (const char *p = strstr(haystack, needle); p; p = strstr(p + 1, needle))
        n++;
    return n;
}

static void test_full_prompt(const myna_catalogue *cat)
{
    const myna_concept *chosen[] = {myna_find_by_id(cat, "modal"),
                                    myna_find_by_id(cat, "click")};
    char buf[2048];
    size_t len = myna_build_prompt(cat, "close the popup", chosen, 2, buf, sizeof buf);

    CHECK(len == strlen(buf));
    CHECK(strncmp(buf, "My request: close the popup\n", 28) == 0);
    CHECK(strstr(buf, "- Modal dialog: A window on top.\n") != NULL);
    CHECK(strstr(buf, "- Outside-click dismissal: Close when clicking outside.\n") != NULL);
    CHECK(strstr(buf, "\nRequirements:\n") != NULL);
    CHECK(strstr(buf, "- Ignore the opening click\n") != NULL);
    CHECK(strstr(buf, "\nDone when:\n- Escape closes it\n- Clicking outside closes it\n") != NULL);

    /* The same requirement in two concepts, in any case, is listed once. */
    CHECK(count_of(buf, "scape key") == 1);

    /* Related concepts that were not chosen are suggested, chosen ones not. */
    CHECK(strstr(buf, "Also consider whether these apply: Escape-key handling, "
                      "Focus management\n") != NULL);
}

static void test_sections_are_optional(const myna_catalogue *cat)
{
    const myna_concept *chosen[] = {myna_find_by_id(cat, "plain")};
    char buf[512];

    myna_build_prompt(cat, NULL, chosen, 1, buf, sizeof buf);
    CHECK(strncmp(buf, "Build it using these techniques:\n", 33) == 0);
    CHECK(strstr(buf, "Requirements") == NULL);
    CHECK(strstr(buf, "Done when") == NULL);
    CHECK(strstr(buf, "Also consider") == NULL);

    myna_build_prompt(cat, "", NULL, 0, buf, sizeof buf);
    CHECK(buf[0] == '\0');
}

static void test_truncation_reports_full_length(const myna_catalogue *cat)
{
    const myna_concept *chosen[] = {myna_find_by_id(cat, "modal")};
    char full[1024];
    char small[16];

    size_t need = myna_build_prompt(cat, "close the popup", chosen, 1, full, sizeof full);
    CHECK(myna_build_prompt(cat, "close the popup", chosen, 1, NULL, 0) == need);
    CHECK(myna_build_prompt(cat, "close the popup", chosen, 1, small, sizeof small) == need);
    CHECK(strlen(small) == sizeof small - 1);
    CHECK(strncmp(small, full, sizeof small - 1) == 0);
}

static void test_review_rubric(const myna_catalogue *cat)
{
    const myna_concept *chosen[] = {myna_find_by_id(cat, "modal"),
                                    myna_find_by_id(cat, "click"),
                                    myna_find_by_id(cat, "plain")};
    char buf[2048];
    size_t len = myna_build_review(chosen, 3, buf, sizeof buf);

    CHECK(len == strlen(buf));
    const char *title = "Review the code for: Modal dialog, Outside-click dismissal, "
                        "Plain concept.\n";
    CHECK(strncmp(buf, title, strlen(title)) == 0);
    CHECK(strstr(buf, "PASS, FAIL or NOT APPLICABLE") != NULL);

    /* One numbered list across both sections, duplicates removed. */
    CHECK(strstr(buf, "\nRequirements:\n1. Close it with the Escape key\n"
                      "2. Trap focus inside\n3. Ignore the opening click\n") != NULL);
    CHECK(strstr(buf, "4. Escape closes it\n5. Clicking outside closes it\n") != NULL);
    CHECK(strstr(buf, "\nNo checklist yet for: Plain concept. Review it against "
                      "general best practice.\n") != NULL);

    CHECK(myna_build_review(chosen, 0, buf, sizeof buf) == 0 && buf[0] == '\0');
}

int main(void)
{
    myna_catalogue cat;
    load_sample(&cat);
    test_full_prompt(&cat);
    test_sections_are_optional(&cat);
    test_truncation_reports_full_length(&cat);
    test_review_rubric(&cat);
    myna_catalogue_free(&cat);

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("All prompt tests passed\n");
    return 0;
}
