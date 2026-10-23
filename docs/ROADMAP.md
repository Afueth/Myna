# Roadmap

Each milestone pairs a product capability with the C and engineering skills
it exercises. Details may change as the prototype is tested with real
developers.

| Milestone | Capability | Focus |
|-----------|------------|-------|
| **0.1 — Vocabulary Search** (current) | Describe a change in plain language and see matching concepts with explanations. | C basics, functions, strings, arrays, input validation |
| 0.2 — Related Concepts | Richer relationships: synonyms, prerequisites and conflicts. | Structs, pointers, dynamic memory, dataset relationships |
| 0.3 — Relevance Ranking | Better scoring so results are small and relevant. | Search algorithms, scoring, sorting, data structures |
| 0.4 — Prompt Builder | Turn selected concepts into a precise AI prompt with behaviour, constraints and acceptance criteria. | File I/O, templates, parsing, modular design |
| 0.5 — Framework Awareness | Explain how concepts map to React, CSS, FastAPI and other stacks. | Framework-specific terminology and curated mappings |
| 0.6 — Project Awareness | Tailor suggestions to the current project's files and dependencies, with consent. | Filesystem traversal, dependency inspection, safe project analysis |
| 0.7 — Local API | Expose the engine over a local HTTP API. | HTTP fundamentals, networking, API boundaries |
| 1.0 — Integrations | Editor integrations and packaged releases. | Editor workflows, packaging, documentation, releases, contributions |

## 0.1 — Vocabulary Search

- [x] Interactive terminal session (`[S]` search again, `[D]` details, `[Q]` quit)
- [x] One-shot `search`, `show` and `list` commands
- [x] Catalogue with explanations, synonyms, trigger phrases and related concepts
- [x] Phrase matching with simple scoring and "Also consider" suggestions
- [x] Safe handling of empty, overlong and unrecognised input
- [x] Tests for parsing, matching and edge cases; catalogue validation in CI
- [ ] Grow the starter dataset with feedback from real requests

## Ideas for later

- Multiple catalogue files merged at load time
- JSON output (`--json`) for scripting and editor integrations
- Prebuilt index for large catalogues
