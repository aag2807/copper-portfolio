// Build-time mermaid rendering: diagrams/*.mmd -> inline SVG in views.
//
// Runs mermaid (from node_modules) in headless Chromium via Playwright and
// splices the resulting SVG into view files between marker comments:
//   <!-- diagram:NAME --> ... <!-- /diagram:NAME -->
// where NAME matches diagrams/NAME.mmd.
//
// LOCAL TOOLING — not part of `npm run build` (the Docker build stage has no
// browser). Patched views are committed; rerun this only when a .mmd changes:
//   NODE_PATH=$(npm root -g) node scripts/build-diagrams.mjs
import { readFileSync, writeFileSync, readdirSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const { chromium } = require("playwright");

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const MERMAID = join(root, "node_modules", "mermaid", "dist", "mermaid.min.js");

// Site palette (paper editorial): paper #e9ecef / card #f7f8f9 / ink #14181c /
// rule #ccd4da / accent (moss) #3f6b52 / copper #a4623a.
const THEME = {
  theme: "base",
  themeVariables: {
    darkMode: false,
    background: "#f7f8f9",
    primaryColor: "#e9ecef",
    primaryTextColor: "#14181c",
    primaryBorderColor: "#9aa4ab",
    lineColor: "#3f6b52",
    secondaryColor: "#dde7e0",
    tertiaryColor: "#f7f8f9",
    clusterBkg: "#f0f2f4",
    clusterBorder: "#ccd4da",
    edgeLabelBackground: "#f7f8f9",
    fontFamily: '"IBM Plex Mono", ui-monospace, monospace',
    fontSize: "14px",
  },
  flowchart: { useMaxWidth: true, curve: "linear", padding: 12 },
  securityLevel: "loose",
  startOnLoad: false,
};

const sources = readdirSync(join(root, "diagrams")).filter((f) => f.endsWith(".mmd"));
// PW_CHROME=/path/to/chrome overrides the browser binary when the global
// playwright install and its cached browsers drift out of sync.
const browser = await chromium.launch({
  args: ["--no-sandbox"],
  ...(process.env.PW_CHROME ? { executablePath: process.env.PW_CHROME } : {}),
});
const page = await browser.newPage();
await page.setContent("<!doctype html><body></body>");
await page.addScriptTag({ path: MERMAID });
await page.evaluate((cfg) => window.mermaid.initialize(cfg), THEME);

const svgs = {};
for (const f of sources) {
  const name = f.replace(/\.mmd$/, "");
  const src = readFileSync(join(root, "diagrams", f), "utf8");
  const svg = await page.evaluate(async ({ name, src }) => {
    const { svg } = await window.mermaid.render("d_" + name.replace(/-/g, "_"), src);
    return svg;
  }, { name, src });
  svgs[name] = svg;
  console.log(`rendered ${f} (${(svg.length / 1024).toFixed(1)}KB svg)`);
}
await browser.close();

// splice into views between markers
const viewFiles = [
  "views/projects/c-copper.html",
  "views/projects/llm-gateway.html",
  "views/projects/payment-gateways.html",
];
for (const vf of viewFiles) {
  const path = join(root, vf);
  let html = readFileSync(path, "utf8");
  let touched = false;
  for (const [name, svg] of Object.entries(svgs)) {
    const re = new RegExp(`(<!-- diagram:${name} -->)[\\s\\S]*?(<!-- /diagram:${name} -->)`);
    if (re.test(html)) {
      html = html.replace(re, `$1\n${svg}\n$2`);
      touched = true;
      console.log(`spliced ${name} -> ${vf}`);
    }
  }
  if (touched) writeFileSync(path, html);
}
