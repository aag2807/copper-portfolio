# Projects — Alvaro Guzman

Everything on [alvaro-guzman.com](https://alvaro-guzman.com), across all tabs.
15 entries. Statuses mirror the site: `live` / `active` / `shipped` / `ongoing` / `in_dev` / `in_prog` / `research` / `archived`.

---

## Systems · Frameworks · Languages — [/systems](https://alvaro-guzman.com/systems)

| # | Project | Stack | Status | Source |
|---|---------|-------|--------|--------|
| 00 | **c-copper** | C99 · pthreads · no deps | `live` | [github.com/aag2807/copper-portfolio](https://github.com/aag2807/copper-portfolio) |
| 01 | **Lunar** | Go · compiler · LSP · bundler | `active` | [github.com/aag2807/lunar](https://github.com/aag2807/lunar) |
| 02 | **Cobre** | C · .cob → .so transpiler · arena | `in_dev` | — |
| 03 | **Axon** | C# · LLM gateway | `archived` | — |

- **c-copper** — The hand-written C MVC framework serving the portfolio itself. Socket loop with pthread dispatch, pattern-matching router with param binding, linked-list middleware chain, handlebars-style template engine. 1,559 lines of C, 7 HTTP routes, 0 runtime dependencies.
- **Lunar** — Statically-typed superset of Lua: compiler, LSP, bundler, and test framework, all in Go. TypeScript-for-Lua, end to end. Strong types on top of Lua semantics so projects that grew past the comfortable Lua size still have a teaching tool for the next person who reads them.
- **Cobre** — Razor-pages-inspired web framework in C. `.cob` source files transpile to `.gen.c`, then compile to `.so` and hot-load. Arena allocator with a `Str` type, `goto cleanup` error handling, TOML config. The opinionated successor to c-copper.
- **Axon** — First pass at an on-premise LLM gateway in C#. Routing, request shaping, prompt template layering. Useful exercise in API surface design and provider abstraction; stays in the archive.

## AI · Applied LLM — [/ai](https://alvaro-guzman.com/ai)

All local-first: running against a self-hosted Ollama daemon (`mxbai-embed-large` embeddings, `sqlite-vec` index). Posture: local_first, no_framework_magic, small_models_first.

| # | Project | Stack | Status |
|---|---------|-------|--------|
| 01 | **The Council** | React · 4 personas + arbiter · SVG avatars | `shipped` |
| 02 | **Doc OCR Indexer** | Python · ollama · mxbai-embed-large | `active` |
| 03 | **RAG Retrieval Bot** | Python · sqlite-vec · ollama | `active` |
| 04 | **Go Coding CLI** | Go · ollama · streaming | `active` |

- **The Council** — Multi-agent deliberation app. Four Claude personas — Philosopher, Contrarian, Romanticist, Analyst — argue a question in parallel; an Arbiter persona reads the four outputs and synthesizes a single answer. Dark grimoire UI, hand-drawn SVG avatars. Built to see if forcing disagreement produces better answers than asking one model twice.
- **Doc OCR Indexer** — Document OCR & extraction pipeline. Ingests scans, runs OCR, then hands raw text to a local LLM that classifies the document type and pulls out the fields that matter — dates, parties, totals, identifiers. Output drops into a searchable store, not a wall of unstructured text.
- **RAG Retrieval Bot** — Retrieval-augmented chat, end to end. Embed a corpus, store vectors, kNN lookup at query time, stitch top chunks into the prompt. Built by wiring the moving parts by hand instead of trusting a framework — chunking, recall thresholds, and prompt budget all picked deliberately.
- **Go Coding CLI** — Terminal coding assistant in a single Go binary talking to the local Ollama daemon. Streams tokens as they arrive, keeps a session buffer, stays out of the way — no browser tab, no auth dance, no leaving the terminal to ask a small question.

## Web — [/web](https://alvaro-guzman.com/web)

| # | Project | Stack | Status | Notes |
|---|---------|-------|--------|-------|
| 00 | **wasmoon-bridge** | Lua 5.4 → WASM · hyperscript · signals | `live` | powers this site |
| 01 | **Scrum Dashboard** | Angular · .NET · SignalR · D3 · OpenRouter | `shipped` | |
| 02 | **Enterprise Web Work** | Angular · .NET · WordPress · 60+ payment APIs | `ongoing` | NDA |

- **wasmoon-bridge** — Browser-side Lua runtime, mounted per route. Pages declare a `data-lua-module` attribute; a wasmoon-compiled Lua engine boots in the browser, fetches only that module, and runs it. No virtual DOM, no React — signals + hyperscript in ~180 lines of Lua. Pages without a module ship zero wasmoon.
- **Scrum Dashboard** — Real-time sprint board with AI assist, over Jira Cloud. SignalR streams delta updates so the board reacts the instant tickets move; a D3 force graph lays out tag-association clusters across the backlog; an OpenRouter-backed chat assistant answers sprint-state questions inline.
- **Enterprise Web Work** — Fintech, banking, and multilingual WordPress at scale. Angular and .NET applications inside banking environments; 60+ payment-API integrations spanning fintech rails; enterprise WordPress with localized content, custom plugins, and legacy-system integrations. Client names withheld — walkthrough available under NDA.

## Gamedev — [/gamedev](https://alvaro-guzman.com/gamedev)

LÖVE 11.5 is the prototyping bench; productized work runs in Unity.

| # | Project | Stack | Status |
|---|---------|-------|--------|
| 00 | **Passages** | Unity · C# · LitRPG action RPG · gambit AI | `in_dev` |
| 01 | **Bevy + SDL2 Track** | Rust · Bevy · SDL2 · C++ · ECS ladder | `in_prog` |
| 02 | **BSP Renderer + Unity Shooter** | C · Unity · from-scratch BSP | `research` |

- **Passages** — LitRPG-inspired third-person action RPG. Combat blends FF12's Gambit system with the rule-driven party logic of .hack//GU. A dual-resource system (Vitality / Essence) governs both health and casting. The Attunement Scripts editor lets the player wire per-companion behavior trees; abilities are authored as ScriptableObjects so designers iterate without recompiling.
- **Bevy + SDL2 Track** — Sequential learning ladder, not a single project: SDL2/C++ for the fundamentals (Pong → Breakout → top-down shooter), then the same exercises graduated into Rust + Bevy to feel ECS-first authoring against identical problems. Active track.
- **BSP Renderer + Unity Shooter** — A BSP renderer written from scratch in C as a graphics deep-dive: tree construction, polygon splitting, painter's-order traversal. Separately, a 3D shooter prototype in Unity to feel the C# tooling end of the same pipeline. Both research artifacts, not productized.

## Tooling

| # | Project | Stack | Status |
|---|---------|-------|--------|
| 00 | **AI CLI Skills System** | bash · markdown · Angular/.NET/React SKILL.md | `active` |

- **AI CLI Skills System** — Skill definitions (SKILL.md) for AI coding CLIs covering Angular, .NET, and React workflows.

---

*Generated from the live portfolio views on 2026-07-25. The site's workshop page is the canonical index: [alvaro-guzman.com/workshop](https://alvaro-guzman.com/workshop).*
