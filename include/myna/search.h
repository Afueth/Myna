#ifndef MYNA_SEARCH_H
#define MYNA_SEARCH_H

#include <stddef.h>

#include "myna/catalogue.h"

#define MYNA_MAX_MATCHED 4

typedef struct {
    const myna_concept *concept;
    int score;
    /* The concept's own terms that were found in the query. */
    const char *matched[MYNA_MAX_MATCHED];
    size_t matched_count;
} myna_match;

/*
 * Find the concepts a plain-language description is most likely about,
 * highest score first. Writes up to `max_results` matches and returns the
 * number written; 0 for an empty or unrecognised description.
 */
size_t myna_search(const myna_catalogue *cat, const char *query,
                   myna_match *results, size_t max_results);

/*
 * Collect concepts related to `matches` that are not matches themselves, in
 * the order they are first referenced. Returns the number written to `out`.
 */
size_t myna_suggest_related(const myna_catalogue *cat, const myna_match *matches,
                            size_t match_count, const myna_concept **out,
                            size_t max_out);

/*
 * The concepts most closely related to `chosen` that are not chosen
 * themselves: a concept that several chosen concepts list as related comes
 * before one that only a single concept lists; ties keep the order they are
 * first listed in. Writes up to `max_out` to `out` and returns the number.
 */
size_t myna_closest_related(const myna_catalogue *cat, const myna_concept *const *chosen,
                            size_t count, const myna_concept **out, size_t max_out);

/* Find a concept by exact id. Returns NULL if not found. */
const myna_concept *myna_find_by_id(const myna_catalogue *cat, const char *id);

#endif /* MYNA_SEARCH_H */
