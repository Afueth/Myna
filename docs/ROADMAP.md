# Roadmap

Each milestone pairs a product capability with the C and engineering skills
it exercises. Details may change as the prototype is tested with real
developers.

| Milestone | Capability | Focus |
|-----------|------------|-------|
| **0.1 — Vocabulary Search** (done) | Describe a change in plain language and see matching concepts with explanations. | C basics, functions, strings, arrays, input validation |
| **0.2 — Prompt Builder and MCP** (current) | Turn selected concepts into a precise AI prompt with requirements and "done when" checks, and let AI coding tools fetch checklists and review rubrics over MCP. | Templates, string building, JSON parsing, JSON-RPC protocols, platform APIs (clipboard) |
| 0.3 — Relevance Ranking | Better scoring so results are small and relevant. | Search algorithms, scoring, sorting, data structures |
| 0.4 — Related Concepts | Richer relationships: synonyms, prerequisites and conflicts. | Structs, pointers, dynamic memory, dataset relationships |
| 0.5 — Framework Awareness | Explain how concepts map to React, CSS, FastAPI and other stacks. | Framework-specific terminology and curated mappings |
| 0.6 — Project Awareness | Tailor suggestions to the current project's files and dependencies, with consent. | Filesystem traversal, dependency inspection, safe project analysis |
| 0.7 — Local API | Expose the engine over a local HTTP API. | HTTP fundamentals, networking, API boundaries |
| 1.0 — Integrations | Editor integrations and packaged releases. | Editor workflows, packaging, documentation, releases, contributions |

## 0.2 — Prompt Builder and MCP

- [x] Optional `requirements` and `done_when` columns in the catalogue
- [x] Core prompt builder: request, concepts, merged checklists without
      duplicates, related concepts
- [x] `[P]` in the interactive session, with a choice of concepts and copying
      to the clipboard
- [x] One-shot `myna prompt <description>` for scripts and pipes
- [x] Checklists for every concept, based on WCAG, the W3C ARIA Authoring
      Practices and the OWASP Cheat Sheets (see `data/SOURCES.md`)
- [x] Full coverage of all 30 APG patterns, all 55 WCAG 2.2 A/AA criteria and
      the 88 relevant OWASP cheat sheets, tracked in `data/source_coverage.csv`
      and checked by `scripts/check_catalogue.py`
- [x] Review rubric (`myna review`) for checking finished code against a checklist
- [x] MCP server (`myna mcp`) with `find_concepts`, `get_checklist` and
      `review_checklist`, for both the 2026-07-28 protocol and earlier
      `initialize`-based versions
- [ ] Measure whether AI-written code passes more checklist items with Myna
      connected than without
- [ ] Try the prompts with real AI assistants and refine the wording

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
