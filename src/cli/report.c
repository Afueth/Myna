#include "report.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define make_dir(path) _mkdir(path)
#else
#include <sys/types.h>
#define make_dir(path) mkdir(path, 0755)
#endif

#define REVIEW_DIR ".myna"
#define PATH_MAX_LEN 1024

int myna_is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

int myna_ensure_dir(const char *path)
{
    if (myna_is_dir(path))
        return 0;
    return make_dir(path) == 0 || errno == EEXIST ? 0 : -1;
}

/* Write `text` to `path` through a temporary file, so readers never see a
 * half-written review. */
static int write_file(const char *path, const char *text)
{
    char tmp[PATH_MAX_LEN + 64];
    int len = snprintf(tmp, sizeof tmp, "%s.tmp", path);
    if (len < 0 || len >= (int)sizeof tmp)
        return -1;

    FILE *fp = fopen(tmp, "wb");
    if (!fp)
        return -1;
    int ok = fputs(text, fp) >= 0 && fputc('\n', fp) != EOF;
    ok = (fclose(fp) == 0) && ok;
    if (!ok) {
        remove(tmp);
        return -1;
    }
    remove(path); /* rename does not replace an existing file on Windows */
    return rename(tmp, path) == 0 ? 0 : -1;
}

static int append_line(const char *path, const char *text)
{
    FILE *fp = fopen(path, "ab");
    if (!fp)
        return -1;
    int ok = fputs(text, fp) >= 0 && fputc('\n', fp) != EOF;
    return (fclose(fp) == 0 && ok) ? 0 : -1;
}

int myna_save_review(const char *project_dir, const char *json, char *saved,
                     size_t saved_size, char *err, size_t err_size)
{
    /* Room for the folder plus "/history.jsonl", so the joins below never
     * truncate once the folder itself fits. */
    char dir[PATH_MAX_LEN], path[PATH_MAX_LEN + 32];

    if (!myna_is_dir(project_dir)) {
        snprintf(err, err_size, "project_dir is not an existing folder: %s", project_dir);
        return -1;
    }
    int len = snprintf(dir, sizeof dir, "%s/%s", project_dir, REVIEW_DIR);
    if (len < 0 || len >= (int)sizeof dir) {
        snprintf(err, err_size, "project_dir is too long");
        return -1;
    }
    if (myna_ensure_dir(dir) != 0) {
        snprintf(err, err_size, "could not create %s", dir);
        return -1;
    }

    snprintf(path, sizeof path, "%s/history.jsonl", dir);
    if (append_line(path, json) != 0) {
        snprintf(err, err_size, "could not write %s", path);
        return -1;
    }
    snprintf(path, sizeof path, "%s/review.json", dir);
    if (write_file(path, json) != 0) {
        snprintf(err, err_size, "could not write %s", path);
        return -1;
    }
    snprintf(saved, saved_size, "%s", path);
    return 0;
}
