---
slug: drawing-the-sky-in-ascii
title: Drawing the sky in ASCII
description: How the Earth, Moon and stars behind this site are drawn: one canvas, a 1.7 KB map, alpha buckets, dirty rectangles, and a few things that went wrong.
date: 2026-09-25
tag: engineering
tags: canvas, javascript, rendering, c-copper
---

Behind every page on this site there is a slowly turning Earth made of text characters, a Moon, and a sky full of little star clusters. Move your mouse over them and they glow. Switch to night mode and the Earth's dark side lights up with cities.

It is one canvas and one JavaScript file with no dependencies. The file is about 31 KB, or 11 KB gzipped. This post is about how it works, the parts that fought back, and why it was one of the most satisfying things I have built for this site.

## The Earth is a spreadsheet

The globe is a grid of cells, one glyph per cell. Each cell is 0.6 by 1.25 times the font size, which is the shape of a monospace character. For every cell inside the circle, I work out where that point sits on a sphere, then which latitude and longitude it lands on.

```js
var nx = (cx - globe.cx) / R, ny = -(cy - globe.cy) / R, r2 = nx * nx + ny * ny;
if (r2 > 1) continue;
var nz = Math.sqrt(1 - r2);
```

That gives a surface normal for free. The dot product of the normal with a fixed light direction is the shading, the old Lambert rule from every graphics course. Bright cells get dense glyphs like `#` and `%`. Dim cells get `-` and `:`.

The continents come from a land mask: 144 by 72 bits, one per cell of an equirectangular map, rasterised from Natural Earth's 110 m land data with 3×3 supersampling. It is stored as a base64 string of about 1.7 KB inside the script. To turn the planet, I only change the longitude offset I read the mask with. The geometry of every cell is computed once, when the page loads or resizes. After that, a frame is mostly table lookups.

The planet has a 23° axial tilt and tips its north pole slightly toward you, because a globe seen perfectly edge-on looks like a flat disc. One full turn takes 150 seconds. The rotation is derived from the wall clock, so when you move between pages the Earth picks up exactly where it was.

## Drawing thousands of glyphs cheaply

The first version cleared the canvas and drew every glyph on every frame. On the machine I tested it on, that cost 60 to 80 ms per frame. A background that eats the main thread is a background that makes the whole site feel slow.

Three changes fixed it.

**A glyph atlas.** Every glyph in every colour is drawn once into an offscreen canvas. Drawing a character becomes copying a small rectangle, which is much cheaper than asking the browser to lay out text thousands of times per frame.

**Alpha buckets.** Changing the canvas opacity between draws is surprisingly expensive, so glyphs are sorted into 24 opacity buckets and drawn bucket by bucket, with one opacity change each.

```js
function push(alpha, tint, glyph, x, y) {
  var b = Math.round((alpha / aMax) * (NB - 1));
  if (b <= 0) return false;
  if (b >= NB) b = NB - 1;
  buckets[b].push(tint * GLYPHS.length + glyph, x, y);
  return true;
}
```

**Dirty rectangles.** The globe only needs a full redraw when its rotation moves to the next column of the map, about four times a second. Between those, the only thing that changes near the planet is the glow under your cursor, so I clear and redraw just the grid cells inside this frame's glow and last frame's. Stars remember the box they were drawn in and clear only that.

The frame rate is capped at 15 fps when nothing is happening and 30 fps while the glow is moving. On phones it drops to 8 fps and the globe redraws about once a second. The motion is slow enough that nobody can tell.

## The glow

The hover effect is a smoothstep falloff around the pointer:

```js
function smooth(d) {
  if (d >= HOVER_R) return 0;
  var k = 1 - d / HOVER_R;
  return k * k * (3 - 2 * k);
}
```

Inside the 190 px radius, glyphs gain opacity, step up to denser characters, and tint toward moss green on the Earth and copper on the Moon and stars. Stars also get pushed gently outward. The pointer position is eased, so the glow trails your cursor a little. That small delay is what makes it feel soft, like light spilling from the pointer.

## Things that fought back

**It was too polite.** The first version was so faint you could barely tell it was there. Keeping body text readable pushed every opacity down, and the result looked like a smudge on the screen. I raised the base opacity, gave land a permanent green tint and made the glow much stronger. Then the text had problems, so see-through panels got a frosted background that keeps the sky visible but blurred behind the words.

**The galaxy.** The left side of the page felt empty, so I drew a spiral galaxy there: logarithmic spiral arms, a bright core, per-cell noise for speckle. It was technically correct. It also looked like someone had wiped a dirty thumb across the page. Flattened arms in a character grid turn into horizontal bands, and no amount of tuning made it read as a galaxy. I deleted it.

**The Moon was the answer.** It pairs with the Earth, and it has a property that made it cheap: the Moon is tidally locked, so we always see the same face. Its maria sit roughly where the real ones are, and 26 craters, plus Tycho and Copernicus, have dark floors and bright rims. Since nothing on it ever turns, every cell's glyph and opacity is computed once. After that the Moon costs nothing until your cursor touches it.

**Stars erasing things.** Each star clears its old box before drawing its new position. When stars drifted over the galaxy, and later the Moon, they punched little holes in it that stayed until the next full redraw. Stars now skip the discs of both bodies.

**The cache.** At one point I changed the script, refreshed, and saw nothing new. The server was telling browsers to keep static files for 24 hours without asking again. My files have no content hash in their names, so the right answer is to let the browser revalidate every time with an ETag. When nothing changed, the reply is an empty 304.

**Night mode and reading.** At night the glyphs get brighter, add their light together where they overlap, and carry a small glow baked into the atlas. It looks great on the home page. Behind a long article it made paragraphs hard to read. Reading pages now set one attribute on the page body and the whole sky dims to 45%.

**Lighthouse.** An audit showed the background script as the largest single CPU cost while the page loads. The sky now waits for the page to finish loading and for the browser to be idle before it starts. Nobody needs a planet in the first 200 milliseconds.

## Night lights

In dark mode, land on the unlit side of the Earth can hold a city light: coastal cells more often than inland ones, and none on the polar caps. Each light twinkles on its own slow period of 5 to 14 seconds, and the lights are tied to the map, so they rotate with the planet and come on as each region crosses into night. It is fake, in the sense that the lights are random and there are no real cities in the data. I still catch myself watching them.

## Being careful with it

A moving background is a gamble with the people reading the page, so the script tries to lose gracefully.

- With reduced motion turned on, it draws one still frame, centred on the Caribbean, and never animates.
- When the tab is hidden, it stops completely.
- The canvas never receives clicks, so it cannot block links, text selection or the terminal on the home page.
- If anything throws, the canvas removes itself and the page looks exactly as it would without it.

## Why I liked building it

Most of my work is systems that move money. They succeed by being boring, and their best days are the ones nobody notices. This was the opposite: a problem where I could see every decision on the screen within a second of making it.

It was also a reminder of how far careful arithmetic goes. The whole sky is a grid, a dot product, a bitmap of the continents and a list of opacities, running on a plain 2D canvas in a browser that is very good at copying small rectangles. When I got the renderer to redraw only what changed and saw the frame cost fall to almost nothing, it felt like the same pleasure I get from a query plan that finally uses the index.

The whole thing lives in [`public/js/universe.js`](https://github.com/aag2807/copper-portfolio/blob/main/public/js/universe.js). The constants at the top control nearly everything: opacity, speed, glow radius, star density and the Moon's phase. Change a few numbers and you get a different sky.
