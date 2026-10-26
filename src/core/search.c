#include "myna/search.h"

#include <ctype.h>
#include <string.h>

#define MAX_TOKENS 128
#define TOKEN_MAX 32
#define MAX_TERMS (1 + MYNA_MAX_SYNONYMS + MYNA_MAX_TRIGGERS)

/* Scoring: a matched term is worth a base amount plus a bonus per content
 * word, so longer, more specific phrases outrank single words. */
#define TERM_BASE_SCORE 4
#define TERM_WORD_SCORE 4
#define NAME_BONUS 4

/* Text broken into lowercase, lightly stemmed words. */
typedef struct {
    char words[MAX_TOKENS][TOKEN_MAX];
    size_t count;
    size_t content_count; /* words that are not stop words */
} token_list;

static const char *const STOP_WORDS[] = {
    "a",      "all",  "an",   "and",  "are",    "as",    "at",   "be",
    "by",     "can",  "do",   "for",  "from",   "gonna", "how",  "i",
    "if",     "in",   "into", "is",   "it",     "its",   "make", "me",
    "my",     "of",   "on",   "only", "or",     "our",   "please", "so",
    "some",   "that", "the",  "their", "them",  "then",  "this", "to",
    "up",     "us",   "wanna", "want", "we",    "when",  "with", "you",
    "your",
};

static int is_stop_word(const char *word)
{
    for (size_t i = 0; i < sizeof STOP_WORDS / sizeof STOP_WORDS[0]; i++) {
        if (strcmp(word, STOP_WORDS[i]) == 0)
            return 1;
    }
    return 0;
}

/* Crude suffix stripping so "users"/"user" and "clicking"/"click" compare equal. */
static void stem(char *w)
{
    size_t len = strlen(w);
    if (len > 3 && w[len - 1] == 's' && w[len - 2] != 's')
        w[--len] = '\0';
    if (len >= 6 && strcmp(w + len - 3, "ing") == 0)
        w[len - 3] = '\0';
    else if (len >= 6 && strcmp(w + len - 2, "ed") == 0)
        w[len - 2] = '\0';
}

static void add_word(token_list *t, char *word, size_t len)
{
    if (len == 0 || t->count == MAX_TOKENS)
        return;
    word[len] = '\0';
    if (!is_stop_word(word))
        t->content_count++;
    stem(word);
    memcpy(t->words[t->count++], word, strlen(word) + 1);
}

/* Lowercase letters and digits form words, '%' is a word of its own, and
 * everything else separates words. Words past MAX_TOKENS are ignored. */
static void tokenize(const char *text, token_list *t)
{
    char word[TOKEN_MAX];
    size_t len = 0;

    t->count = 0;
    t->content_count = 0;
    for (const char *p = text;; p++) {
        unsigned char ch = (unsigned char)*p;
        if (ch != '\0' && ch < 128 && isalnum(ch)) {
            if (len < TOKEN_MAX - 1)
                word[len++] = (char)tolower(ch);
            continue;
        }
        add_word(t, word, len);
        len = 0;
        if (ch == '%') {
            char pct[2] = "%";
            add_word(t, pct, 1);
        }
        if (ch == '\0')
            break;
    }
}

/* Equal words match, and so do close variants that share a long prefix,
 * like "animate" and "animation". When one word is the start of the other,
 * the rest must be a common ending, so "deploy" matches "deployment" but
 * "check" does not match "checkbox", nor "table" "tablet". */
static int is_word_ending(const char *s)
{
    static const char *const ENDINGS[] = {
        "e", "s", "es", "ed", "er", "ers", "ing", "ion", "ions", "ation", "ations",
        "ment", "ments", "al", "ally", "ly", "able", "ity", "ive", "or", "ors",
    };
    for (size_t i = 0; i < sizeof ENDINGS / sizeof ENDINGS[0]; i++) {
        if (strcmp(s, ENDINGS[i]) == 0)
            return 1;
    }
    return 0;
}

static int words_match(const char *a, const char *b)
{
    size_t prefix = 0;
    while (a[prefix] && a[prefix] == b[prefix])
        prefix++;
    if (a[prefix] == '\0' && b[prefix] == '\0')
        return 1;

    size_t la = strlen(a), lb = strlen(b);
    size_t shorter = la < lb ? la : lb;
    if (prefix < 5 || prefix + 2 < shorter)
        return 0;
    if (prefix == shorter)
        return is_word_ending((la > lb ? a : b) + prefix);
    return 1;
}

static int contains_phrase(const token_list *text, const token_list *phrase)
{
    if (phrase->count == 0 || phrase->count > text->count)
        return 0;
    for (size_t i = 0; i + phrase->count <= text->count; i++) {
        size_t j = 0;
        while (j < phrase->count && words_match(text->words[i + j], phrase->words[j]))
            j++;
        if (j == phrase->count)
            return 1;
    }
    return 0;
}

static int equals_ci(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
    }
    return *a == *b;
}

/* The name first, then synonyms, then trigger phrases. */
static size_t concept_terms(const myna_concept *c, const char **terms)
{
    size_t n = 0;
    terms[n++] = c->name;
    for (size_t i = 0; i < c->synonym_count; i++)
        terms[n++] = c->synonyms[i];
    for (size_t i = 0; i < c->trigger_count; i++)
        terms[n++] = c->triggers[i];
    return n;
}

static int repeats_earlier_term(const char **terms, size_t index)
{
    for (size_t i = 0; i < index; i++) {
        if (equals_ci(terms[i], terms[index]))
            return 1;
    }
    return 0;
}

static myna_match score_concept(const myna_concept *c, const token_list *query,
                                token_list *scratch)
{
    const char *terms[MAX_TERMS];
    size_t n = concept_terms(c, terms);
    myna_match m = {.concept = c};

    for (size_t k = 0; k < n; k++) {
        if (repeats_earlier_term(terms, k))
            continue;
        tokenize(terms[k], scratch);
        if (scratch->content_count == 0 || !contains_phrase(query, scratch))
            continue;

        m.score += TERM_BASE_SCORE + TERM_WORD_SCORE * (int)scratch->content_count;
        if (k == 0)
            m.score += NAME_BONUS;
        if (m.matched_count < MYNA_MAX_MATCHED)
            m.matched[m.matched_count++] = terms[k];
    }
    return m;
}

/* Insert keeping results sorted by score; ties keep catalogue order. */
static void insert_ranked(myna_match *results, size_t *count, size_t max,
                          const myna_match *m)
{
    size_t pos = *count;
    while (pos > 0 && results[pos - 1].score < m->score)
        pos--;
    if (pos >= max)
        return;

    size_t end = *count < max ? *count : max - 1;
    memmove(&results[pos + 1], &results[pos], (end - pos) * sizeof *results);
    results[pos] = *m;
    if (*count < max)
        (*count)++;
}

size_t myna_search(const myna_catalogue *cat, const char *query,
                   myna_match *results, size_t max_results)
{
    token_list words, scratch;
    size_t found = 0;

    if (!query || max_results == 0)
        return 0;
    tokenize(query, &words);
    if (words.content_count == 0)
        return 0;

    for (size_t i = 0; i < cat->count; i++) {
        myna_match m = score_concept(&cat->items[i], &words, &scratch);
        if (m.score > 0)
            insert_ranked(results, &found, max_results, &m);
    }
    return found;
}

static int is_match(const myna_match *matches, size_t count, const myna_concept *c)
{
    for (size_t i = 0; i < count; i++) {
        if (matches[i].concept == c)
            return 1;
    }
    return 0;
}

static int is_listed(const myna_concept **list, size_t count, const myna_concept *c)
{
    for (size_t i = 0; i < count; i++) {
        if (list[i] == c)
            return 1;
    }
    return 0;
}

size_t myna_suggest_related(const myna_catalogue *cat, const myna_match *matches,
                            size_t match_count, const myna_concept **out,
                            size_t max_out)
{
    size_t n = 0;

    for (size_t i = 0; i < match_count; i++) {
        const myna_concept *c = matches[i].concept;
        for (size_t r = 0; r < c->related_count && n < max_out; r++) {
            const myna_concept *rel = myna_find_by_id(cat, c->related[r]);
            if (rel && !is_match(matches, match_count, rel) && !is_listed(out, n, rel))
                out[n++] = rel;
        }
    }
    return n;
}

const myna_concept *myna_find_by_id(const myna_catalogue *cat, const char *id)
{
    for (size_t i = 0; i < cat->count; i++) {
        if (strcmp(cat->items[i].id, id) == 0)
            return &cat->items[i];
    }
    return NULL;
}
