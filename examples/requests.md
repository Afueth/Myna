# Example requests

All examples use the bundled `data/concepts.csv`. On Windows, run the program
as `.\build\myna.exe`.

## Interactive session

Run `myna` with no arguments:

```console
$ myna
MYNA - Developer Intent Engine
==============================
187 concepts loaded. Type Q to quit.

Describe what you want to implement:
> Release a feature to only 10% of users

Possible concepts:

  1. Feature flags
     Switches in code or configuration that turn a feature on or off without
     deploying new code.
     Matched: "release a feature"

  2. Percentage rollout
     Enabling a feature for a chosen share of users, such as 10%, and
     increasing that share over time.
     Matched: "% of users"

  3. Feature targeting
     ...
  4. Gradual deployment
     ...

Also consider: A/B testing, Canary release, Authorization and roles

[S] Search again  [D] Details and related concepts  [P] Build prompt  [Q] Quit
>
```

At the menu you can:

- type `D` (or a result number such as `2`) to see a concept's synonyms and
  related concepts,
- type `S` to describe something new, or just type the new description,
- type `Q` to quit.

## The popup example

```console
$ myna search "Make this popup modern, blur the background, animate it, and close it when I click outside."
  1. Outside-click dismissal     Matched: "click outside", "close it when i click"
  2. Backdrop blur / overlay     Matched: "blur the background"
  3. Modal dialog                Matched: "popup"
  4. Entry and exit transitions  Matched: "animate"

Also consider: Escape-key handling, Focus management, Reduced motion
```

(Output shortened; each result also shows its explanation.)

## More to try

```console
$ myna search store passwords safely
$ myna search the list should load more when I reach the bottom
$ myna search only admins can see this page
$ myna search my site is hard to read on a phone
```

## Building a prompt

After a search, type `P` to turn the results into a prompt for an AI
assistant. With several results, Myna asks which to use: type numbers such as
`1,3`, or press Enter for all of them. In a terminal, the prompt is also
copied to your clipboard.

The one-shot `prompt` command uses all results and prints the prompt, so it
can be piped:

```console
$ myna prompt "show a small message in the corner that says saved"
My request: show a small message in the corner that says saved

Build it using these techniques:
- Toast notification: A small, temporary message that appears briefly to confirm an action or report a problem without interrupting the user.
...

Requirements:
- Show it in a corner without covering important content
- Hide it after about 4 to 6 seconds, pausing while hovered or focused
- Announce it to screen readers with role=status or aria-live
- Let users dismiss it, and stack several toasts neatly
...
```

## Other commands

```console
$ myna list                 # every concept id and name
$ myna show escape-key      # one concept in full
$ myna -d my-concepts.csv   # use a different catalogue
```

## Exit codes

| Code | Meaning                                       |
|------|-----------------------------------------------|
| 0    | Success                                       |
| 1    | No results, or the catalogue could not be read |
| 2    | Invalid usage                                 |
