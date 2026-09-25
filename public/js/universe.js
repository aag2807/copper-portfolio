/* universe.js: ASCII Earth + starfield page background.
   One fixed 2D canvas behind the page (see #universe in css/app.css).
   No dependencies, no build step. Fails silently: if anything goes wrong
   the canvas is removed and the page looks exactly as it would without it. */
(function () {
  "use strict";

  /* ---- tuning knobs ------------------------------------------------------ */
  var FPS_ACTIVE = 30;        // frame cap while the pointer glow is moving
  var FPS_IDLE = 15;          // frame cap otherwise (rotation + twinkle are slow)
  var ROT_PERIOD = 150;       // seconds per full turn of the globe
  var ROT_STEPS = 4;          // globe redraws per mask column (~3.8/s); lower = cheaper
  var TILT = 0.41;            // axial tilt (rad), leans the pole to the right
  var PITCH = 0.22;           // tips the north pole toward the viewer (rad)
  var LIGHT = [-0.62, 0.42, 0.66]; // light direction (x right, y up, z to viewer)
  var FONT_PX = 10;           // glyph size (CSS px); cell = 0.6em x 1.25em
  var FONT_PX_SMALL = 9;      // glyph size on phones
  var A_LAND = 0.22;          // base alpha of lit land
  var A_OCEAN = 0.10;         // base alpha of lit ocean
  var A_STAR = 0.3;           // base alpha of a star at full twinkle
  var HOVER_R = 190;          // glow radius around the pointer (CSS px)
  var HOVER_PEAK = 0.85;      // alpha at the centre of the glow
  var A_MAX = 0.9;            // opacity ceiling for any glyph
  var LAND_TINT = 2;          // resting moss tint on land (0 = plain ink, up to TINTS-1)
  var HOVER_EASE = 9;         // pointer follow rate (1/s); lower = longer trail
  var STAR_PUSH = 22;         // max outward push of stars near the pointer (px)
  var STAR_AREA = 14000;      // one lone star per this many px^2 (desktop)
  var STAR_AREA_SMALL = 26000;// one lone star per this many px^2 (phones / touch)
  var CLUSTER_AREA = 15000;   // one little star cluster per this many px^2 (desktop)
  var CLUSTER_AREA_SMALL = 30000; // same, phones / touch
  var CLUSTER_MIN = 3, CLUSTER_MAX = 8; // stars per cluster
  var CLUSTER_SPREAD = 3;     // cluster radius in glyph cells
  var STATIC_LON = -62;       // centre longitude of the reduced-motion frame (Caribbean)

  // the Moon, upper left, balancing the Earth on the right
  var MOON_ON = true;         // false = Earth + stars only
  var A_MOON = 0.24;          // base alpha of the brightest lit highlands
  var MOON_LIGHT = [0.78, 0.22, 0.58]; // sunlight from the right: a waxing gibbous
  var MOON_EARTHSHINE = 0.16; // how much of the night side still shows (0 = none)
  var MOON_TINT = 0;          // resting tint (0 = silver ink, 1-3 = toward copper)

  var GLYPHS = ".:-=+*#%@·";          // atlas glyphs (index 9 = middle dot)
  var STAR_GLYPHS = [0, 0, 0, 0, 0, 9, 9, 9, 5, 6]; // '.' mostly, then '·', '+', '*'
  var TINTS = 4;                           // tint steps per hue (0 = ink)

  // 144x72 equirectangular land mask, 1 bit per cell, row-major from 90N/180W.
  // Rasterised from Natural Earth 110m land (world-atlas) with 3x3 supersampling.
  var MASK_W = 144, MASK_H = 72;
  var MASK_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/////AAAAAAAAAAAAAAAAAED/f//8AB8AAAAAAAAAAAAAB6Z8P//8AAAAAgAH4AAAAAAADwC8Af/8AAAAGAH/8AAAAAEADfm/wP/4AAHgEG/////gAf////////////ADwlAAAAAAAH///////////+AKAAAAAAAAAA/////YcHwGAH3//////////A////+BwDgAAPn////////74AOD//+B+AAAAPj///////4GAAAAf//h/AAACHP///////gOAAAAP//7/gAAPH////////4MAAAAH////wAAD/////////8AAAAAD///84AAD/////////wAAAAAD///+AAAB///3/////gAAAAAD///wAAAB6+Hv/////MAAAAAD///gAAAPh/73////8IAAAAAB///AAAAPAN/3////IIAAAAAB///AAAAE8Bf/////kwAAAAAA//+AAAAH+AP/////BgAAAAAAf/8AAAAP/u//////gAAAAAAAP8EAAAAf///v////gAAAAAAAD4EAAAA////z////AAAAAAAAB4AAAAA///3+H//+gAAAAAAAA4jAAAB///7/D+foAAAAAAAAA9gQAAB///7+B8fgAAAAAAAAAPgAAAB///94B4PggAAAAAAAAA4AAAB////gAwHwgAAAAAAAAAY4AAA////QAwFgQAAAAAAAAAF/AAA////wAIEAYAAAAAAAAAB/gAAf///wAICCAAAAAAAAAAB/8AAAH//gAAGGAAAAAAAAAAD/8AAAD//AAAHOAAAAAAAAAAD//AAAD/+AAADOhAAAAAAAAAD//4AAD/8AAABAh8AAAAAAAAD//8AAB/8AAAAgAeAAAAAAAAD//8AAB/8AAAAAAIAAAAAAAAB//4AAB/8AAAAAAAAAAAAAAAB//4AAB/8QAAAADkAAAAAAAAA//4AAD/8wAAAAfsAAAAAAAAAP/wAAB/wwAAAAf+AAAAAAAAAP/wAAB/xgAAAD//AAAAAAAAAP/AAAA/xgAAAH//gAAAAAAAAP+AAAA/gAAAAH//gAAAAAAAAf8AAAA/gAAAAD//gAAAAAAAAf8AAAAfAAAAAD//gAAAAAAAAf4AAAAeAAAAADw/gAAAAAAAAfgAAAAAAAAAAAAPAAAAAAAAAfgAAAAAAAAAAAACACAAAAAAA+AAAAAAAAAAAAACAEAAAAAAA8AAAAAAAAAAAAAAAYAAAAAAA4AAAAAAAAAAAAAAAQAAAAAAA4AAAAAAAAAAAAAAAAAAAAAAA4AAAAAAAAAAAAAAAAAAAAAAAYAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGAAAAAAAOAADMBgAAAAAAAAAGAAAAAAT/8f////wAAAAAAAA+AAAF////5//////wAAAHgf//AAA////////////wAAf////wAAP////////////AAX/////ADj/////////////AAB//////HH////////////+AAB/////////////////////gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

  var root = document.documentElement;
  var canvas, ctx, atlas, mask;
  var W = 0, H = 0, dpr = 1, cw = 6, ch = 12, aw = 0, ah = 0;
  var small = false, canHover = false, reduce = false;
  var globe = null, moon = null, stars = [];
  var buckets = [], NB = 24;
  var ptr = { tx: -1e4, ty: -1e4, x: -1e4, y: -1e4, h: 0, th: 0, moved: 0 };
  var raf = 0, last = 0, t0 = 0, running = false, resizeTimer = 0;
  var mqReduce, mqHover;

  function fail() {
    running = false;
    if (raf) cancelAnimationFrame(raf);
    if (canvas && canvas.parentNode) canvas.parentNode.removeChild(canvas);
  }

  function hex(name, fb) {
    var v = getComputedStyle(root).getPropertyValue(name).trim();
    var m = /^#([0-9a-f]{6})$/i.exec(v) || /^#([0-9a-f]{6})$/i.exec(fb);
    var n = parseInt(m[1], 16);
    return [n >> 16, (n >> 8) & 255, n & 255];
  }
  function mix(a, b, t) {
    return "rgb(" + Math.round(a[0] + (b[0] - a[0]) * t) + "," +
      Math.round(a[1] + (b[1] - a[1]) * t) + "," + Math.round(a[2] + (b[2] - a[2]) * t) + ")";
  }

  function decodeMask() {
    var bin = atob(MASK_B64), out = new Uint8Array(MASK_W * MASK_H);
    for (var i = 0; i < out.length; i++) out[i] = (bin.charCodeAt(i >> 3) >> (7 - (i & 7))) & 1;
    return out;
  }

  /* Glyph atlas: rows = tint (ink, ink->moss x3, ink->copper x3), cols = glyph. */
  function buildAtlas() {
    var ink = hex("--color-ink", "#14181c"),
        moss = hex("--color-accent", "#3f6b52"),
        copper = hex("--color-copper", "#a4623a");
    var fs = small ? FONT_PX_SMALL : FONT_PX;
    cw = fs * 0.6; ch = Math.round(fs * 1.25);
    aw = Math.ceil(cw * dpr) + 2; ah = Math.ceil(ch * dpr) + 2;
    atlas = document.createElement("canvas");
    atlas.width = aw * GLYPHS.length; atlas.height = ah * (TINTS * 2 - 1);
    var a = atlas.getContext("2d");
    a.font = "400 " + fs * dpr + 'px "IBM Plex Mono", ui-monospace, monospace';
    a.textAlign = "center"; a.textBaseline = "middle";
    for (var r = 0; r < TINTS * 2 - 1; r++) {
      var hue = r < TINTS ? moss : copper, k = r < TINTS ? r : r - TINTS + 1;
      a.fillStyle = mix(ink, hue, k / (TINTS - 1));
      for (var g = 0; g < GLYPHS.length; g++) a.fillText(GLYPHS[g], g * aw + aw / 2, r * ah + ah / 2);
    }
  }

  /* Per-cell geometry is fixed between resizes; only longitude changes per frame. */
  function buildGlobe() {
    var R;
    if (small) {
      R = Math.min(W * 0.42, H * 0.25);
      globe = { cx: W - R * 0.5, cy: H - R * 0.6, R: R };
    } else {
      R = Math.min(H * 0.42, W * 0.3);
      globe = { cx: W * 0.78, cy: H * 0.56, R: R };
    }
    var cols = Math.ceil((2 * R) / cw), rows = Math.ceil((2 * R) / ch);
    var x0 = globe.cx - (cols * cw) / 2, y0 = globe.cy - (rows * ch) / 2;
    var ll = Math.hypot(LIGHT[0], LIGHT[1], LIGHT[2]);
    var lx = LIGHT[0] / ll, ly = LIGHT[1] / ll, lz = LIGHT[2] / ll;
    var ct = Math.cos(TILT), st = Math.sin(TILT), cp = Math.cos(PITCH), sp = Math.sin(PITCH);
    var px = [], py = [], v = [], u = [], shade = [], rim = [], cl = [], rw = [];
    for (var j = 0; j < rows; j++) {
      for (var i = 0; i < cols; i++) {
        var cx = x0 + (i + 0.5) * cw, cy = y0 + (j + 0.5) * ch;
        if (cx < -cw || cx > W + cw || cy < -ch || cy > H + ch) continue;
        var nx = (cx - globe.cx) / R, ny = -(cy - globe.cy) / R, r2 = nx * nx + ny * ny;
        if (r2 > 1) continue;
        var nz = Math.sqrt(1 - r2);
        // undo the roll (tilt about view z), then the pitch (about view x)
        var ax = nx * ct - ny * st, ay = nx * st + ny * ct;
        var by = ay * cp + nz * sp, bz = nz * cp - ay * sp;
        var lat = Math.asin(Math.max(-1, Math.min(1, by))), lon = Math.atan2(ax, bz);
        px.push(cx); py.push(cy); cl.push(i); rw.push(j);
        v.push(Math.min(MASK_H - 1, Math.floor(((Math.PI / 2 - lat) / Math.PI) * MASK_H)));
        u.push((lon / (2 * Math.PI) + 0.5) * MASK_W);
        shade.push(Math.max(0, nx * lx + ny * ly + nz * lz));
        rim.push(r2 > 0.93 ? 2 : (i + j) % 2 === 0 && (j % 2 === 0) ? 1 : 0); // limb, sparse ocean dither
      }
    }
    globe.n = px.length;
    globe.px = new Float32Array(px); globe.py = new Float32Array(py);
    globe.v = new Uint16Array(v); globe.u = new Float32Array(u);
    globe.shade = new Float32Array(shade); globe.rim = new Uint8Array(rim);
    globe.col = new Uint16Array(cl); globe.row = new Uint16Array(rw);
    globe.x0 = x0; globe.y0 = y0; globe.cols = cols; globe.rows = rows;
    globe.rot = NaN; globe.glow = null;
  }

  /* Moon: its near side never turns away (tidal locking), so every cell's glyph
     and alpha are precomputed here and only the hover glow changes per frame.
     Albedo = bright highlands, dark maria roughly where the real ones are, and a
     scatter of craters with shadowed floors and bright rims. */
  var MARIA = [ // view-space direction (x right, y up, z to viewer), angular radius
    [-0.62, 0.12, 0.78, 0.42], [-0.3, 0.52, 0.8, 0.3], [0.2, 0.46, 0.86, 0.2],
    [0.36, 0.16, 0.92, 0.24], [0.74, 0.3, 0.6, 0.13], [0.6, -0.12, 0.79, 0.16],
    [-0.16, -0.36, 0.92, 0.19], [-0.45, -0.3, 0.84, 0.15]
  ];
  function angTo(nx, ny, nz, q) { return Math.acos(Math.min(1, nx * q[0] + ny * q[1] + nz * q[2])) / q[3]; }
  function buildMoon() {
    moon = null;
    if (!MOON_ON) return;
    var R, cx, cy, k;
    if (small) { R = Math.min(W * 0.17, H * 0.1); cx = W * 0.14; cy = H * 0.14; }
    else { R = Math.min(H * 0.17, W * 0.105); cx = W * 0.13; cy = H * 0.27; }
    var ll = Math.hypot(MOON_LIGHT[0], MOON_LIGHT[1], MOON_LIGHT[2]);
    var lx = MOON_LIGHT[0] / ll, ly = MOON_LIGHT[1] / ll, lz = MOON_LIGHT[2] / ll;
    var rand = rnd(1969), craters = [];
    for (k = 0; k < 26; k++) { // random directions on the visible hemisphere
      var z = 0.25 + rand() * 0.75, t = rand() * 2 * Math.PI, rr = Math.sqrt(1 - z * z);
      craters.push([rr * Math.cos(t), rr * Math.sin(t), z, 0.035 + rand() * rand() * 0.12]);
    }
    craters.push([-0.12, -0.72, 0.68, 0.06]); // Tycho
    craters.push([-0.38, 0.3, 0.87, 0.055]);  // Copernicus
    var cols = Math.ceil((2 * R) / cw), rows = Math.ceil((2 * R) / ch);
    var x0 = cx - (cols * cw) / 2, y0 = cy - (rows * ch) / 2;
    var px = [], py = [], cl = [], rw = [], gi = [], al = [];
    for (var j = 0; j < rows; j++) {
      for (var i = 0; i < cols; i++) {
        var x = x0 + (i + 0.5) * cw, y = y0 + (j + 0.5) * ch;
        if (x < -cw || x > W + cw || y < -ch || y > H + ch) continue;
        var nx = (x - cx) / R, ny = -(y - cy) / R, r2 = nx * nx + ny * ny;
        if (r2 > 1) continue;
        var nz = Math.sqrt(1 - r2), alb = 1, d;
        for (k = 0; k < MARIA.length; k++) {
          d = angTo(nx, ny, nz, MARIA[k]);
          if (d < 1.3) alb = Math.min(alb, 0.3 + 0.7 * Math.min(1, Math.max(0, (d - 0.75) / 0.55)));
        }
        for (k = 0; k < craters.length; k++) {
          d = angTo(nx, ny, nz, craters[k]);
          if (d < 0.75) alb *= 0.55;                            // shadowed floor
          else if (d < 1.15) alb = Math.min(1.25, alb + 0.3);   // lit rim
        }
        var lit = Math.max(0, nx * lx + ny * ly + nz * lz);
        var b = lit > 0.02 ? (0.45 + 0.55 * Math.pow(lit, 0.5)) * alb * 0.85 : MOON_EARTHSHINE * alb * 0.6;
        var g = -1, a = 0;
        if (b > 0.07) { g = Math.min(8, 1 + Math.round(b * 6.5)); a = A_MOON * (0.3 + 0.7 * Math.min(1, b)); }
        else if (r2 > 0.9) { g = 0; a = A_MOON * 0.35; }  // outline of the night side
        else if (b > 0.02) { g = 0; a = A_MOON * 0.25; }  // earthshine
        px.push(x); py.push(y); cl.push(i); rw.push(j); gi.push(g); al.push(a);
      }
    }
    moon = {
      cx: cx, cy: cy, R: R, n: px.length,
      px: new Float32Array(px), py: new Float32Array(py),
      col: new Uint16Array(cl), row: new Uint16Array(rw),
      gi: new Int8Array(gi), a: new Float32Array(al),
      x0: x0, y0: y0, cols: cols, rows: rows, glow: null
    };
  }
  function inMoon(x, y, pad) { // is a point on (or just around) the Moon's disc?
    return !!moon && Math.hypot(x - moon.cx, y - moon.cy) < moon.R + pad;
  }

  function rnd(seed) { // small deterministic PRNG so the sky is stable across resizes
    return function () { seed = (seed * 16807) % 2147483647; return (seed - 1) / 2147483646; };
  }
  function addStar(x, y, g, a, vx, vy, rand) {
    stars.push({
      x: x, y: y, g: g, a: a,
      w: (2 * Math.PI) / (3 + rand() * 8),   // twinkle: 3-11s period
      p: rand() * 2 * Math.PI,
      vx: vx, vy: vy, ox: 0, oy: 0
    });
  }
  function buildStars() {
    var lite = small || !canHover, rand = rnd(20240917), i, k;
    stars = [];
    var n = Math.round((W * H) / (lite ? STAR_AREA_SMALL : STAR_AREA));
    for (i = 0; i < n; i++) {
      var drift = rand() < 0.3;
      addStar(rand() * W, rand() * H, STAR_GLYPHS[Math.floor(rand() * STAR_GLYPHS.length)],
        0.55 + rand() * 0.45, drift ? (rand() - 0.35) * 4 : 0, drift ? (rand() - 0.5) * 1.2 : 0, rand);
    }
    // clusters: one bright '+' or '*' heart, small dots packed around it on the
    // glyph grid, all sharing one drift so the group moves as a unit
    var nc = Math.round((W * H) / (lite ? CLUSTER_AREA_SMALL : CLUSTER_AREA));
    for (i = 0; i < nc; i++) {
      var cx = rand() * W, cy = rand() * H, m = CLUSTER_MIN + Math.floor(rand() * (CLUSTER_MAX - CLUSTER_MIN + 1));
      var dr = rand() < 0.35, vx = dr ? (rand() - 0.35) * 3 : 0, vy = dr ? (rand() - 0.5) * 0.9 : 0, used = {};
      addStar(cx, cy, rand() < 0.5 ? 5 : 6, 0.9 + rand() * 0.1, vx, vy, rand); // '*' or '#' heart
      used["0,0"] = 1;
      for (k = 1; k < m; k++) {
        for (var tries = 0; tries < 6; tries++) {
          var ang = rand() * 2 * Math.PI, rad = 0.8 + rand() * CLUSTER_SPREAD;
          var gx = Math.round(Math.cos(ang) * rad), gy = Math.round(Math.sin(ang) * rad * 0.5);
          if (used[gx + "," + gy]) continue;
          used[gx + "," + gy] = 1;
          var near = Math.abs(gx) + Math.abs(gy) <= 2;
          var rg = rand(), sg = near ? (rg < 0.35 ? 5 : rg < 0.75 ? 4 : 9) : (rg < 0.4 ? 4 : rg < 0.8 ? 9 : 0);
          addStar(cx + gx * cw, cy + gy * ch, sg, (near ? 0.7 : 0.5) + rand() * 0.3, vx, vy, rand);
          break;
        }
      }
    }
  }

  function push(alpha, tint, glyph, x, y) {
    var b = Math.round((alpha / A_MAX) * (NB - 1));
    if (b <= 0) return false;
    if (b >= NB) b = NB - 1;
    buckets[b].push(tint * GLYPHS.length + glyph, x, y);
    return true;
  }

  function smooth(d) { // 1 at the pointer, 0 at HOVER_R, eased
    if (d >= HOVER_R) return 0;
    var k = 1 - d / HOVER_R;
    return k * k * (3 - 2 * k);
  }

  // One globe cell -> glyph (G.gi), alpha (G.a), tint (G.tint); false if blank.
  // f is the pointer glow at this cell (0..1).
  var G = { gi: 0, a: 0, tint: 0 };
  function cell(i, rot, f) {
    var g = globe, uu = Math.floor(g.u[i] + rot) % MASK_W;
    if (uu < 0) uu += MASK_W;
    var land = mask[g.v[i] * MASK_W + uu], s = g.shade[i], gi = -1, a = 0;
    if (land) { gi = 2 + Math.round(s * 5); a = A_LAND * (0.45 + 0.55 * s); }
    else if (g.rim[i] === 2 || (g.rim[i] && s > 0.2)) { gi = 0; a = A_OCEAN * (0.4 + 0.6 * s); }
    else if (f > 0.2) gi = 0; // the glow reveals the rest of the ocean
    if (gi < 0) return false;
    G.tint = land ? LAND_TINT : 0;
    if (f > 0) {
      a += ((land ? HOVER_PEAK : HOVER_PEAK * 0.6) - a) * f;
      G.tint = Math.max(G.tint, Math.min(TINTS - 1, Math.ceil(f * (TINTS - 1))));
      gi = Math.min(land ? 8 : 1, gi + Math.round(f * 2));
    }
    G.gi = gi; G.a = a;
    return true;
  }

  // One moon cell -> G.gi / G.a / G.tint from the precomputed face plus the glow.
  function moonCell(i, rot, f) {
    var m = moon, gi = m.gi[i], a = m.a[i];
    if (gi < 0) { if (f > 0.3) { gi = 0; a = 0; } else return false; } // glow reveals the night side
    G.tint = MOON_TINT ? TINTS - 1 + MOON_TINT : 0;
    if (f > 0) {
      a += (HOVER_PEAK - a) * f;
      G.tint = TINTS - 1 + Math.max(MOON_TINT, Math.min(TINTS - 1, Math.ceil(f * (TINTS - 1))));
      gi = Math.min(8, gi + Math.round(f * 2));
    }
    G.gi = gi; G.a = a;
    return true;
  }

  function clearBuckets() { for (var i = 0; i < NB; i++) buckets[i].length = 0; }

  // Draw queued glyphs, one globalAlpha change per bucket.
  function flush(c) {
    var ng = GLYPHS.length;
    for (var b = 1; b < NB; b++) {
      var q = buckets[b];
      if (!q.length) continue;
      c.globalAlpha = (b / (NB - 1)) * A_MAX;
      for (var k = 0; k < q.length; k += 3) {
        var id = q[k], col = id % ng, row = (id - col) / ng;
        c.drawImage(atlas, col * aw, row * ah, aw, ah,
          Math.round(q[k + 1] * dpr - aw / 2), Math.round(q[k + 2] * dpr - ah / 2), aw, ah);
      }
    }
    c.globalAlpha = 1;
    clearBuckets();
  }

  function rotation() { // longitude offset in mask columns; surface moves west -> east
    if (reduce) return (STATIC_LON / 360) * MASK_W;
    return Math.round((((Date.now() / 1000) / ROT_PERIOD) % 1) * -MASK_W * ROT_STEPS) / ROT_STEPS;
  }

  // Dirty-rect renderer: nothing is cleared that is not redrawn this frame.
  //  - the whole globe is redrawn only when the quantised rotation steps (~4/s);
  //  - otherwise only the grid-snapped cells under the pointer glow (this frame's
  //    circle plus last frame's) are cleared and redrawn;
  //  - stars clear their previous glyph box and redraw every frame.
  // Redraw one cell-grid body (globe or moon): everything on a full frame,
  // otherwise only the grid-snapped cells under this and last frame's glow.
  function paintBody(B, rot, full, h, mx, my, fn) {
    var i, lit = canHover && h > 0.005 && Math.hypot(mx - B.cx, my - B.cy) < B.R + HOVER_R;
    var cur = lit ? [mx - HOVER_R, my - HOVER_R, mx + HOVER_R, my + HOVER_R] : null;
    var c0 = 0, c1 = B.cols, r0 = 0, r1 = B.rows;
    if (!full) {
      if (!cur && !B.glow) { B.glow = null; return; }
      var a = cur || B.glow, b = B.glow || cur;
      c0 = Math.max(0, Math.floor((Math.min(a[0], b[0]) - B.x0) / cw));
      c1 = Math.min(B.cols, Math.ceil((Math.max(a[2], b[2]) - B.x0) / cw));
      r0 = Math.max(0, Math.floor((Math.min(a[1], b[1]) - B.y0) / ch));
      r1 = Math.min(B.rows, Math.ceil((Math.max(a[3], b[3]) - B.y0) / ch));
      B.glow = cur;
      if (c1 <= c0 || r1 <= r0) return;
      var X0 = Math.floor((B.x0 + c0 * cw) * dpr), Y0 = Math.floor((B.y0 + r0 * ch) * dpr);
      ctx.clearRect(X0, Y0, Math.ceil((B.x0 + c1 * cw) * dpr) - X0, Math.ceil((B.y0 + r1 * ch) * dpr) - Y0);
    } else B.glow = cur;
    for (i = 0; i < B.n; i++) {
      var col = B.col[i], row = B.row[i];
      if (col < c0 || col >= c1 || row < r0 || row >= r1) continue;
      var f = lit ? smooth(Math.hypot(B.px[i] - mx, B.py[i] - my)) * h : 0;
      if (fn(i, rot, f)) push(G.a, G.tint, G.gi, B.px[i], B.py[i]);
    }
  }

  function frame(now, dt, full) {
    var i, h = ptr.h, mx = ptr.x, my = ptr.y, g = globe;
    var t = (now - t0) / 1000, rot = rotation();

    ctx.setTransform(1, 0, 0, 1, 0, 0);
    clearBuckets();
    full = full || rot !== g.rot;
    if (full) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      for (i = 0; i < stars.length; i++) stars[i].bw = 0;
      g.rot = rot;
    }
    paintBody(g, rot, full, h, mx, my, cell);
    if (moon) paintBody(moon, 0, full, h, mx, my, moonCell);

    // stars: clear every previous glyph box first, then queue the new ones
    for (i = 0; i < stars.length; i++) {
      var st = stars[i];
      if (st.bw) ctx.clearRect(st.bx, st.by, st.bw, ah);
      st.bw = 0;
    }
    var R2 = (g.R + ch) * (g.R + ch);
    for (i = 0; i < stars.length; i++) {
      st = stars[i];
      if (!reduce && dt) {
        st.x += st.vx * dt; st.y += st.vy * dt;
        if (st.x > W + 10) st.x = -10; else if (st.x < -10) st.x = W + 10;
        if (st.y > H + 10) st.y = -10; else if (st.y < -10) st.y = H + 10;
      }
      var sx = st.x + st.ox, sy = st.y + st.oy;
      var dx = sx - g.cx, dy = sy - g.cy;
      if (dx * dx + dy * dy < R2 || inMoon(sx, sy, ch)) continue;
      var tw = reduce ? 0.8 : 0.35 + 0.65 * (0.5 + 0.5 * Math.sin(t * st.w + st.p));
      var sa = A_STAR * st.a * tw, stint = 0, tox = 0, toy = 0;
      if (h > 0.005) {
        var qx = st.x - mx, qy = st.y - my, sd = Math.hypot(qx, qy), sf = smooth(sd) * h;
        if (sf > 0) {
          sa += (HOVER_PEAK - sa) * sf;
          stint = TINTS - 1 + Math.min(TINTS - 1, Math.ceil(sf * (TINTS - 1)));
          if (sd > 0.01) { tox = (qx / sd) * STAR_PUSH * sf; toy = (qy / sd) * STAR_PUSH * sf; }
        }
      }
      if (dt) { var e = 1 - Math.exp(-dt * 6); st.ox += (tox - st.ox) * e; st.oy += (toy - st.oy) * e; }
      if (push(sa, stint, st.g, sx, sy)) {
        st.bx = Math.round(sx * dpr - aw / 2); st.by = Math.round(sy * dpr - ah / 2); st.bw = aw;
      }
    }
    flush(ctx);
  }

  function tick(now) {
    raf = 0;
    if (!running) return;
    var active = now - ptr.moved < 1500 || Math.abs(ptr.th - ptr.h) > 0.01;
    var dtms = now - last;
    if (dtms >= 1000 / (active ? FPS_ACTIVE : FPS_IDLE) - 2) {
      var dt = Math.min(0.1, dtms / 1000);
      last = now;
      var e = 1 - Math.exp(-dt * HOVER_EASE);
      ptr.x += (ptr.tx - ptr.x) * e; ptr.y += (ptr.ty - ptr.y) * e;
      ptr.h += (ptr.th - ptr.h) * (1 - Math.exp(-dt * 4));
      try { frame(now, dt); } catch (err) { fail(); return; }
    }
    raf = requestAnimationFrame(tick);
  }

  function start() {
    if (running || reduce || document.hidden) return;
    running = true; last = performance.now();
    raf = requestAnimationFrame(tick);
  }
  function stop() {
    running = false;
    if (raf) cancelAnimationFrame(raf);
    raf = 0;
  }

  function layout() {
    W = canvas.clientWidth || innerWidth; H = canvas.clientHeight || innerHeight;
    dpr = Math.min(2, window.devicePixelRatio || 1);
    small = W < 768;
    canHover = !!(mqHover && mqHover.matches) && !reduce;
    canvas.width = Math.round(W * dpr); canvas.height = Math.round(H * dpr);
    buildAtlas(); buildGlobe(); buildMoon(); buildStars();
    if (!canHover) { ptr.th = 0; ptr.h = 0; }
    frame(performance.now(), 0, true);
  }

  function onResize() {
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(function () { try { layout(); } catch (e) { fail(); } }, 150);
  }

  function onPointer(e) {
    if (!canHover || e.pointerType === "touch") return;
    ptr.tx = e.clientX; ptr.ty = e.clientY; ptr.th = 1; ptr.moved = performance.now();
    if (ptr.h < 0.005) { ptr.x = e.clientX; ptr.y = e.clientY; } // no sweep in from off-screen
  }
  function onLeave(e) { if (!e.relatedTarget) ptr.th = 0; }

  function onMotionPref() {
    reduce = mqReduce.matches;
    stop();
    try { layout(); } catch (e) { fail(); return; }
    start();
  }

  function init() {
    canvas = document.getElementById("universe");
    if (!canvas || !canvas.getContext) return;
    ctx = canvas.getContext("2d");
    if (!ctx || !window.atob) return fail();
    for (var i = 0; i < NB; i++) buckets.push([]);
    mask = decodeMask();
    mqReduce = matchMedia("(prefers-reduced-motion: reduce)");
    mqHover = matchMedia("(hover: hover) and (pointer: fine)");
    reduce = mqReduce.matches;
    t0 = performance.now();
    layout();
    canvas.classList.add("on");
    window.addEventListener("resize", onResize, { passive: true });
    if (window.visualViewport) visualViewport.addEventListener("resize", onResize, { passive: true });
    window.addEventListener("pointermove", onPointer, { passive: true });
    document.addEventListener("pointerout", onLeave, { passive: true });
    window.addEventListener("blur", function () { ptr.th = 0; });
    document.addEventListener("visibilitychange", function () {
      if (document.hidden) stop(); else start();
    });
    var onChange = function () { onMotionPref(); };
    if (mqReduce.addEventListener) { mqReduce.addEventListener("change", onChange); mqHover.addEventListener("change", onChange); }
    else if (mqReduce.addListener) { mqReduce.addListener(onChange); mqHover.addListener(onChange); }
    start();
  }

  function boot() {
    var go = function () { try { init(); } catch (e) { fail(); } };
    var fonts = document.fonts;
    if (!fonts || !fonts.load) return go();
    var done = false, once = function () { if (!done) { done = true; go(); } };
    setTimeout(once, 2500); // fall back to the monospace stack
    fonts.load('400 10px "IBM Plex Mono"').then(function () {
      if (done && canvas && ctx) onResize(); else once(); // late font: re-render the atlas
    }, once);
  }

  try { boot(); } catch (e) { fail(); }
})();
