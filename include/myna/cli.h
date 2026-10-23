#ifndef MYNA_CLI_H
#define MYNA_CLI_H

#include <stdio.h>

#include "myna/catalogue.h"

/* Parse arguments and run the requested command. Returns a process exit code. */
int myna_cli_run(int argc, char **argv);

/* Run the interactive session, reading from `in` and writing to `out`. */
int myna_repl(const myna_catalogue *cat, FILE *in, FILE *out);

#endif /* MYNA_CLI_H */
