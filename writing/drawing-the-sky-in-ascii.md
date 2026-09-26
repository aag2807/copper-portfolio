---
slug: drawing-the-sky-in-ascii
title: Drawing the sky in ASCII
description: How the Earth, Moon and stars behind this site are drawn: one canvas, a 1.7 KB map, alpha buckets, dirty rectangles, and a few things that went wrong.
date: 2026-09-25
tag: engineering
tags: canvas, javascript, rendering, c-copper
---

If you're reading this on a laptop, there's an Earth turning slowly behind the text, made entirely of characters. There's a Moon on the left and a bunch of little star clusters. Move your mouse over them and they light up. Switch to dark mode and the night side of the planet gets city lights.

All of it is one canvas and one JavaScript file, no libraries. It's about 31 KB, or 11 KB gzipped. I want to write down how it works, because a few parts of it were harder than I expected and I learned something from each one.

## The Earth is a grid

The globe is a grid of cells, one character per cell. Each cell is 0.6 by 1.25 times the font size, which is roughly the shape of a monospace character. For every cell inside the circle I work out where that point would sit on a sphere, and from there its latitude and longitude.

```js
var nx = (cx - globe.cx) / R, ny = -(cy - globe.cy) / R, r2 = nx * nx + ny * ny;
if (r2 > 1) continue;
var nz = Math.sqrt(1 - r2);
```

On a unit sphere that point is also the surface normal, so shading is a dot product with a fixed light direction. Plain Lambert shading. Bright cells get dense characters like `#` and `%`, dim cells get `-` and `:`.

The continents come from a land mask of 144 by 72 bits, one bit per cell of an equirectangular map. I rasterised it from Natural Earth's 110 m land data with 3×3 supersampling and stored it in the script as a base64 string, around 1.7 KB. Turning the planet just means reading that mask with a different longitude offset. The geometry for every cell is computed once, on load and on resize, so a normal frame is mostly lookups.

The Earth has a 23° axial tilt and the north pole leans a little toward you. Straight edge-on, a globe reads as a flat disc. One turn takes 150 seconds, and the rotation comes from the clock, so when you click to another page the planet continues from the same spot.

## Making it cheap

My first version cleared the whole canvas and drew every character on every frame. On the machine I was testing on, that took 60 to 80 ms per frame, which is far too much for something that is supposed to sit quietly in the background.

Three things brought it down.

First, a glyph atlas. Every character in every colour gets drawn once into an offscreen canvas, and after that drawing a character is copying a small rectangle out of it. Much cheaper than having the browser render text thousands of times a frame.

Second, alpha buckets. Changing the canvas opacity between draws costs more than you'd think, so each glyph goes into one of 24 opacity buckets and I draw bucket by bucket, changing the opacity once per bucket.

```js
function push(alpha, tint, glyph, x, y) {
  var b = Math.round((alpha / aMax) * (NB - 1));
  if (b <= 0) return false;
  if (b >= NB) b = NB - 1;
  buckets[b].push(tint * GLYPHS.length + glyph, x, y);
  return true;
}
```

Third, dirty rectangles. The globe only needs a full redraw when the rotation reaches the next column of the map, about four times a second. In between, the only thing changing near the planet is the glow under the cursor, so I clear and redraw only the cells inside this frame's glow and the previous one. Each star remembers the box it was drawn in and clears just that.

On top of that there's a frame cap: 15 fps when idle, 30 fps while the glow is moving. On phones it's 8 fps and the globe redraws about once a second. Everything moves so slowly that you can't really see the difference.

## The glow

The hover effect is a smoothstep falloff around the pointer:

```js
function smooth(d) {
  if (d >= HOVER_R) return 0;
  var k = 1 - d / HOVER_R;
  return k * k * (3 - 2 * k);
}
```

Within 190 px of the cursor, characters get more opaque, switch to denser glyphs, and shift toward green on the Earth and copper on the Moon and stars. Nearby stars get pushed out a little. The glow follows an eased copy of the pointer position, so it lags a bit behind the mouse, and that lag is a big part of why it feels smooth.

## What went wrong along the way

### Too faint

The first version was so faint you could hardly see it. I had pushed every opacity down to protect the text, and the result looked like a smudge. Honestly it was too light and not flashy enough. I doubled the base opacity, gave the land a permanent green tint and made the glow a lot stronger. That created a new problem: the see-through panels on the home page now had characters running through their text. Those panels got a frosted background, so the sky still shows behind them but blurred.

### The galaxy

The left side felt empty, so I tried a spiral galaxy there, with logarithmic arms, a bright core and some per-cell noise. The math was fine. It looked a little ugly. Flattened spiral arms on a character grid turn into horizontal streaks, and after a few rounds of tuning it still didn't read as a galaxy, so I threw it away.

### The Moon

In its place I put the Moon, and it turned out to be the cheapest thing on the page. The Moon is tidally locked, so it always shows the same face and never needs to rotate. The maria are placed roughly where the real ones are, and there are 26 random craters plus Tycho and Copernicus, each with a dark floor and a bright rim. Because nothing on it moves, every cell is computed once, and after that the Moon only costs something when the cursor is over it.

### Stars punching holes

Each star clears its previous box before it draws again. When stars drifted over the galaxy, and later the Moon, they left small holes that stayed until the next full redraw. The fix was to keep stars out of both discs.

### The cache

At some point I changed the script, refreshed, and nothing happened. The server was telling browsers to keep static files for 24 hours without checking back. My file names don't include a content hash, so the correct setup is to make the browser revalidate every time using an ETag. If the file hasn't changed, the server answers with an empty 304, which is tiny.

### Dark mode and long posts

At night the characters are brighter, their light adds up where they overlap, and each one has a small glow baked into the atlas. On the home page it looks great. Behind a long article, like this one, it made paragraphs hard to read. Reading pages now set a single attribute on the body and the whole sky dims to 45%.

### Lighthouse

A Lighthouse audit showed the background script as the biggest single CPU cost during page load. Now the sky waits until the page has loaded and the browser is idle before it starts, so the content comes first and the planet fades in right after.

## City lights

In dark mode, land on the night side can hold a light. Coastal cells get one more often than inland ones, and the polar caps get none. Each light twinkles on its own slow cycle, between 5 and 14 seconds, and they're pinned to the map, so they rotate with the planet and switch on as each region moves into the dark. They're random, there's no real city data behind them, but watching a coastline roll into night and light up is my favourite part of the whole thing.

## Staying out of the way

I didn't want any of this to get in the way of someone reading the page, so:

- With reduced motion turned on, it draws one still frame centred on the Caribbean and never animates.
- When the tab is hidden, it stops.
- The canvas never receives clicks, so links, text selection and the terminal on the home page all work normally.
- If anything throws an error, the canvas removes itself and the page looks the way it would without it.

## Why I enjoyed it

Most of my day-to-day work is payments and banking software, where a good day means nothing happened and nobody noticed. This was fun because the feedback was instant. I'd change one number, refresh, and see the result a second later.

It also surprised me how far simple math goes. The whole sky is a grid, a dot product, a bitmap of the continents and a list of opacities on a plain 2D canvas. Getting the renderer to redraw only what changed and watching the frame time drop to almost nothing felt a lot like finally getting a slow query to use the right index.

The code is in [`public/js/universe.js`](https://github.com/aag2807/copper-portfolio/blob/main/public/js/universe.js) if you want to play with it. The constants at the top control most of it: opacity, speed, glow radius, star density, even the Moon's phase.
