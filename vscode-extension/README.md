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

Click a failed item to jump to its line, or use **Copy fix request for your
AI** to paste the list of fixes into your AI chat.

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
| Myna: Copy Fix Request for Your AI | Copies the failed items as a message for your AI chat |
| Myna: Refresh Review | Reads the review file again |

## Requirements

VS Code 1.90 or later, and Myna 0.2.1 or later connected to your AI assistant
(earlier versions do not have `report_review`).

## License

MIT
