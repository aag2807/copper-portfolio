// Build-time markdown pipeline: writing/*.md -> views/writing/*.html
// + views/writing/index.html + controllers/writing_manifest.h
// Zero runtime deps preserved: the C server only ever serves generated HTML.
import { marked } from "marked";
import { readFileSync, writeFileSync, readdirSync, mkdirSync, rmSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const SRC = join(root, "writing");
const OUT = join(root, "views", "writing");
mkdirSync(OUT, { recursive: true });

// Frontmatter keys: slug, title, description, date (required);
// tag (e.g. SYSTEMS / AI-LLM / WEB), tags (comma list), draft (true) (optional).
//
// Drafts: `draft: true` posts are skipped entirely (no view, not in the index,
// the C manifest or prev/next links) unless DRAFTS=1 is set. With DRAFTS=1 they
// are built with a visible draft eyebrow, a robots noindex meta and highlighted
// [CONFIRM: ...] review marks. A non-draft post containing "[CONFIRM" fails the
// build, so an unreviewed claim can never ship.
const INCLUDE_DRAFTS = process.env.DRAFTS === "1";
function parseFrontmatter(raw) {
  const m = raw.match(/^---\n([\s\S]*?)\n---\n([\s\S]*)$/);
  if (!m) throw new Error("missing frontmatter block");
  const meta = {};
  for (const line of m[1].split("\n")) {
    const i = line.indexOf(":");
    if (i > 0) meta[line.slice(0, i).trim()] = line.slice(i + 1).trim();
  }
  return { meta, body: m[2] };
}

// The C template engine interprets {{...}}, @csrf and @layout (it deletes the
// whole @layout line) — generated HTML must never contain them, or article
// content would be mangled at render time.
const neutralize = (html) =>
  html
    .replace(/\{\{/g, "&#123;&#123;")
    .replace(/\}\}/g, "&#125;&#125;")
    .replace(/@csrf/g, "&#64;csrf")
    .replace(/@layout/g, "&#64;layout");

// Escape for a C string literal: backslash first, then quotes and newlines.
const cString = (s) => String(s).replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(/\r?\n/g, "\\n");

const escapeHtml = (s) =>
  String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");

const slugify = (s) =>
  String(s)
    .toLowerCase()
    .replace(/<[^>]+>/g, "")
    .replace(/&[a-z#0-9]+;/g, "")
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "");

const readMinutes = (md) => Math.max(1, Math.round(md.split(/\s+/).filter(Boolean).length / 220));

// Heading ids for the in-page table of contents (marked >= 5 dropped headerIds).
function renderWithToc(body) {
  const toc = [];
  const renderer = new marked.Renderer();
  renderer.heading = function ({ text, depth, tokens }) {
    const inner = this.parser.parseInline(tokens);
    const id = slugify(text);
    if (depth === 2) toc.push({ id, text: inner });
    return `<h${depth} id="${id}">${inner}</h${depth}>\n`;
  };
  // Block/inline HTML comments are author notes (e.g. a review checklist in a
  // draft): drop them so they never reach the served page.
  renderer.html = ({ text }) => (/^\s*<!--[\s\S]*?-->\s*$/.test(text) ? "" : text);
  const html = marked.parse(body, { renderer });
  return { html, toc };
}

// Wrap [CONFIRM: ...] review markers in a visible highlight (draft builds only).
const markConfirms = (html) =>
  html.replace(
    /\[CONFIRM:[^\]]*\]/g,
    (m) => `<mark class="confirm" style="background:#fde68a;color:#1c1917;padding:0 .2em;border-radius:2px">${m}</mark>`,
  );

const posts = [];
const skippedDrafts = [];
for (const f of readdirSync(SRC).filter((f) => f.endsWith(".md")).sort()) {
  const raw = readFileSync(join(SRC, f), "utf8");
  const { meta, body } = parseFrontmatter(raw);
  const draft = meta.draft === "true";
  if (!draft && raw.includes("[CONFIRM")) {
    console.error(`${f}: contains "[CONFIRM" markers but is not a draft; resolve them or set draft: true`);
    process.exit(1);
  }
  if (draft && !INCLUDE_DRAFTS) {
    skippedDrafts.push(f);
    // Remove a view left behind by an earlier DRAFTS=1 build.
    if (/^[a-z0-9-]+$/.test(meta.slug || "")) rmSync(join(OUT, `${meta.slug}.html`), { force: true });
    continue;
  }
  for (const k of ["slug", "title", "description", "date"])
    if (!meta[k]) throw new Error(`${f}: frontmatter missing "${k}"`);
  if (!/^[a-z0-9-]+$/.test(meta.slug)) throw new Error(`${f}: slug must be [a-z0-9-]`);
  if (meta.description.length > 160)
    console.warn(`${f}: description is ${meta.description.length} chars (search snippets cut at ~160)`);
  const { html, toc } = renderWithToc(body);
  posts.push({
    ...meta,
    file: f,
    html: draft ? markConfirms(neutralize(html)) : neutralize(html),
    draft,
    toc,
    tag: meta.tag || "NOTES",
    tags: (meta.tags || "").split(",").map((t) => t.trim()).filter(Boolean),
    read: readMinutes(body),
  });
}

posts.sort((a, b) => (a.date < b.date ? 1 : -1));

// ---- post pages ----
posts.forEach((p, i) => {
  const prev = posts[i + 1]; // older
  const next = posts[i - 1]; // newer
  const tocHtml = p.toc.length
    ? `      <nav aria-label="Contents" class="panel p-5">
        <div class="label mb-3.5">CONTENTS</div>
        <div class="flex flex-col gap-2.5 text-[13.5px] leading-[1.45]">
${p.toc.map((t) => `          <a href="#${t.id}" class="hover:text-accent transition-colors">${t.text}</a>`).join("\n")}
        </div>
      </nav>`
    : "";
  const tagsHtml = p.tags.length
    ? `      <div class="panel-plain p-5">
        <div class="label mb-3.5">Filed under</div>
        <div class="flex flex-wrap gap-[7px]">
${p.tags.map((t, j) => `          <span class="tag tag-sm${j === 0 ? " tag-accent" : ""}">${escapeHtml(t)}</span>`).join("\n")}
        </div>
      </div>`
    : "";
  const navCell = (post, dir) =>
    post
      ? `  <a href="/writing/${post.slug}" class="p-7 block${dir === "next" ? " md:text-right" : ""}">
    <div class="label${dir === "next" ? " text-accent" : ""} mb-3">${dir === "next" ? "NEXT →" : "← PREVIOUS"}</div>
    <div class="text-[19px] font-semibold tracking-[-.015em] leading-[1.25]">${escapeHtml(post.title)}</div>
  </a>`
      : `  <a href="/writing" class="p-7 block${dir === "next" ? " md:text-right" : ""}">
    <div class="label mb-3">${dir === "next" ? "ALL POSTS →" : "← ALL POSTS"}</div>
    <div class="text-[19px] font-semibold tracking-[-.015em] leading-[1.25]">Writing</div>
  </a>`;

  writeFileSync(
    join(OUT, `${p.slug}.html`),
    `<!-- generated from writing/${p.file} — do not edit by hand -->
${p.draft ? '<meta name="robots" content="noindex">\n' : ""}<div id="read-progress" aria-hidden="true"></div>

<div class="flex items-center gap-2 py-5 mono-sm tracking-[.1em]">
  <a href="/writing" class="hover:text-accent transition-colors">← Writing</a>
</div>

<article class="border-t border-rule">
  <header class="pt-12 pb-10 lg:pt-[52px] border-b border-rule max-w-[78ch]">
${p.draft ? '    <div class="eyebrow mb-4" style="color:#b45309">DRAFT — NOT PUBLISHED</div>\n' : ""}    <div class="flex items-center gap-4 font-mono text-[11px] tracking-[.14em] text-muted mb-6">
      <span class="text-accent">${escapeHtml(p.tag)}</span>
      <span>${escapeHtml(p.date)}</span>
      <span>${p.read} MIN</span>
    </div>
    <h1 class="text-[clamp(36px,4.6vw,60px)] leading-[1.02] tracking-[-.032em] font-semibold max-w-[26ch]">${escapeHtml(p.title)}</h1>
    <p class="mt-6 font-serif text-[20px] leading-[1.55] text-muted max-w-[56ch]">${escapeHtml(p.description)}</p>
  </header>

  <div class="grid grid-cols-1 lg:grid-cols-[minmax(0,1fr)_240px] gap-12 lg:gap-16 pt-12">
    <div class="prose-article">
${p.html}
      <div class="mt-12 pt-6 border-t border-rule flex flex-wrap gap-2.5 font-sans">
        <a href="/writing" class="btn btn-sm btn-outline">← All writing</a>
        <a href="/contact" class="btn btn-sm btn-ghost">Get in touch →</a>
      </div>
    </div>

    <aside class="lg:sticky lg:top-24 self-start flex flex-col gap-3.5 order-first lg:order-none">
${tocHtml}
${tagsHtml}
    </aside>
  </div>
</article>

<nav aria-label="More posts" class="rule-grid grid-cols-1${prev || next ? " md:grid-cols-2" : ""} my-16">
${prev || next ? navCell(prev, "prev") + "\n" + navCell(next, "next") : navCell(null, "prev")}
</nav>

<script>
  (function () {
    var bar = document.getElementById("read-progress");
    if (!bar) return;
    var h = document.documentElement;
    function tick() {
      var max = h.scrollHeight - h.clientHeight;
      bar.style.width = (max > 0 ? Math.min(100, (h.scrollTop / max) * 100) : 0).toFixed(1) + "%";
    }
    window.addEventListener("scroll", tick, { passive: true });
    window.addEventListener("resize", tick);
    tick();
  })();
</script>
`,
  );
});

// ---- index view ----
const topics = [...new Set(posts.flatMap((p) => [p.tag, ...p.tags]))];
writeFileSync(
  join(OUT, "index.html"),
  `<!-- generated by scripts/build-writing.mjs — do not edit by hand -->
<section class="pt-12 pb-10 lg:pt-[72px] grid grid-cols-1 lg:grid-cols-[minmax(0,1.5fr)_minmax(0,1fr)] gap-8 lg:gap-12 items-end border-b border-rule">
  <div>
    <div class="eyebrow mb-6">Writing</div>
    <h1 class="display max-w-[16ch]">Notes from writing it myself.</h1>
  </div>
  <p class="lede-muted max-w-[42ch]">
    Long-form writeups of things built here — systems work, applied LLMs in regulated environments, and what breaks
    when you remove the framework. Canonical on this domain.
  </p>
</section>

<section class="grid grid-cols-1 lg:grid-cols-[minmax(0,2.1fr)_minmax(0,1fr)] gap-10 lg:gap-14 pt-11 pb-14">
  <div>
${posts
  .map(
    (p) => `    <a href="/writing/${p.slug}" class="row">
      <div class="flex items-center gap-4 font-mono text-[10.5px] tracking-[.12em] text-muted mb-3.5">
        <span>${escapeHtml(p.date)}</span>
        <span class="text-accent">${escapeHtml(p.tag)}</span>
        <span>${p.read} MIN</span>
      </div>
      <div class="text-[24px] md:text-[27px] leading-[1.14] tracking-[-.025em] font-semibold max-w-[34ch]">${escapeHtml(p.title)}</div>
      <div class="text-[16px] leading-[1.6] text-muted mt-3 max-w-[62ch]">${escapeHtml(p.description)}</div>
    </a>`,
  )
  .join("\n")}
    <div class="mono-sm mt-5 px-2">${posts.length} ${posts.length === 1 ? "entry" : "entries"}</div>
  </div>

  <aside class="flex flex-col gap-3.5">
    <div class="panel-accent">
      <div class="label text-accent mb-3">Topics</div>
      <div class="flex flex-wrap gap-[7px]">
${topics.map((t) => `        <span class="tag tag-sm tag-accent">${escapeHtml(t)}</span>`).join("\n")}
      </div>
    </div>
    <div class="panel">
      <div class="label mb-3">How it's built</div>
      <p class="text-[15px] leading-[1.6] text-muted">Markdown in the repo, compiled to HTML at build time, served by the C framework like everything else. No newsletter, no tracking.</p>
      <a href="https://github.com/aag2807/copper-portfolio/tree/main/writing" target="_blank" rel="noreferrer" class="btn btn-sm btn-outline mt-4">writing/*.md →</a>
    </div>
  </aside>
</section>
`,
);

// ---- C manifest — compile-time allowlist for the controller ----
writeFileSync(
  join(root, "controllers", "writing_manifest.h"),
  `/* generated by scripts/build-writing.mjs — do not edit by hand */
#ifndef WRITING_MANIFEST_H
#define WRITING_MANIFEST_H

typedef struct
{
    const char* slug;
    const char* view;
    const char* title;
    const char* desc;
    const char* date; /* YYYY-MM-DD, for article:published_time */
} WritingPost;

static const WritingPost kWritingPosts[] = {
${posts
  .map(
    (p) =>
      `    {"${cString(p.slug)}", "writing/${cString(p.slug)}.html", "${cString(p.title)}", "${cString(p.description)}", "${cString(p.date)}"},`,
  )
  .join("\n")}
};

#endif /* WRITING_MANIFEST_H */
`,
);

if (skippedDrafts.length)
  console.log(`writing: skipped ${skippedDrafts.length} draft(s) (set DRAFTS=1 to build): ${skippedDrafts.join(", ")}`);
console.log(`writing: ${posts.length} post(s) -> views/writing/ + controllers/writing_manifest.h`);
