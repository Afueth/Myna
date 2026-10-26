#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L /* opendir, readdir */
#endif

#include "myna/catalogue.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif

#define LINE_MAX_LEN 8192
#define PATH_MAX_LEN 1024
#define MAX_CATALOGUE_FILES 256
/* id..related are required; requirements and done_when are optional. */
#define MIN_FIELD_COUNT 6
#define MAX_FIELD_COUNT 8

void myna_catalogue_init(myna_catalogue *cat)
{
    cat->items = NULL;
    cat->count = 0;
    cat->capacity = 0;
    cat->skipped = 0;
}

int myna_catalogue_add(myna_catalogue *cat, const myna_concept *concept)
{
    if (cat->count == cat->capacity) {
        size_t new_cap = cat->capacity ? cat->capacity * 2 : 16;
        myna_concept *grown = realloc(cat->items, new_cap * sizeof *grown);
        if (!grown)
            return -1;
        cat->items = grown;
        cat->capacity = new_cap;
    }
    cat->items[cat->count++] = *concept;
    return 0;
}

static void copy_trimmed(char *dst, size_t dst_size, const char *src)
{
    while (isspace((unsigned char)*src))
        src++;
    size_t len = strlen(src);
    while (len > 0 && isspace((unsigned char)src[len - 1]))
        len--;
    if (len >= dst_size)
        len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

/*
 * Split one CSV record in place. Fields may be wrapped in double quotes to
 * contain commas, and "" inside quotes stands for a literal quote.
 * Returns the number of fields, or -1 if malformed or over `max_fields`.
 */
static int split_csv(char *line, char **fields, int max_fields)
{
    int n = 0;
    char *p = line;

    for (;;) {
        if (n == max_fields)
            return -1;
        char *out = p;
        fields[n++] = p;

        if (*p == '"') {
            char *r = p + 1;
            for (;;) {
                if (*r == '\0')
                    return -1; /* unterminated quote */
                if (*r == '"') {
                    if (r[1] == '"') {
                        *out++ = '"';
                        r += 2;
                        continue;
                    }
                    r++;
                    break;
                }
                *out++ = *r++;
            }
            p = r;
            if (*p != ',' && *p != '\0')
                return -1; /* text after the closing quote */
        } else {
            while (*p && *p != ',')
                p++;
            out = p;
        }

        char sep = *p;
        *out = '\0';
        if (sep == '\0')
            return n;
        p++;
    }
}

/* Split a ';'-separated list into fixed-size slots, skipping empty items. */
static size_t split_list(char *list, char *slots, size_t slot_size, size_t max_slots)
{
    size_t count = 0;
    char *start = list;

    for (;;) {
        char *semi = strchr(start, ';');
        if (semi)
            *semi = '\0';
        if (count < max_slots) {
            char *slot = slots + count * slot_size;
            copy_trimmed(slot, slot_size, start);
            if (slot[0] != '\0')
                count++;
        }
        if (!semi)
            return count;
        start = semi + 1;
    }
}

int myna_concept_parse(const char *line, myna_concept *out)
{
    char buf[LINE_MAX_LEN];
    char *fields[MAX_FIELD_COUNT];
    size_t len = strlen(line);

    if (len >= sizeof buf)
        return -1;
    memcpy(buf, line, len + 1);
    int n = split_csv(buf, fields, MAX_FIELD_COUNT);
    if (n < MIN_FIELD_COUNT)
        return -1;

    memset(out, 0, sizeof *out);
    copy_trimmed(out->id, sizeof out->id, fields[0]);
    copy_trimmed(out->name, sizeof out->name, fields[1]);
    copy_trimmed(out->explanation, sizeof out->explanation, fields[2]);
    out->synonym_count = split_list(fields[3], &out->synonyms[0][0],
                                    MYNA_TERM_MAX, MYNA_MAX_SYNONYMS);
    out->trigger_count = split_list(fields[4], &out->triggers[0][0],
                                    MYNA_TERM_MAX, MYNA_MAX_TRIGGERS);
    out->related_count = split_list(fields[5], &out->related[0][0],
                                    MYNA_ID_MAX, MYNA_MAX_RELATED);
    if (n > 6)
        out->requirement_count = split_list(fields[6], &out->requirements[0][0],
                                            MYNA_ITEM_MAX, MYNA_MAX_REQUIREMENTS);
    if (n > 7)
        out->done_when_count = split_list(fields[7], &out->done_when[0][0],
                                          MYNA_ITEM_MAX, MYNA_MAX_DONE_WHEN);

    return (out->id[0] && out->name[0]) ? 0 : -1;
}

int myna_catalogue_load_csv(myna_catalogue *cat, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return -1;

    char line[LINE_MAX_LEN];
    int loaded = 0;
    int is_header = 1;

    cat->skipped = 0;
    while (fgets(line, sizeof line, fp)) {
        size_t len = strlen(line);
        if (len == sizeof line - 1 && line[len - 1] != '\n' && !feof(fp)) {
            /* Too long for the buffer: discard the rest of the line. */
            int ch;
            while ((ch = fgetc(fp)) != EOF && ch != '\n')
                ;
            cat->skipped++;
            is_header = 0;
            continue;
        }

        strip_newline(line);
        if (is_header) {
            is_header = 0;
            continue;
        }
        if (line[0] == '\0' || line[0] == '#')
            continue;

        myna_concept c;
        if (myna_concept_parse(line, &c) != 0) {
            cat->skipped++;
            continue;
        }
        if (myna_catalogue_add(cat, &c) != 0) {
            fclose(fp);
            return -1;
        }
        loaded++;
    }

    fclose(fp);
    return loaded;
}

static int ends_with_csv(const char *name)
{
    size_t len = strlen(name);
    return len > 4 && strcmp(name + len - 4, ".csv") == 0;
}

static int compare_names(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* Append a copy of `name` to `names` if it is a .csv file and there is room. */
static void add_csv_name(char **names, int *n, int max, const char *name)
{
    if (*n >= max || !ends_with_csv(name))
        return;
    size_t len = strlen(name) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, name, len);
        names[(*n)++] = copy;
    }
}

/* Collect the names of the .csv files in `dir`. Returns the number found, or
 * -1 if `dir` is not a folder that can be read. */
static int list_csv_files(const char *dir, char **names, int max)
{
    int n = 0;
#ifdef _WIN32
    char pattern[PATH_MAX_LEN];
    WIN32_FIND_DATAA found;
    if (snprintf(pattern, sizeof pattern, "%s/*.csv", dir) >= (int)sizeof pattern)
        return -1;
    DWORD attrs = GetFileAttributesA(dir);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return -1;
    HANDLE h = FindFirstFileA(pattern, &found);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            add_csv_name(names, &n, max, found.cFileName);
    } while (FindNextFileA(h, &found));
    FindClose(h);
#else
    DIR *d = opendir(dir);
    if (!d)
        return -1;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
        add_csv_name(names, &n, max, entry->d_name);
    closedir(d);
#endif
    qsort(names, (size_t)n, sizeof *names, compare_names);
    return n;
}

int myna_catalogue_load(myna_catalogue *cat, const char *path)
{
    char *names[MAX_CATALOGUE_FILES];
    int n = list_csv_files(path, names, MAX_CATALOGUE_FILES);
    if (n < 0)
        return myna_catalogue_load_csv(cat, path);

    /* Every file is loaded in name order; any file that cannot be read makes
     * the whole load fail. */
    int loaded = n == 0 ? -1 : 0;
    size_t skipped = 0;
    for (int i = 0; i < n; i++) {
        char file[PATH_MAX_LEN];
        int fits = snprintf(file, sizeof file, "%s/%s", path, names[i]) < (int)sizeof file;
        int r = (loaded >= 0 && fits) ? myna_catalogue_load_csv(cat, file) : -1;
        if (r < 0) {
            loaded = -1;
        } else {
            loaded += r;
            skipped += cat->skipped;
        }
        free(names[i]);
    }
    cat->skipped = skipped;
    return loaded;
}

void myna_catalogue_free(myna_catalogue *cat)
{
    free(cat->items);
    myna_catalogue_init(cat);
}
