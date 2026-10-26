#ifndef MYNA_TERMINAL_H
#define MYNA_TERMINAL_H

#include <stddef.h>
#include <stdio.h>

/*
 * Prepare stdout for colour output. Returns 1 if stdout is an interactive
 * terminal that can show the colour logo, 0 otherwise (piped output, NO_COLOR
 * set, or a console without escape-sequence support). Console settings changed
 * here are restored when the program exits.
 */
int myna_terminal_init(void);

/* Set the terminal window or tab title. */
void myna_terminal_set_title(const char *title);

/*
 * Write the full path of the running executable to `buf`. Returns 1 on
 * success, 0 if the platform can't tell or the path doesn't fit.
 */
int myna_executable_path(char *buf, size_t size);

/* Returns 1 if `path` names an existing file or folder. */
int myna_path_exists(const char *path);

/* Returns 1 if `stream` is an interactive terminal rather than a file or pipe. */
int myna_stream_is_terminal(FILE *stream);

/*
 * Put UTF-8 `text` on the system clipboard. Uses the Windows clipboard,
 * pbcopy on macOS, and wl-copy, xclip or xsel elsewhere. Returns 1 on
 * success, 0 if no clipboard is available.
 */
int myna_copy_to_clipboard(const char *text);

#endif /* MYNA_TERMINAL_H */
