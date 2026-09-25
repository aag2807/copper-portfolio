// Renders scripts/og-card.html -> public/og.png (1200×630) in headless Chromium.
//
// LOCAL TOOLING, like build-diagrams.mjs — not part of `npm run build`.
//   NODE_PATH=$(npm root -g) PW_CHROME=/path/to/chrome node scripts/build-og.mjs
// Needs network access for the IBM Plex fonts (Google Fonts).
import { join, dirname } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const { chromium } = require("playwright");

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const browser = await chromium.launch({
  args: ["--no-sandbox"],
  ...(process.env.PW_CHROME ? { executablePath: process.env.PW_CHROME } : {}),
});
const page = await browser.newPage({ viewport: { width: 1200, height: 630 }, deviceScaleFactor: 1 });
await page.goto(pathToFileURL(join(root, "scripts", "og-card.html")).href, { waitUntil: "networkidle" });
await page.evaluate(() => document.fonts.ready);
const missing = await page.evaluate(() =>
  ["IBM Plex Sans", "IBM Plex Mono"].filter((f) => !document.fonts.check(`16px "${f}"`)),
);
if (missing.length) console.warn("fonts not loaded:", missing.join(", "));
const out = join(root, "public", "og.png");
await page.screenshot({ path: out, clip: { x: 0, y: 0, width: 1200, height: 630 } });
await browser.close();
console.log(`og: wrote ${out}`);
