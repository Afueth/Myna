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

## Catalogue growth

The catalogue grows from 187 concepts to about 2,900, in categories that each
get their own file under `data/concepts/`. A concept lives in exactly one
category, chosen by the question the user is asking, and links to concepts in
other categories through `related`. Each batch is built from the sources in
`data/SOURCES.md`, de-duplicated against the whole catalogue, and comes with
test requests in `tests/requests.csv`.

Language syntax, computer science theory and framework-specific APIs are out
of scope: there is nothing to build, so there is no checklist, and their
names are easy to search for already. Framework mappings come in 0.5.

### Categories

| Category | Covers | Now | Target |
|----------|--------|-----|--------|
| `ui` | Components and interactions: dialogs, menus, tabs, cards, sliders, toasts | 61 | 300 |
| `forms` | Inputs and form behaviour: validation, dates, addresses, passwords, uploads | 15 | 100 |
| `pages` | Page types: landing hero, footer, 404, error, confirmation, check answers | 10 | 50 |
| `accessibility` | Focus, keyboard, contrast, ARIA, screen readers, captions | 9 | 50 |
| `web` | Browser side: performance, SEO, offline, storage, internationalization | 11 | 100 |
| `mobile` | iOS and Android: gestures, push, deep links, in-app purchases, app security | 8 | 200 |
| `product` | Common app features: payments, comments, chat, search, cart, onboarding, admin | 11 | 350 |
| `ai` | LLM features: chatbots, RAG, streaming, tool use, guardrails, evals | 1 | 100 |
| `backend` | Inside one app: jobs, databases, transactions, caching, retries, audit logs | 12 | 150 |
| `architecture` | System design: load balancing, sharding, queues, microservices, circuit breakers | 2 | 175 |
| `api` | REST, GraphQL, webhooks, versioning, errors, rate limits | 14 | 150 |
| `security` | Authentication, sessions, passkeys, encryption, injection, privacy | 20 | 200 |
| `devops` | Shipping and running: CI/CD, feature flags, canary releases, rollback, monitoring | 13 | 150 |
| `workflow` | Working on the code: Git, pull requests, code review, versioning, linting | 0 | 70 |
| `algorithms` | Problems people describe in plain words: shortest route, ranking, matching, scheduling, deduplication | 0 | 150 |
| `data-engineering` | Pipelines, ETL, data warehouses, change data capture, data quality | 0 | 120 |
| `games` | Game loop, collision detection, pathfinding, save games, multiplayer sync | 0 | 150 |
| `desktop` | Installers, auto-update, system tray, file associations, native menus | 0 | 80 |
| `machine-learning` | Training and serving models: data splits, overfitting, feature scaling, model monitoring | 0 | 120 |
| `embedded` | Embedded and IoT: sensors, firmware updates over the air, low power, MQTT | 0 | 100 |
| `media` | Video transcoding, image processing, audio and video streaming, thumbnails | 0 | 80 |
| **Total** | | **187** | **about 2,900** |

### Steps

- [x] Better matching for a large catalogue: stricter word variants, filler
      words inside phrases, rare words weighted higher, phrases whose words
      are apart, and a minimum score
- [x] Fail the catalogue check when unrelated concepts share a term
- [x] First UI batch from component.gallery, GOV.UK and USWDS (42 concepts)
- [x] Split the catalogue into one file per category, merged at load time
- [ ] Add `platforms` and `sources` columns, and raise the trigger limit
- [ ] Build a de-duplicated inventory of candidate concepts for each category
- [ ] Batches for every category

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
- [x] `report_review` MCP tool: the assistant reports its verdicts, and Myna
      saves them in the project as `.myna/review.json` for editors to show
- [ ] Measure whether AI-written code passes more checklist items with Myna
      connected than without (round 1 done, see `docs/COMPARISON.md`)
- [x] VS Code extension that shows saved reviews, with Myna in the sidebar
      (`vscode-extension/`, not yet on the Marketplace)
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

- JSON output (`--json`) for scripting and editor integrations
- Prebuilt index for large catalogues
