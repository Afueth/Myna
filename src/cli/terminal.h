#ifndef MYNA_TERMINAL_H
#define MYNA_TERMINAL_H

/*
 * Prepare stdout for colour output. Returns 1 if stdout is an interactive
 * terminal that can show the colour logo, 0 otherwise (piped output, NO_COLOR
 * set, or a console without escape-sequence support). Console settings changed
 * here are restored when the program exits.
 */
int myna_terminal_init(void);

/* Set the terminal window or tab title. */
void myna_terminal_set_title(const char *title);

#endif /* MYNA_TERMINAL_H */
