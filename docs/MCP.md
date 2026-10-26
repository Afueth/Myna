# Using Myna inside AI coding tools (MCP)

Myna can run as an [MCP](https://modelcontextprotocol.io) server, so AI
coding assistants such as Claude Code, Cursor and GitHub Copilot use its
checklists by themselves:

1. **Before** writing code, the assistant asks Myna what a good version of
   the feature needs.
2. **After** writing code, it asks Myna for a review checklist and checks its
   own work against every item.

You don't have to copy prompts or remember to open Myna.

## Setup

Install Myna first (see the [README](../README.md#download-and-install)). With
the Windows installer, tick **Add myna to PATH**. Otherwise, use the full path
to the program, such as `C:\Tools\myna\bin\myna.exe` or
`/home/you/myna/bin/myna`, wherever the examples below say `myna`.

Check that it works:

```console
$ myna --version
myna 0.2.0
```

### Claude Code

```console
$ claude mcp add myna -- myna mcp
```

Run `/mcp` inside Claude Code to check that Myna is connected.

### Cursor

Add Myna to `~/.cursor/mcp.json` (all projects) or `.cursor/mcp.json` (one
project):

```json
{
  "mcpServers": {
    "myna": { "command": "myna", "args": ["mcp"] }
  }
}
```

### VS Code (GitHub Copilot)

Add Myna to `.vscode/mcp.json` in your project:

```json
{
  "servers": {
    "myna": { "type": "stdio", "command": "myna", "args": ["mcp"] }
  }
}
```

### Claude Desktop and other tools

Most tools take the same three things: a name (`myna`), a command (`myna`),
and arguments (`mcp`). For Claude Desktop, add the Cursor-style
`mcpServers` entry above to `claude_desktop_config.json`.

## What the assistant can use

| Tool | When | What Myna returns |
|---|---|---|
| `find_concepts` | You ask for a feature in plain words | The matching concepts with ids, explanations, and related concepts to consider |
| `get_checklist` | Before writing code | The requirements and "done when" checks for the chosen concepts |
| `review_checklist` | After writing code | A numbered rubric to mark PASS, FAIL or NOT APPLICABLE against the code |

All three only read Myna's catalogue. They never change files, and they make
no network requests.

To nudge an assistant into using Myna, you can say so in your request, for
example: *"Use Myna's checklist to build a dropdown menu, then review it with
Myna."*

## Without MCP

The same checklists work in any AI chat:

```console
$ myna prompt "close the popup when I click outside"    # before: a brief to paste
$ myna review modal-dialog outside-click-dismissal      # after: a review rubric
```

Paste the review rubric into the chat together with your code.

## Technical details

- Transport: stdio, one JSON-RPC message per line.
- Protocol versions: `2026-07-28` (per-request `_meta`, with
  `server/discover`) and, after an `initialize` handshake, `2025-11-25`,
  `2025-06-18`, `2025-03-26` and `2024-11-05`.
- Myna reads its catalogue from `MYNA_DATA` if set, otherwise from `data/`
  next to the program. Set `MYNA_DATA` in the tool's MCP config to use your
  own catalogue.
- Messages go to stdout only; warnings go to stderr.
