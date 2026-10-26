#include "myna/search.h"

#include <ctype.h>
#include <string.h>

#define MAX_TOKENS 128
#define TOKEN_MAX 32
#define MAX_TERMS (1 + MYNA_MAX_SYNONYMS + MYNA_MAX_TRIGGERS)

/* Scoring: a matched term is worth a base amount plus the weight of each of
 * its content words, so longer, more specific phrases outrank single words.
 * A word's weight falls as more concepts use it, so "passkey" counts for
 * more than "page". */
#define TERM_BASE_SCORE 4
#define NAME_BONUS 4
/* A lone common word is noise, not a match. */
#define MIN_SCORE 2

/* Filler words such as "the" may sit inside a phrase in the query, so
 * "change the language" still matches "change language". */
#define MAX_GAPS 2

/* Text broken into lowercase, lightly stemmed words. */
typedef struct {
    char words[MAX_TOKENS][TOKEN_MAX];
    unsigned char stop[MAX_TOKENS]; /* a stop word, which never scores */
    unsigned char gap[MAX_TOKENS];  /* a filler word allowed inside a phrase */
    size_t count;
    size_t content_count; /* words that are not stop words */
} token_list;

/* The query with a weight for each word, from how many concepts use it. */
typedef struct {
    token_list text;
    int weight[MAX_TOKENS]; /* when the word is part of a matched term */
    int rare[MAX_TOKENS];   /* when the word only appears somewhere in a concept */
    size_t first[MAX_TOKENS]; /* index of the first occurrence of the same word */
} query_words;

static const char *const STOP_WORDS[] = {
    "a",      "all",  "an",   "and",  "are",    "as",    "at",   "be",
    "by",     "can",  "could", "do",  "for",    "from",  "gonna", "how",
    "i",      "if",   "in",   "into", "is",     "it",    "its",  "let",
    "make",   "me",   "must", "my",   "need",   "of",    "on",   "only",
    "or",     "our",  "please", "s",  "should", "so",    "some", "t",
    "that",   "the",  "their", "them", "then",  "this",  "to",   "up",
    "us",     "wanna", "want", "we",  "when",   "will",  "with", "would",
    "you",    "your",
};

static const char *const GAP_WORDS[] = {
    "a",    "all",  "an",   "any",  "her",   "his",   "its",  "my",
    "our",  "s",    "some", "that", "the",   "their", "these", "this",
    "those", "your",
};

static int in_list(const char *word, const char *const *list, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (strcmp(word, list[i]) == 0)
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
    t->stop[t->count] = (unsigned char)in_list(word, STOP_WORDS,
                                               sizeof STOP_WORDS / sizeof STOP_WORDS[0]);
    t->gap[t->count] = (unsigned char)in_list(word, GAP_WORDS,
                                              sizeof GAP_WORDS / sizeof GAP_WORDS[0]);
    if (!t->stop[t->count])
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

/* Equal words match, and so do close variants that share a long prefix and
 * differ only by a common ending, like "animate" and "animation" or "deploy"
 * and "deployment". So "check" does not match "checkbox", "table" does not
 * match "tablet", and "contact" does not match "container". */
static int is_word_ending(const char *s)
{
    static const char *const ENDINGS[] = {
        "",     "e",     "s",      "es",      "ed",     "er",    "ers",  "ing",
        "ion",  "ions",  "ation",  "ations",  "ication", "ment", "ments", "al",
        "ally", "ly",    "able",   "ible",    "ibility", "ility", "ity",  "ive",
        "le",   "or",    "ors",    "y",
    };
    return in_list(s, ENDINGS, sizeof ENDINGS / sizeof ENDINGS[0]);
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
    return is_word_ending(a + prefix) && is_word_ending(b + prefix);
}

/* Find the phrase in the text, allowing a few filler words between its
 * words. On success, writes the text position of each phrase word to `pos`. */
static int find_phrase(const token_list *text, const token_list *phrase, size_t *pos)
{
    if (phrase->count == 0 || phrase->count > text->count)
        return 0;
    for (size_t i = 0; i < text->count; i++) {
        size_t j = 0, k = i, gaps = 0;
        while (j < phrase->count && k < text->count) {
            if (words_match(text->words[k], phrase->words[j])) {
                pos[j++] = k++;
            } else if (j > 0 && text->gap[k] && gaps < MAX_GAPS) {
                gaps++;
                k++;
            } else {
                break;
            }
        }
        if (j == phrase->count)
            return 1;
    }
    return 0;
}

/* Find every content word of the phrase somewhere in the text, in any order.
 * Writes the text position of each content word to `pos`. */
static int find_scattered(const token_list *text, const token_list *phrase, size_t *pos)
{
    for (size_t j = 0; j < phrase->count; j++) {
        if (phrase->stop[j])
            continue;
        size_t k = 0;
        while (k < text->count &&
               (text->stop[k] || !words_match(text->words[k], phrase->words[j])))
            k++;
        if (k == text->count)
            return 0;
        pos[j] = k;
    }
    return 1;
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

/* Weigh each query word by how many concepts use it in their terms. */
static void weigh_query(const myna_catalogue *cat, query_words *q, token_list *scratch)
{
    const token_list *t = &q->text;
    size_t df[MAX_TOKENS] = {0};
    unsigned char seen[MAX_TOKENS];

    for (size_t i = 0; i < t->count; i++) {
        q->first[i] = i;
        for (size_t j = 0; j < i; j++) {
            if (strcmp(t->words[j], t->words[i]) == 0) {
                q->first[i] = j;
                break;
            }
        }
    }

    for (size_t c = 0; c < cat->count; c++) {
        const char *terms[MAX_TERMS];
        size_t n = concept_terms(&cat->items[c], terms);
        memset(seen, 0, sizeof seen);
        for (size_t k = 0; k < n; k++) {
            tokenize(terms[k], scratch);
            for (size_t w = 0; w < scratch->count; w++) {
                for (size_t i = 0; i < t->count; i++) {
                    if (!t->stop[i] && q->first[i] == i && !seen[i] &&
                        strcmp(t->words[i], scratch->words[w]) == 0)
                        seen[i] = 1;
                }
            }
        }
        for (size_t i = 0; i < t->count; i++)
            df[i] += seen[i];
    }

    for (size_t i = 0; i < t->count; i++) {
        size_t d = df[q->first[i]];
        q->weight[i] = d <= 2 ? 4 : d <= 5 ? 3 : d <= 12 ? 2 : 1;
        q->rare[i] = d <= 2 ? 3 : d <= 5 ? 2 : d <= 10 ? 1 : 0;
    }
}

static void add_matched(myna_match *m, const char *term)
{
    for (size_t i = 0; i < m->matched_count; i++) {
        if (m->matched[i] == term)
            return;
    }
    if (m->matched_count < MYNA_MAX_MATCHED)
        m->matched[m->matched_count++] = term;
}

static myna_match score_concept(const myna_concept *c, const query_words *q,
                                token_list *scratch)
{
    const token_list *text = &q->text;
    const char *terms[MAX_TERMS];
    size_t n = concept_terms(c, terms);
    myna_match m = {.concept = c};
    unsigned char used[MAX_TOKENS] = {0};    /* query words scored by a term */
    unsigned char in_name[MAX_TOKENS] = {0};
    const char *found_in[MAX_TOKENS] = {0};  /* first term that has the word */
    size_t pos[MAX_TOKENS];

    for (size_t k = 0; k < n; k++) {
        if (repeats_earlier_term(terms, k))
            continue;
        tokenize(terms[k], scratch);
        if (scratch->content_count == 0)
            continue;

        for (size_t i = 0; i < text->count; i++) {
            if (text->stop[i] || found_in[i])
                continue;
            for (size_t w = 0; w < scratch->count; w++) {
                if (words_match(text->words[i], scratch->words[w])) {
                    found_in[i] = terms[k];
                    in_name[i] = (unsigned char)(k == 0);
                    break;
                }
            }
        }

        /* The whole term in order scores most; all its words apart score
         * only their weights. */
        int score = 0;
        if (find_phrase(text, scratch, pos)) {
            score = TERM_BASE_SCORE + (k == 0 ? NAME_BONUS : 0);
            for (size_t j = 0; j < scratch->count; j++) {
                if (!text->stop[pos[j]]) {
                    score += q->weight[pos[j]];
                    used[pos[j]] = 1;
                }
            }
        } else if (scratch->content_count >= 2 && find_scattered(text, scratch, pos)) {
            for (size_t j = 0; j < scratch->count; j++) {
                if (!scratch->stop[j]) {
                    score += q->weight[pos[j]];
                    used[pos[j]] = 1;
                }
            }
        }
        if (score > 0) {
            m.score += score;
            add_matched(&m, terms[k]);
        }
    }

    /* Telling words found anywhere in the concept's terms add a little, and
     * words from its name a little more. */
    for (size_t i = 0; i < text->count; i++) {
        if (text->stop[i] || used[i] || !found_in[i] || q->first[i] != i)
            continue;
        int score = in_name[i] ? (q->weight[i] > 1 ? q->weight[i] - 1 : 1) : q->rare[i];
        if (score > 0) {
            m.score += score;
            add_matched(&m, found_in[i]);
        }
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
    query_words q;
    token_list scratch;
    size_t found = 0;

    if (!query || max_results == 0)
        return 0;
    tokenize(query, &q.text);
    if (q.text.content_count == 0)
        return 0;
    weigh_query(cat, &q, &scratch);

    for (size_t i = 0; i < cat->count; i++) {
        myna_match m = score_concept(&cat->items[i], &q, &scratch);
        if (m.score >= MIN_SCORE)
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
