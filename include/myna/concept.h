#ifndef MYNA_CONCEPT_H
#define MYNA_CONCEPT_H

#include <stddef.h>

#define MYNA_ID_MAX 48
#define MYNA_NAME_MAX 64
#define MYNA_EXPLANATION_MAX 512
#define MYNA_TERM_MAX 64
#define MYNA_MAX_SYNONYMS 8
#define MYNA_MAX_TRIGGERS 16
#define MYNA_MAX_RELATED 8
#define MYNA_ITEM_MAX 128
#define MYNA_MAX_REQUIREMENTS 8
#define MYNA_MAX_DONE_WHEN 6

/* A single entry in the concept catalogue. */
typedef struct {
    char id[MYNA_ID_MAX];
    char name[MYNA_NAME_MAX];
    char explanation[MYNA_EXPLANATION_MAX];

    /* Other names developers use for the concept, e.g. "popup". */
    char synonyms[MYNA_MAX_SYNONYMS][MYNA_TERM_MAX];
    size_t synonym_count;

    /* Everyday phrases that suggest the concept, e.g. "click outside". */
    char triggers[MYNA_MAX_TRIGGERS][MYNA_TERM_MAX];
    size_t trigger_count;

    /* Ids of related concepts. */
    char related[MYNA_MAX_RELATED][MYNA_ID_MAX];
    size_t related_count;

    /* What a good implementation must do, e.g. "Close it with the Escape key". */
    char requirements[MYNA_MAX_REQUIREMENTS][MYNA_ITEM_MAX];
    size_t requirement_count;

    /* How to check it works, e.g. "Keyboard users can't tab out of it". */
    char done_when[MYNA_MAX_DONE_WHEN][MYNA_ITEM_MAX];
    size_t done_when_count;
} myna_concept;

#endif /* MYNA_CONCEPT_H */
