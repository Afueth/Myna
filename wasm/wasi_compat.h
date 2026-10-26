/* Included before every source file in the WebAssembly build.
 *
 * WASI cannot start processes, so the Linux clipboard helper's system() and
 * popen() calls fail cleanly and Myna prints the prompt instead of copying it.
 * stdio.h is included here before terminal.c defines _POSIX_C_SOURCE, which
 * hides fileno(), so it is declared again. */
#include <stdio.h>

static inline int system(const char *command)
{
    (void)command;
    return -1;
}

static inline FILE *popen(const char *command, const char *mode)
{
    (void)command;
    (void)mode;
    return NULL;
}

static inline int pclose(FILE *stream)
{
    (void)stream;
    return -1;
}

int fileno(FILE *stream);
