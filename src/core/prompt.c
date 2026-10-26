#include "myna/prompt.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "myna/search.h"

#define MAX_RELATED_NAMES 3

/* Output that keeps counting after the buffer is full, like snprintf. */
typedef struct {
    char *buf;
    size_t size;
    size_t len;
} writer;

static void put(writer *w, const char *fmt, ...)
{
    char *dst = w->len < w->size ? w->buf + w->len : NULL;
    size_t room = dst ? w->size - w->len : 0;
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(dst, room, fmt, ap);
    va_end(ap);
    if (n > 0)
        w->len += (size_t)n;
}

static int equals_ci(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
    }
    return *a == *b;
}

/* Whether `item` (item `ii` of concept `ci`) already appeared earlier, in a
 * previous concept or earlier in the same list. `want_done` picks done_when
 * instead of requirements. */
static int seen_before(const myna_concept *const *chosen, size_t ci, size_t ii,
                       const char *item, int want_done)
{
    for (size_t c = 0; c <= ci; c++) {
        const myna_concept *k = chosen[c];
        size_t n = want_done ? k->done_when_count : k->requirement_count;
        if (c == ci)
            n = ii;
        for (size_t i = 0; i < n; i++) {
            const char *other = want_done ? k->done_when[i] : k->requirements[i];
            if (equals_ci(other, item))
                return 1;
        }
    }
    return 0;
}

/* Write the merged requirements (or done_when items) under `heading`. Items
 * are bulleted, or numbered from *number when `number` is not NULL. */
static void put_section(writer *w, const myna_concept *const *chosen, size_t count,
                        const char *heading, int want_done, int *number)
{
    int started = 0;

    for (size_t c = 0; c < count; c++) {
        const myna_concept *k = chosen[c];
        size_t n = want_done ? k->done_when_count : k->requirement_count;
        for (size_t i = 0; i < n; i++) {
            const char *item = want_done ? k->done_when[i] : k->requirements[i];
            if (seen_before(chosen, c, i, item, want_done))
                continue;
            if (!started) {
                put(w, "\n%s\n", heading);
                started = 1;
            }
            if (number)
                put(w, "%d. %s\n", (*number)++, item);
            else
                put(w, "- %s\n", item);
        }
    }
}

static int is_chosen(const myna_concept *const *chosen, size_t count,
                     const myna_concept *c)
{
    for (size_t i = 0; i < count; i++) {
        if (chosen[i] == c)
            return 1;
    }
    return 0;
}

static void put_related(writer *w, const myna_catalogue *cat,
                        const myna_concept *const *chosen, size_t count)
{
    const myna_concept *names[MAX_RELATED_NAMES];
    size_t n = 0;

    for (size_t c = 0; c < count && n < MAX_RELATED_NAMES; c++) {
        for (size_t r = 0; r < chosen[c]->related_count && n < MAX_RELATED_NAMES; r++) {
            const myna_concept *rel = myna_find_by_id(cat, chosen[c]->related[r]);
            if (rel && !is_chosen(chosen, count, rel) && !is_chosen(names, n, rel))
                names[n++] = rel;
        }
    }
    if (n == 0)
        return;

    put(w, "\nAlso consider whether these apply: ");
    for (size_t i = 0; i < n; i++)
        put(w, "%s%s", i ? ", " : "", names[i]->name);
    put(w, "\n");
}

size_t myna_build_prompt(const myna_catalogue *cat, const char *request,
                         const myna_concept *const *chosen, size_t count,
                         char *buf, size_t size)
{
    writer w = {buf, size, 0};

    if (buf && size > 0)
        buf[0] = '\0';

    if (request && *request)
        put(&w, "My request: %s\n\n", request);

    if (count > 0) {
        put(&w, "Build it using these techniques:\n");
        for (size_t c = 0; c < count; c++)
            put(&w, "- %s: %s\n", chosen[c]->name, chosen[c]->explanation);
    }

    put_section(&w, chosen, count, "Requirements:", 0, NULL);
    put_section(&w, chosen, count, "Done when:", 1, NULL);
    put_related(&w, cat, chosen, count);

    return w.len;
}

size_t myna_build_review(const myna_concept *const *chosen, size_t count,
                         char *buf, size_t size)
{
    writer w = {buf, size, 0};
    int number = 1;
    size_t missing = 0;

    if (buf && size > 0)
        buf[0] = '\0';
    if (count == 0)
        return 0;

    put(&w, "Review the code for: ");
    for (size_t c = 0; c < count; c++)
        put(&w, "%s%s", c ? ", " : "", chosen[c]->name);
    put(&w, ".\n\nCheck the code against each numbered item below. For every item, "
            "answer PASS, FAIL or NOT APPLICABLE, and point to the code that shows "
            "it. Then list the fixes needed for each FAIL, most important first.\n");

    put_section(&w, chosen, count, "Requirements:", 0, &number);
    put_section(&w, chosen, count,
                "Done when (check by reading the code, or by running it if you can):",
                1, &number);

    for (size_t c = 0; c < count; c++) {
        if (chosen[c]->requirement_count || chosen[c]->done_when_count)
            continue;
        put(&w, missing++ ? ", %s" : "\nNo checklist yet for: %s", chosen[c]->name);
    }
    if (missing)
        put(&w, ". Review %s against general best practice.\n",
            missing == 1 ? "it" : "them");

    return w.len;
}
