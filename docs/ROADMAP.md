# Roadmap

Each milestone pairs a product capability with the C and engineering skills
it exercises. Details may change as the prototype is tested with real
developers.

| Milestone | Capability | Focus | Status |
|-----------|------------|-------|--------|
| **0.1 — Vocabulary Search** | Describe a change in plain language and see matching concepts with explanations. | C basics, functions, strings, arrays, input validation | Done |
| **0.2 — Prompt Builder and MCP** | Turn selected concepts into a precise AI prompt with requirements and "done when" checks, let AI coding tools fetch checklists and review rubrics over MCP, and show reviews in the editor. | Templates, string building, JSON parsing, JSON-RPC protocols, platform APIs (clipboard, files) | Mostly done |
| **0.3 — Relevance Ranking** (next) | Better scoring so results are small and relevant, with a catalogue of thousands of concepts. | Search algorithms, scoring, sorting, data structures | Started |
| 0.4 — Related Concepts | Richer relationships: synonyms, prerequisites and conflicts. | Structs, pointers, dynamic memory, dataset relationships | Started |
| 0.5 — Framework Awareness | Explain how concepts map to React, CSS, FastAPI and other stacks. | Framework-specific terminology and curated mappings | Not started |
| 0.6 — Project Awareness | Tailor suggestions to the current project's files and dependencies, with consent. | Filesystem traversal, dependency inspection, safe project analysis | First step |
| 0.7 — Local API | Expose the engine over a local HTTP API. | HTTP fundamentals, networking, API boundaries | Not started |
| 1.0 — Integrations | Editor integrations and packaged releases. | Editor workflows, packaging, documentation, releases, contributions | Partly done |

## Catalogue growth

The catalogue grows from 1,198 concepts to about 2,900, in categories that each
get their own file under `data/concepts/`. A concept lives in exactly one
category, chosen by the question the user is asking, and links to concepts in
other categories through `related`. Each batch is built from the sources in
`data/SOURCES.md`, de-duplicated against the whole catalogue, and comes with
test requests in `tests/requests.csv`.

Language syntax, computer science theory and framework-specific APIs are out
of scope: there is nothing to build, so there is no checklist, and their
names are easy to search for already. Framework mappings come in 0.5.

Today: 1,198 concepts with 11,153 checklist items (every concept has a
checklist), in 21 categories, about 41% of the target.

### Categories

| Category | Covers | Now | Target |
|----------|--------|-----|--------|
| `ui` | Components and interactions: dialogs, menus, tabs, cards, sliders, toasts | 95 | 300 |
| `forms` | Inputs and form behaviour: validation, dates, addresses, passwords, uploads | 36 | 100 |
| `pages` | Page types: landing hero, footer, 404, error, confirmation, check answers | 31 | 50 |
| `accessibility` | Focus, keyboard, contrast, ARIA, screen readers, captions | 30 | 50 |
| `web` | Browser side: performance, SEO, offline, storage, internationalization | 58 | 100 |
| `mobile` | iOS and Android: gestures, push, deep links, in-app purchases, app security | 80 | 200 |
| `product` | Common app features: payments, comments, chat, search, cart, onboarding, admin | 120 | 350 |
| `ai` | LLM features: chatbots, RAG, streaming, tool use, guardrails, evals | 50 | 100 |
| `backend` | Inside one app: jobs, databases, transactions, caching, retries, audit logs | 38 | 150 |
| `architecture` | System design: load balancing, sharding, queues, microservices, circuit breakers | 37 | 175 |
| `api` | REST, GraphQL, webhooks, versioning, errors, rate limits | 43 | 150 |
| `security` | Authentication, sessions, passkeys, encryption, injection, privacy | 85 | 200 |
| `devops` | Shipping and running: CI/CD, feature flags, canary releases, rollback, monitoring | 58 | 150 |
| `workflow` | Working on the code: Git, pull requests, code review, versioning, linting | 35 | 70 |
| `algorithms` | Problems people describe in plain words: shortest route, ranking, matching, scheduling, deduplication | 54 | 150 |
| `data-engineering` | Pipelines, ETL, data warehouses, change data capture, data quality | 46 | 120 |
| `games` | Game loop, collision detection, pathfinding, save games, multiplayer sync | 97 | 150 |
| `desktop` | Installers, auto-update, system tray, file associations, native menus | 33 | 80 |
| `machine-learning` | Training and serving models: data splits, overfitting, feature scaling, model monitoring | 78 | 120 |
| `embedded` | Embedded and IoT: sensors, firmware updates over the air, low power, MQTT | 61 | 100 |
| `media` | Video transcoding, image processing, audio and video streaming, thumbnails | 33 | 80 |
| **Total** | | **1,198** | **about 2,900** |

### Steps

- [x] Better matching for a large catalogue: stricter word variants, filler
      words inside phrases, rare words weighted higher, phrases whose words
      are apart, and a minimum score
- [x] Fail the catalogue check when unrelated concepts share a term
- [x] First UI batch from component.gallery, GOV.UK and USWDS (42 concepts)
- [x] Split the catalogue into one file per category, merged at load time
- [x] Record the source each new concept is built on (`data/concept_sources.csv`)
- [x] Build a de-duplicated inventory of candidate concepts for each category
- [x] First full round for every category: 1,011 concepts written and reviewed
- [x] License the data under CC BY 4.0, with a contributor licence agreement
- [ ] Add `platforms` and `sources` columns, and raise the trigger limit
- [ ] Find and merge near-duplicate concepts, such as `status-indicator` and
      `presence-indicators`
- [ ] Batches for every category, up to the targets above
- [ ] Give every checklist item an id, a must or should level and the source
      it comes from (`data/checklists.csv`)

## 0.3 — Relevance Ranking (next)

With 1,198 concepts, search quality dropped: on the 1,258 test requests in
`tests/requests.csv`, 33% get the expected concept first and 44% don't get it
in the top five. Three causes: unrelated concepts matching on single rare
words, near-duplicate concepts splitting the results, and test expectations
that accept only one correct answer.

- [x] Weighted matching: rare words count for more, filler words may sit
      inside phrases, and only real word endings make variants (so "check" no
      longer matches "checkbox", nor "table" "tablet")
- [ ] Stop unrelated results: a result must cover the main idea of the
      request, and results far below the best one are dropped
- [ ] Merge near-duplicate concepts (see Catalogue growth)
- [ ] Let test requests accept several correct answers where that is right
- [ ] A batch mode so `scripts/coverage.py` loads the catalogue once and runs
      in seconds instead of minutes
- [ ] Get back above 85% of test requests with the expected concept first

## 0.2 — Prompt Builder and MCP

- [x] Optional `requirements` and `done_when` columns in the catalogue
- [x] Core prompt builder: request, concepts, merged checklists without
      duplicates, related concepts
- [x] `[P]` in the interactive session, with a choice of concepts and copying
      to the clipboard
- [x] One-shot `myna prompt <description>` for scripts and pipes
- [x] Checklists for every concept, based on WCAG, the W3C ARIA Authoring
      Practices and the OWASP Cheat Sheets (see `data/SOURCES.md`)
- [x] Full coverage of all 30 APG patterns, all 55 WCAG 2.2 A/AA criteria, the
      88 relevant OWASP cheat sheets, all 24 MASVS controls, Core Web Vitals,
      NIST SP 800-63B, the API guidelines, component.gallery, GOV.UK and USWDS,
      tracked in `data/source_coverage.csv` and checked by
      `scripts/check_catalogue.py`
- [x] Independent reviews of the checklists against their sources, with every
      finding fixed
- [x] Review rubric (`myna review`) for checking finished code against a checklist
- [x] MCP server (`myna mcp`) with `find_concepts`, `get_checklist` and
      `review_checklist`, for both the 2026-07-28 protocol and earlier
      `initialize`-based versions
- [x] `list_concepts` MCP tool to browse categories, and `keywords` in
      `find_concepts` for technical names the user didn't say
- [x] `report_review` MCP tool: the assistant reports its verdicts, and Myna
      saves them in the project as `.myna/review.json`, with a history in
      `.myna/history.jsonl`
- [x] Reviews choose concepts from what the code contains, not only from the
      request, and add up to four closely related concepts (`include_related`)
- [x] Chosen and added concepts are reported apart (`concept_ids`,
      `related_ids`), so reviewing again gives a review of the same size
- [x] VS Code extension that shows saved reviews: Myna in its own sidebar,
      warnings on the reviewed lines and in the Problems panel, the score in
      the status bar (`vscode-extension/`)
- [x] Extension: **Fix with Claude Code** and **Review again with Claude
      Code** fill in a Claude Code chat, an existing chat of the project or a
      new one (`myna.claudeCodeChat`); other AI tools get the message copied
- [x] Blind comparison test, round 1: 30/30 with Myna against 25/30 without
      (see `docs/COMPARISON.md`)
- [ ] More comparison rounds (round 2, a sign-up form, is prepared) before
      making public claims
- [ ] Try the prompts with real AI assistants and refine the wording
- [ ] Publish the extension on the VS Code Marketplace: publisher account,
      screenshots in its README
- [ ] Extension: fill in GitHub Copilot's chat too, and warn when the
      installed Myna is too old to save reviews

## 0.4 — Related Concepts

- [x] `related` links between concepts, shown as "Also consider"
- [x] The most closely related concepts, ranked by how many chosen concepts
      point to them, are added to reviews
- [ ] Synonym, prerequisite and conflict relationships

## 0.5 — Framework Awareness

- [ ] How each concept maps to React, Vue, CSS, FastAPI, Django and other
      stacks

## 0.6 — Project Awareness

- [x] First step: Myna's instructions ask the AI assistant to choose concepts
      from the features the code actually contains
- [ ] Myna detects features in the project's files itself, with consent

## 0.7 — Local API

- [ ] Expose the engine over a local HTTP API (MCP already covers AI tools)

## 1.0 — Integrations

- [x] Releases for Windows, macOS and Linux, built and tested by CI
- [x] Windows installer: Start menu, optional PATH, uninstall entry, and it
      ends running Myna servers before updating
- [x] MCP server, so every MCP-capable AI tool can use Myna
- [x] VS Code extension (see 0.2)
- [x] WebAssembly build that runs without a C compiler (`wasm/`)
- [ ] Extension on the VS Code Marketplace
- [ ] Packages: winget, Homebrew, npm
- [ ] Contributor process: good first issues, a guide to adding a concept

## 0.1 — Vocabulary Search

- [x] Interactive terminal session (`[S]` search again, `[D]` details, `[Q]` quit)
- [x] One-shot `search`, `show` and `list` commands
- [x] Catalogue with explanations, synonyms, trigger phrases and related concepts
- [x] Phrase matching with simple scoring and "Also consider" suggestions
- [x] Safe handling of empty, overlong and unrecognised input
- [x] Tests for parsing, matching and edge cases; catalogue validation in CI
- [ ] Grow the dataset with feedback from real requests

## Ideas for later

- JSON output (`--json`) for scripting and editor integrations
- Prebuilt index for large catalogues
- Reviews that check code without an AI assistant, using simple rules for
  common mistakes
- A web page to search Myna without installing anything
