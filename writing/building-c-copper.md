---
slug: building-c-copper
title: Building c-copper: what a web framework in C teaches you
description: Why I wrote the framework serving this page in C99 with zero dependencies, the decisions that mattered, and what the numbers say.
date: 2026-07-25
tag: SYSTEMS
tags: C99, MVC, pthreads, wasmoon
---

The page you're reading was routed, rendered, and delivered by roughly 1,600 lines of C99 I wrote myself. No nginx in front doing the real work, no framework underneath, no runtime dependencies at all — `accept()` to template interpolation, every byte assembled by code in one repo.

This is not because C is the right tool for a portfolio site. It's because after seven years of shipping on .NET and Angular — where the socket loop, the router, and the view engine are all somebody else's excellent work — I wanted to know exactly what lives under the abstractions I use every day. The honest way to find out is to build the whole lifecycle yourself and then run it in production.

## The shape of the thing

c-copper is a small MVC framework: a pthread-per-request socket loop, a linked-list middleware chain (logging → static files → rate limiting → CSRF → router), a route table with `{param}` binding matched via `strtok_r`, and a handlebars-style template engine with a two-pass layout render. Controllers are plain C functions behind an `ACTION` macro; views are HTML files with `{{key}}`, `#if`, and `#each`.

The browser side is its own experiment: pages that need interactivity declare a Lua module, and a wasmoon-compiled Lua VM boots in the browser, fetches only that module, and drives the DOM through signals and hyperscript. No React, no virtual DOM — about 180 lines of Lua.

## Decisions worth defending

**Thread-per-request over an event loop.** Everyone knows the event loop scales better. But an event loop turns every handler into a state machine, and the entire point of this project was legibility — being able to hold the whole request lifecycle in one head. The measured ceiling (below) says the simple model is nowhere near being this site's bottleneck.

**A template engine with three directives.** Substitution, conditionals, loops. No partials, no filters, no expression language. Every feature a template engine grows is a parser bug you haven't found yet; three directives were enough to build every page on this site.

**Build-time everything.** Tailwind compiles to a static 40KB file at build time. These articles are markdown compiled to HTML at build time. The server's job stays brutally simple: match a route, fill a template, write bytes.

## What the numbers say

Measured with `hey` at 50 concurrent connections on an Intel N100 under WSL2 — deliberately unimpressive hardware:

- ~1,900 requests/second for fully-rendered dynamic pages
- ~3,700 requests/second for static files
- p50 latency 23ms, p99 65ms under that load

A thread-per-request C server on a four-core mini-PC chip comfortably outruns anything a portfolio will ever see. That's the quiet lesson: most of the scaling machinery we reach for by default is solving problems most systems don't have yet.

## What it actually taught me

The framework took longer than using Next.js would have. Every hour of that gap was the point. Writing the HTTP parser makes you better at debugging anyone's HTTP stack. Writing the template engine makes you understand why injection vulnerabilities happen — my `{{key}}` doesn't escape HTML, which is exactly why request data never touches it and slugs are matched against compile-time allowlists. Writing the thread dispatch makes connection-pool tuning in .NET feel less like folklore.

The day job is .NET, Angular, and applied AI for banking — regulated systems where the framework choices are conservative and correct. c-copper is the proof that when those abstractions leak, I can read the water line.

Source is on [GitHub](https://github.com/aag2807/copper-portfolio), and the full case study with architecture diagrams lives [here](/projects/c-copper).
