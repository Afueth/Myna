#ifndef MYNA_CATALOGUE_H
#define MYNA_CATALOGUE_H

#include <stddef.h>

#include "myna/concept.h"

typedef struct {
    myna_concept *items;
    size_t count;
    size_t capacity;
    size_t skipped; /* malformed lines ignored by the last load */
} myna_catalogue;

/* Initialise an empty catalogue. */
void myna_catalogue_init(myna_catalogue *cat);

/* Append a copy of `concept`. Returns 0 on success, -1 on allocation failure. */
int myna_catalogue_add(myna_catalogue *cat, const myna_concept *concept);

/*
 * Parse one CSV record (without its line ending) into `out`.
 * Returns 0 on success, -1 if the record is malformed.
 */
int myna_concept_parse(const char *line, myna_concept *out);

/*
 * Load concepts from a CSV file (see docs/DATA_FORMAT.md). Malformed lines
 * are skipped and counted in `cat->skipped`.
 * Returns the number of concepts loaded, or -1 if the file cannot be read.
 */
int myna_catalogue_load_csv(myna_catalogue *cat, const char *path);

/*
 * Load a catalogue from `path`: either one CSV file, or a folder whose .csv
 * files (one per category) are loaded in name order. Malformed lines are
 * skipped and counted in `cat->skipped`.
 * Returns the number of concepts loaded, or -1 if the path, or any file in
 * the folder, cannot be read, or the folder has no .csv files.
 */
int myna_catalogue_load(myna_catalogue *cat, const char *path);

/* Release all memory held by the catalogue. */
void myna_catalogue_free(myna_catalogue *cat);

#endif /* MYNA_CATALOGUE_H */
