# Does Myna make AI-written code better?

This page records blind tests of the same AI coding assistant building the
same feature with and without Myna connected. Results are added as rounds are
run. A single round is a signal, not proof: AI output varies from run to run.

## Method

1. **Two empty folders** with neutral names (for example `Compare-A` and
   `Compare-B`).
2. **The tester secretly chooses** which folder gets Myna, and keeps it secret
   until scoring is finished.
3. **The same AI tool and model** builds the feature in each folder, in a new
   chat each time:
   - with Myna: Myna connected over MCP (see [MCP.md](MCP.md));
   - without Myna: Myna disabled in that chat.
4. **The same prompt, word for word.** It does not mention Myna, so the test
   also shows whether the assistant uses Myna by itself.
5. **The scoring rubric is fixed before anything is built**: the output of
   `myna review` for the concepts the feature needs.
6. **Blind scoring**: two independent reviewers, one per folder, get identical
   instructions. They are not told what is being compared. They mark every
   item PASS, FAIL or NOT APPLICABLE with file and line evidence, compute
   contrast ratios instead of estimating them, and list serious problems
   outside the rubric.
7. **The tester reveals** which folder had Myna.

### Limits to keep in mind

- The rubric is Myna's own checklist, so a version built with Myna was in
  effect shown the test. That is how Myna is meant to work, but it means the
  most independent evidence is the list of problems **outside** the rubric.
- Reviewers judge by reading the code; nothing is run in a browser or with a
  screen reader.
- Each round is a single run per side.

## Round 1: settings page

**Date:** 26 October 2026, Myna 0.2.0, Claude Code.

**Prompt:**

```
Build a settings page for a web app in plain HTML, CSS and JavaScript, with no frameworks. Put all files in this folder.

It needs:
1. A switch to turn dark mode on and off. The choice should be remembered.
2. An "Account" button that opens a dropdown menu with Profile, Billing and Sign out.
3. A "Delete account" button that asks for confirmation before deleting.

Make it production quality.
```

**Rubric:** `myna review toggle-switch dark-mode dropdown-menu alert-dialog`
(30 items).

### Results

| | With Myna | Without Myna |
|---|---|---|
| Rubric score | **30 / 30** | **25 / 30** |
| Dark-mode switch (7 items) | 7 | 6 |
| Dark mode (6 items) | 6 | 6 |
| Dropdown menu (10 items) | 10 | 8 |
| Confirmation dialog (7 items) | 7 | 5 |
| Serious problems outside the rubric | None (5 minor) | 1: a mock API is on by default, so the page reports "Your account has been deleted" without deleting anything |

**What the version without Myna missed:**

- The switch's off state has a contrast of 1.87:1 against its background,
  below the 3:1 that WCAG 1.4.11 requires for controls, so it is hard to see.
- The dropdown menu has no positioning logic and can open off-screen.
- The confirmation dialog has no `role="alertdialog"`.
- Focus goes to a text field instead of the safe button when the dialog opens.

**What both versions did well:** a real `role="switch"`, a remembered theme
with no flash of the wrong one, a keyboard-operable menu with type-ahead, and
text contrast above 4.5:1 in both themes. The difference was in the details
that are easy to miss.

## Running a round yourself

1. Create two empty folders, and pick in secret which one gets Myna.
2. Fix the rubric first: `myna review <concept ids>` for the feature, saved
   to a file.
3. Build with the same prompt in a new chat in each folder, with Myna
   enabled in one and disabled in the other.
4. Score each folder blind against the rubric, then reveal.

Please share your results in an issue, whichever way they turn out.
