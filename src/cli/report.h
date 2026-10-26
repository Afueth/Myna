#ifndef MYNA_REPORT_H
#define MYNA_REPORT_H

#include <stddef.h>

/*
 * Saving review results inside a project, so editors can show them:
 * <project>/.myna/review.json holds the latest review and
 * <project>/.myna/history.jsonl gets one line per review.
 */

/* Returns 1 if `path` is an existing directory. */
int myna_is_dir(const char *path);

/* Create `path` as a directory unless it exists. Returns 0 on success. */
int myna_ensure_dir(const char *path);

/*
 * Save `json` (one line, no trailing newline) as the latest review of the
 * project in `project_dir`, and append it to the history. Writes the path of
 * review.json to `saved`. Returns 0 on success, or -1 and a message in `err`.
 */
int myna_save_review(const char *project_dir, const char *json, char *saved,
                     size_t saved_size, char *err, size_t err_size);

#endif /* MYNA_REPORT_H */
