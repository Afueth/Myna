#ifndef MYNA_MCP_H
#define MYNA_MCP_H

#include <stdio.h>

#include "myna/catalogue.h"

/*
 * A Model Context Protocol server over stdio, so AI coding tools can ask Myna
 * for concepts, checklists and review rubrics. It serves both protocol eras:
 * legacy clients that open with `initialize` (2025-11-25 and earlier) and
 * modern clients that send per-request `_meta` (2026-07-28).
 */

typedef struct {
    int legacy; /* an `initialize` handshake selected legacy semantics */
} myna_mcp_state;

void myna_mcp_init(myna_mcp_state *st);

/*
 * Handle one JSON-RPC message. Returns the response as a malloc'd single
 * line without a trailing newline, or NULL when no response is due
 * (notifications). The caller frees the result.
 */
char *myna_mcp_handle(myna_mcp_state *st, const myna_catalogue *cat, const char *line);

/* Serve newline-delimited messages from `in` until end of input. */
int myna_mcp_serve(const myna_catalogue *cat, FILE *in, FILE *out);

#endif /* MYNA_MCP_H */
