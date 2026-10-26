#include "myna/cli.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myna/catalogue.h"
#include "myna/prompt.h"
#include "myna/search.h"
#include "myna/version.h"

#include "logo.h"
#include "mcp.h"
#include "terminal.h"

#define DEFAULT_CATALOGUE "data/concepts.csv"
#define MAX_RESULTS 5
#define MAX_SUGGESTIONS 3
#define INPUT_MAX 500
#define WRAP_WIDTH 76
#define PATH_MAX_LEN 1024
#define MAX_REVIEW_IDS 10

#define STYLE_TITLE "\033[1;38;2;255;196;0m"
#define STYLE_DIM "\033[38;2;150;150;175m"
#define STYLE_RESET "\033[0m"

typedef enum { INPUT_OK, INPUT_EMPTY, INPUT_TOO_LONG, INPUT_EOF } input_status;

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-d FILE] [command]\n"
            "\n"
            "With no command, starts an interactive session.\n"
            "\n"
            "Commands:\n"
            "  search <description>   Find concepts for a plain-language description\n"
            "  prompt <description>   Build an AI prompt from the best matching concepts\n"
            "  review <id>...         Print a checklist for reviewing finished code\n"
            "  show <id>              Show a single concept by id\n"
            "  list                   List every concept\n"
            "  mcp                    Run as an MCP server for AI coding tools (stdio)\n"
            "\n"
            "Options:\n"
            "  -d FILE                Catalogue CSV (default: " DEFAULT_CATALOGUE ")\n"
            "  -h, --help             Show this help\n"
            "  -v, --version          Show the version\n",
            prog);
}

/* The colour logo with the title beside it, or a plain text title. */
static void print_banner(FILE *out, int colour)
{
    if (!colour) {
        fprintf(out, "\nMYNA - Developer Intent Engine\n");
        fprintf(out, "==============================\n");
        return;
    }

    const char *beside[] = {
        STYLE_TITLE "M Y N A" STYLE_RESET,
        "Developer Intent Engine",
        "",
        STYLE_DIM "Turn what you want into the" STYLE_RESET,
        STYLE_DIM "technical language AI understands." STYLE_RESET,
    };
    int count = (int)(sizeof beside / sizeof beside[0]);
    int first = (MYNA_LOGO_LINES - count) / 2;

    fputc('\n', out);
    for (int i = 0; i < MYNA_LOGO_LINES; i++) {
        fprintf(out, "  %s", MYNA_LOGO[i]);
        if (i >= first && i < first + count)
            fprintf(out, "   %s", beside[i - first]);
        fputc('\n', out);
    }
}

static void trim(char *s)
{
    size_t len = strlen(s);
    size_t start = 0;
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
    while (isspace((unsigned char)s[start]))
        start++;
    memmove(s, s + start, len - start + 1);
}

/* Read one line and trim it. A line that does not fit is discarded whole. */
static input_status read_line(FILE *in, char *buf, size_t size)
{
    if (!fgets(buf, (int)size, in))
        return INPUT_EOF;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (!feof(in)) {
        int ch;
        while ((ch = fgetc(in)) != EOF && ch != '\n')
            ;
        return INPUT_TOO_LONG;
    }
    trim(buf);
    return buf[0] ? INPUT_OK : INPUT_EMPTY;
}

/* Print `text` word-wrapped, with every line indented by `indent` spaces. */
static void print_wrapped(FILE *out, const char *text, int indent)
{
    int col = 0;
    const char *p = text;

    while (*p) {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        const char *end = p;
        while (*end && *end != ' ')
            end++;
        int wlen = (int)(end - p);

        if (col > 0 && col + 1 + wlen > WRAP_WIDTH - indent) {
            fputc('\n', out);
            col = 0;
        }
        if (col == 0) {
            fprintf(out, "%*s", indent, "");
        } else {
            fputc(' ', out);
            col++;
        }
        fwrite(p, 1, (size_t)wlen, out);
        col += wlen;
        p = end;
    }
    if (col > 0)
        fputc('\n', out);
}

static void print_results(FILE *out, const myna_catalogue *cat,
                          const myna_match *matches, size_t count)
{
    fprintf(out, "\nPossible concepts:\n");
    for (size_t i = 0; i < count; i++) {
        const myna_match *m = &matches[i];
        fprintf(out, "\n  %d. %s\n", (int)(i + 1), m->concept->name);
        print_wrapped(out, m->concept->explanation, 5);
        fprintf(out, "     Matched: ");
        for (size_t j = 0; j < m->matched_count; j++)
            fprintf(out, "%s\"%s\"", j ? ", " : "", m->matched[j]);
        fputc('\n', out);
    }

    const myna_concept *related[MAX_SUGGESTIONS];
    size_t n = myna_suggest_related(cat, matches, count, related, MAX_SUGGESTIONS);
    if (n > 0) {
        fprintf(out, "\nAlso consider: ");
        for (size_t i = 0; i < n; i++)
            fprintf(out, "%s%s", i ? ", " : "", related[i]->name);
        fputc('\n', out);
    }
}

static void print_details(FILE *out, const myna_catalogue *cat, const myna_concept *c)
{
    size_t name_len = strlen(c->name);

    fprintf(out, "\n%s\n", c->name);
    for (size_t i = 0; i < name_len; i++)
        fputc('-', out);
    fputc('\n', out);
    print_wrapped(out, c->explanation, 0);

    if (c->synonym_count > 0) {
        fprintf(out, "\nAlso called: ");
        for (size_t i = 0; i < c->synonym_count; i++)
            fprintf(out, "%s%s", i ? ", " : "", c->synonyms[i]);
        fputc('\n', out);
    }

    if (c->related_count > 0) {
        fprintf(out, "\nRelated concepts:\n");
        for (size_t i = 0; i < c->related_count; i++) {
            const myna_concept *rel = myna_find_by_id(cat, c->related[i]);
            if (!rel)
                continue;
            fprintf(out, "  - %s\n", rel->name);
            print_wrapped(out, rel->explanation, 4);
        }
    }
}

static void print_list(FILE *out, const myna_catalogue *cat)
{
    for (size_t i = 0; i < cat->count; i++)
        fprintf(out, "%-26s %s\n", cat->items[i].id, cat->items[i].name);
}

/* Returns the 0-based index for a choice like "2", or -1 if out of range. */
static int parse_choice(const char *s, size_t count)
{
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < 1 || v > (long)count)
        return -1;
    return (int)(v - 1);
}

static int is_key(const char *input, char key)
{
    return input[0] != '\0' && input[1] == '\0' &&
           toupper((unsigned char)input[0]) == key;
}

static int is_quit(const char *input)
{
    return is_key(input, 'Q') || strcmp(input, "quit") == 0 ||
           strcmp(input, "exit") == 0;
}

static size_t run_search(FILE *out, const myna_catalogue *cat, const char *query,
                         myna_match *matches)
{
    size_t count = myna_search(cat, query, matches, MAX_RESULTS);
    if (count == 0)
        fprintf(out, "\nNo matching concepts found. Try describing what the user "
                     "should see or do,\nfor example \"make the popup bigger\" or "
                     "\"close the popup when I click outside\".\n");
    else
        print_results(out, cat, matches, count);
    return count;
}

/*
 * Print the prompt for `chosen`. In the interactive session it is framed and,
 * when `out` is a terminal, copied to the clipboard; otherwise it is printed
 * bare so it can be piped. Returns 0, or -1 if memory runs out.
 */
static int print_prompt(FILE *out, const myna_catalogue *cat, const char *request,
                        const myna_concept *const *chosen, size_t count,
                        int interactive)
{
    size_t len = myna_build_prompt(cat, request, chosen, count, NULL, 0);
    char *text = malloc(len + 1);
    if (!text) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }
    myna_build_prompt(cat, request, chosen, count, text, len + 1);

    if (!interactive) {
        fputs(text, out);
    } else {
        fprintf(out, "\n------------ Copy this into your AI assistant ------------\n\n");
        fputs(text, out);
        fprintf(out, "\n----------------------------------------------------------\n");
        if (myna_stream_is_terminal(out) && myna_copy_to_clipboard(text))
            fprintf(out, "Copied to clipboard.\n");
    }
    free(text);
    return 0;
}

/*
 * Parse a choice like "1,3" or "2 4" into the chosen matches, in result
 * order. Returns the number chosen, or 0 if any part is not a valid number.
 */
static size_t parse_selection(const char *s, const myna_match *matches, size_t count,
                              const myna_concept **chosen)
{
    int picked[MAX_RESULTS] = {0};

    while (*s) {
        if (*s == ',' || isspace((unsigned char)*s)) {
            s++;
            continue;
        }
        char *end;
        long v = strtol(s, &end, 10);
        if (end == s || v < 1 || v > (long)count)
            return 0;
        picked[v - 1] = 1;
        s = end;
    }

    size_t n = 0;
    for (size_t i = 0; i < count; i++) {
        if (picked[i])
            chosen[n++] = matches[i].concept;
    }
    return n;
}

static void choose_prompt(FILE *in, FILE *out, const myna_catalogue *cat,
                          const char *request, const myna_match *matches, size_t count)
{
    const myna_concept *chosen[MAX_RESULTS];
    size_t n = 0;

    if (count > 1) {
        char buf[64];
        fprintf(out, "Which concepts? (for example 1,3 - press Enter for all): ");
        fflush(out);
        input_status status = read_line(in, buf, sizeof buf);
        if (status == INPUT_EOF)
            return;
        if (status != INPUT_EMPTY) {
            n = status == INPUT_OK ? parse_selection(buf, matches, count, chosen) : 0;
            if (n == 0) {
                fprintf(out, "Please enter numbers from 1 to %d, separated by commas.\n",
                        (int)count);
                return;
            }
        }
    }
    if (n == 0) {
        for (; n < count; n++)
            chosen[n] = matches[n].concept;
    }
    print_prompt(out, cat, request, chosen, n, 1);
}

static void choose_details(FILE *in, FILE *out, const myna_catalogue *cat,
                           const myna_match *matches, size_t count)
{
    char buf[32];
    int choice = 0;

    if (count > 1) {
        fprintf(out, "Which concept? (1-%d): ", (int)count);
        fflush(out);
        if (read_line(in, buf, sizeof buf) != INPUT_OK)
            return;
        choice = parse_choice(buf, count);
        if (choice < 0) {
            fprintf(out, "Please enter a number from 1 to %d.\n", (int)count);
            return;
        }
    }
    print_details(out, cat, matches[choice].concept);
}

int myna_repl(const myna_catalogue *cat, FILE *in, FILE *out)
{
    char input[INPUT_MAX + 2];
    char request[INPUT_MAX + 2] = ""; /* the description behind the results */
    myna_match matches[MAX_RESULTS];
    size_t count = 0;

    fprintf(out, "\n%d concepts loaded. Type Q to quit.\n", (int)cat->count);

    for (;;) {
        input_status status;

        if (count == 0) {
            fprintf(out, "\nDescribe what you want to implement:\n> ");
            fflush(out);
            status = read_line(in, input, sizeof input);
            if (status == INPUT_EOF)
                break;
            if (status == INPUT_EMPTY) {
                fprintf(out, "Please type a short description, for example: "
                             "\"close the popup when I click outside\".\n");
                continue;
            }
            if (status == INPUT_TOO_LONG) {
                fprintf(out, "That description is too long (max %d characters). "
                             "Please shorten it.\n", INPUT_MAX);
                continue;
            }
            if (is_quit(input))
                break;
            if (is_key(input, 'S') || is_key(input, 'D') || is_key(input, 'P')) {
                fprintf(out, "First describe what you want in a few words. The S, D "
                             "and P options work once results are shown.\n");
                continue;
            }
            strcpy(request, input);
            count = run_search(out, cat, input, matches);
            continue;
        }

        fprintf(out, "\n[S] Search again  [D] Details and related concepts  "
                     "[P] Build prompt  [Q] Quit\n> ");
        fflush(out);
        status = read_line(in, input, sizeof input);
        if (status == INPUT_EOF)
            break;
        if (status != INPUT_OK)
            continue;

        if (is_quit(input)) {
            break;
        } else if (is_key(input, 'S')) {
            count = 0;
        } else if (is_key(input, 'D')) {
            choose_details(in, out, cat, matches, count);
        } else if (is_key(input, 'P')) {
            choose_prompt(in, out, cat, request, matches, count);
        } else if (isdigit((unsigned char)input[0])) {
            int choice = parse_choice(input, count);
            if (choice < 0)
                fprintf(out, "Please enter a number from 1 to %d.\n", (int)count);
            else
                print_details(out, cat, matches[choice].concept);
        } else if (input[1] != '\0') {
            /* A longer reply is treated as a new description. */
            strcpy(request, input);
            count = run_search(out, cat, input, matches);
        } else {
            fprintf(out, "Please choose S, D, P or Q.\n");
        }
    }

    fprintf(out, "\nGoodbye!\n");
    return 0;
}

static int file_exists(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return 0;
    fclose(fp);
    return 1;
}

/* MYNA_DATA wins, then data/ beside the executable's folder, then ./data.
 * The executable's own data comes before ./data because AI tools start
 * `myna mcp` inside the user's project, which may have a data/ folder of its
 * own. The real executable path is used when known, because argv[0] is just
 * "myna" when it is started from PATH. */
static const char *find_catalogue(const char *prog, char *buf, size_t size)
{
    const char *env = getenv("MYNA_DATA");
    if (env && *env)
        return env;

    char exe[PATH_MAX_LEN];
    if (myna_executable_path(exe, sizeof exe))
        prog = exe;

    const char *sep = strrchr(prog, '/');
    const char *bsep = strrchr(prog, '\\');
    if (!sep || (bsep && bsep > sep))
        sep = bsep;
    if (sep) {
        snprintf(buf, size, "%.*s/../%s", (int)(sep - prog), prog, DEFAULT_CATALOGUE);
        if (file_exists(buf))
            return buf;
    }
    return DEFAULT_CATALOGUE; /* ./data, relative to the current folder */
}

/* `myna review <id>...`: print the review rubric for the given concepts. */
static int run_review(const myna_catalogue *cat, char **ids, int n)
{
    const myna_concept *chosen[MAX_REVIEW_IDS];
    size_t count = 0;

    if (n > MAX_REVIEW_IDS) {
        fprintf(stderr, "Too many concepts (max %d)\n", MAX_REVIEW_IDS);
        return 2;
    }
    for (int k = 0; k < n; k++) {
        const myna_concept *c = myna_find_by_id(cat, ids[k]);
        if (!c) {
            fprintf(stderr, "No concept with id \"%s\". Run \"myna list\" to see "
                            "every id.\n", ids[k]);
            return 1;
        }
        chosen[count++] = c;
    }

    size_t len = myna_build_review(chosen, count, NULL, 0);
    char *text = malloc(len + 1);
    if (!text) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    myna_build_review(chosen, count, text, len + 1);
    fputs(text, stdout);
    free(text);
    return 0;
}

static int join_args(char **args, int n, char *buf, size_t size)
{
    size_t len = 0;

    buf[0] = '\0';
    for (int i = 0; i < n; i++) {
        size_t wlen = strlen(args[i]);
        if (len + wlen + 2 > size)
            return -1;
        if (i > 0)
            buf[len++] = ' ';
        memcpy(buf + len, args[i], wlen + 1);
        len += wlen;
    }
    return 0;
}

int myna_cli_run(int argc, char **argv)
{
    const char *prog = argc > 0 ? argv[0] : "myna";
    const char *data_path = NULL;
    char path_buf[PATH_MAX_LEN];
    int i = 1;

    for (; i < argc && argv[i][0] == '-'; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(prog);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("myna %s\n", MYNA_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            data_path = argv[++i];
            continue;
        }
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        print_usage(prog);
        return 2;
    }

    const char *command = i < argc ? argv[i++] : NULL;
    int is_search = command && strcmp(command, "search") == 0;
    int is_prompt = command && strcmp(command, "prompt") == 0;
    int is_review = command && strcmp(command, "review") == 0;
    int is_show = command && strcmp(command, "show") == 0;
    int is_list = command && strcmp(command, "list") == 0;
    int is_mcp = command && strcmp(command, "mcp") == 0;

    if (command && !is_search && !is_prompt && !is_review && !is_show && !is_list &&
        !is_mcp) {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_usage(prog);
        return 2;
    }
    if ((is_search || is_prompt || is_review || is_show) && i >= argc) {
        print_usage(prog);
        return 2;
    }

    if (!data_path)
        data_path = find_catalogue(prog, path_buf, sizeof path_buf);

    myna_catalogue cat;
    myna_catalogue_init(&cat);
    if (myna_catalogue_load_csv(&cat, data_path) < 0) {
        fprintf(stderr, "Could not read catalogue: %s\n"
                        "Run Myna from the project folder, or pass -d path/to/concepts.csv\n",
                data_path);
        return 1;
    }
    if (cat.skipped > 0)
        fprintf(stderr, "Warning: skipped %d malformed line(s) in %s. "
                        "Run scripts/check_catalogue.py for details.\n",
                (int)cat.skipped, data_path);

    int status = 0;

    if (is_mcp) {
        status = myna_mcp_serve(&cat, stdin, stdout);
    } else if (is_review) {
        status = run_review(&cat, &argv[i], argc - i);
    } else if (is_list) {
        print_list(stdout, &cat);
    } else if (is_show) {
        const myna_concept *c = myna_find_by_id(&cat, argv[i]);
        if (c) {
            print_details(stdout, &cat, c);
        } else {
            fprintf(stderr, "No concept with id \"%s\"\n", argv[i]);
            status = 1;
        }
    } else if (is_search) {
        char query[INPUT_MAX + 1];
        myna_match matches[MAX_RESULTS];
        if (join_args(&argv[i], argc - i, query, sizeof query) != 0) {
            fprintf(stderr, "Description too long (max %d characters)\n", INPUT_MAX);
            status = 2;
        } else if (run_search(stdout, &cat, query, matches) == 0) {
            status = 1;
        }
    } else if (is_prompt) {
        char query[INPUT_MAX + 1];
        myna_match matches[MAX_RESULTS];
        const myna_concept *chosen[MAX_RESULTS];
        size_t n;
        if (join_args(&argv[i], argc - i, query, sizeof query) != 0) {
            fprintf(stderr, "Description too long (max %d characters)\n", INPUT_MAX);
            status = 2;
        } else if ((n = myna_search(&cat, query, matches, MAX_RESULTS)) == 0) {
            fprintf(stderr, "No matching concepts found, so there is no prompt to build.\n");
            status = 1;
        } else {
            for (size_t k = 0; k < n; k++)
                chosen[k] = matches[k].concept;
            if (print_prompt(stdout, &cat, query, chosen, n, 0) != 0)
                status = 1;
        }
    } else {
        int colour = myna_terminal_init();
        if (colour)
            myna_terminal_set_title("Myna");
        print_banner(stdout, colour);
        status = myna_repl(&cat, stdin, stdout);
    }

    myna_catalogue_free(&cat);
    return status;
}
