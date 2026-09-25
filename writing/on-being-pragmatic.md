---
slug: on-being-pragmatic
title: On being pragmatic
description: Correct and right are different things. Correct ships; right matches an idea the author had. Most of the damage in this industry lives in the gap.
date: 2026-08-24
tag: engineering
tags: pragmatism, architecture, fintech, c-copper
---

Every codebase I have regretted was written by someone who was trying to be right. Correct and right are different things, and most of the damage in this industry lives in the gap between them.

Correct means the program does what it needs to do, for the people who need it, at a cost they can afford. Right means the program matches an idea the author had about how programs should look. Correct ships. Right gets refactored by the next person, who has their own idea about how programs should look.

## What pragmatism is not

People hear pragmatic and think lazy, or "just make it work." It is the opposite. Purism is the easy option because it comes with a rulebook. Pragmatism comes with a question, which is what does this actually need, and answering it takes work.

A purist knows the answer before looking at the problem: dependency injection everywhere, every function pure, every abstraction abstracted. A pragmatist has to look first.

None of this is an argument against craft. The best engineers I have worked with cared a great deal about quality. They spent that care on the parts that mattered and let the rest be ordinary. A boring function that works is craft. A clever one that needs a comment to be understood is debt.

## The three questions

When I am about to add structure to something, I try to answer three things before typing.

**Who pays for this?** Every abstraction has a reader on the other end. A layer that saves me ten minutes today and costs every future reader five minutes of indirection is a bad trade by the second reader.

**What breaks if I am wrong?** If the answer is "I rename a function," the decision does not deserve a meeting. If the answer is "we migrate a database," it does. Rigor should be proportional to the cost of a mistake.

**Is this the problem, or the problem I would rather be solving?** This is the uncomfortable one. A lot of what gets called architecture is a developer avoiding a boring task by inventing an interesting one next to it.

## Two failures with the same cause

A banking client once needed a nightly reconciliation job. The first proposal was an event-driven pipeline with a message broker, retries, dead-letter queues, and a dashboard. The volume was four thousand rows. A single scheduled process with a transaction and a log file ran for three years without incident. Nobody ever asked for the dashboard.

I have also watched a quick script become the ingestion path for a product's entire customer base, still parsing CSV with `split(',')`, still failing silently whenever a field contained a comma. Building something small was the correct call at the time. The failure was that nobody asked the question again once the script became load-bearing.

Both cases went wrong in the same way. A decision was made once and never revisited. Pragmatism is a habit of re-deciding, and it stops working the moment you treat it as a decision.

## Why this site is written in C

This page is served by an HTTP server, a router, and a template engine I wrote from scratch in C. For a portfolio site, that is an unreasonable choice. Any static generator would have done the job in an afternoon.

The website was the excuse, though. I wanted to understand sockets, threads, and parsing from the ground up, and a site I actually had to keep online was the cheapest thing that would force me to finish. Measured against that goal, C was the pragmatic choice. Measured against "I need a portfolio," it was absurd.

Pragmatism only works if you are honest about what you are optimizing for. If you lie about the goal, every decision downstream inherits the lie.

## The short version

> Build the thing the problem needs. Build it well. Do not build the thing you wish the problem needed.

Know what you are optimizing for. Spend rigor where mistakes are expensive. Revisit decisions when the facts change. Let the rest be boring.

---

Most of what I know about this I learned by getting it wrong first. That is probably the most pragmatic thing about it.
