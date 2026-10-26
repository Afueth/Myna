#ifndef MYNA_PROMPT_H
#define MYNA_PROMPT_H

#include <stddef.h>

#include "myna/catalogue.h"

/*
 * Build a prompt for an AI assistant from the user's request and the chosen
 * concepts: the request, each concept with its explanation, their merged
 * requirements and "done when" checks (duplicates removed), and related
 * concepts worth considering. `request` may be NULL or empty.
 *
 * Writes at most `size` bytes including the terminating NUL, like snprintf,
 * and returns the length the full prompt needs. A return value of `size` or
 * more means the prompt was truncated.
 */
size_t myna_build_prompt(const myna_catalogue *cat, const char *request,
                         const myna_concept *const *chosen, size_t count,
                         char *buf, size_t size);

/*
 * Build a review rubric for checking finished code against the chosen
 * concepts: their merged requirements and "done when" checks as one numbered
 * list, with instructions to mark each item PASS, FAIL or NOT APPLICABLE.
 * Concepts without a checklist are named at the end. Writes like
 * myna_build_prompt() and returns the length the full rubric needs; returns 0
 * when `count` is 0.
 */
size_t myna_build_review(const myna_concept *const *chosen, size_t count,
                         char *buf, size_t size);

#endif /* MYNA_PROMPT_H */
