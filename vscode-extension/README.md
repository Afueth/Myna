# Myna for VS Code

**See how your AI's code did against expert checklists, right next to your files.**

When an AI coding assistant builds a feature with [Myna](https://github.com/Afueth/Myna)
connected, it checks its own code against Myna's checklists and reports the
result. This extension shows that review:

- **In its own sidebar**: click the Myna bird in the activity bar. Myna tells
  you how the review went, such as *"Settings page: 28 of 30 checks passed, 2
  to fix."*
- **On the lines**: every failed item is a warning on the line it is about,
  and in the **Problems** panel.
- **In the status bar**: the latest score, at a glance.

Click a failed item to jump to its line. Two buttons turn the review into a
message for your AI:

- **Fix with Claude Code**: the failed items, and the instruction to review
  again with Myna afterwards, so the panel shows the new result.
- **Review again with Claude Code**: asks your AI to check the current code
  against the same checklists and save the new result.

With [Claude Code](https://marketplace.visualstudio.com/items?itemName=anthropic.claude-code)
installed, each button asks where the message should go: one of this
project's recent Claude Code chats, shown by title (such as the chat that
just built the feature), or a new chat. The message is filled in; press Enter
to send it. Without Claude Code, the buttons copy the message so you can paste
it into any AI chat.

## Settings

| Setting | Values |
|---|---|
| `myna.claudeCodeChat` | `ask` (default): choose each time. `latest`: always continue the most recent chat of the project. `new`: always start a new chat. |

Myna finds a project's recent chats in Claude Code's own folder
(`~/.claude/projects`). If it can't read them, it starts a new chat.

## How it works

1. Connect Myna to your AI coding assistant: see
   [Using Myna in AI tools](https://github.com/Afueth/Myna/blob/main/docs/MCP.md).
2. Ask the assistant to build something. It uses Myna's checklist, reviews
   its code, and calls Myna's `report_review` tool.
3. Myna saves the review as `.myna/review.json` in your project. This
   extension notices the file and shows it.

The extension only reads `.myna/review.json` files in your workspace. It
needs no account, sends nothing anywhere, and works without an AI model of
its own.

## Commands

| Command | What it does |
|---|---|
| Myna: Show Review | Opens the Myna panel |
| Myna: Fix with Claude Code | Opens a Claude Code chat with the fix request filled in |
| Myna: Review Again with Claude Code | Opens a Claude Code chat asking it to check the code again |
| Myna: Copy Fix Request for Your AI | Copies the failed items, and the instruction to review again, as a message for your AI chat |
| Myna: Copy Review-Again Request for Your AI | Copies a message asking your AI to check the code again with the same checklists |
| Myna: Refresh Review | Reads the review file again |

## Requirements

VS Code 1.90 or later, and Myna 0.2.1 or later connected to your AI assistant
(earlier versions do not have `report_review`).

## License

MIT
