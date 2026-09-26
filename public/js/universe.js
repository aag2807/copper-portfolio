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
  var FPS_TOUCH = 8;          // idle frame cap on touch / no-hover devices (phones)
  var ROT_STEPS_TOUCH = 1;    // globe redraws ~1/s on phones: same motion, far less CPU
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

  // night theme (dark tokens): the ink token is then a light paper colour, so
  // the same atlas turns luminous; these replace the day values above
  var A_LAND_N = 0.44;        // A_LAND at night
  var A_OCEAN_N = 0.18;       // A_OCEAN at night
  var A_STAR_N = 0.75;        // A_STAR at night
  var A_MOON_N = 0.38;        // A_MOON at night: a brighter Moon
  var HOVER_PEAK_N = 0.9;     // HOVER_PEAK at night
  var A_MAX_N = 1;            // A_MAX at night
  var NIGHT_DIM_SMALL = 0.5; // phones at night: Earth + Moon sit behind body text, so dim them
  var READ_DIM = 0.45;        // pages with <body data-sky="dim"> (long-form reading): Earth, Moon
                              // and stars at this fraction of their usual brightness, both themes
  var LAND_FLOOR = 0.45;      // alpha share kept by unlit land (day)
  var LAND_FLOOR_N = 0.2;     // same at night: darker night side so city lights read
  var GLOW_N = 2;              // glow radius baked into the night atlas (CSS px); 0 = none
  var GLOW_A_N = 1;           // glow strength (shadow alpha)
  var ADDITIVE_N = true;      // night glyphs add light ('lighter'), so glows overlap softly
  // city lights: sparse warm glyphs on land on the Earth's night side (night only)
  var CITY_ON = true;
  var CITY_INLAND = 0.16;     // chance an inland land cell holds a light
  var CITY_COAST = 0.5;       // chance a coastal land cell holds a light
  var CITY_DUSK = 0.1;        // lights come on where the shade drops below this
  var A_CITY = 0.95;          // peak alpha of a light
  var CITY_TWINKLE = 0.45;    // twinkle depth (0 = steady); period 5-14 s per light

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
  var buckets = [], ALPHA_BUCKETS = 24;
  var ptr = { tx: -1e4, ty: -1e4, x: -1e4, y: -1e4, h: 0, th: 0, moved: 0 };
  var raf = 0, last = 0, t0 = 0, running = false, resizeTimer = 0;
  var mqReduce, mqHover;
  // live values: the day constants, or their night variants (see applyTheme)
  var night = false, aLand = A_LAND, aOcean = A_OCEAN, aStar = A_STAR, aMoon = A_MOON,
      hoverPeak = HOVER_PEAK, aMax = A_MAX, landFloor = LAND_FLOOR, pad = 0, tnow = 0;
  var cities = null; // per mask cell: 0 = no light, else twinkle phase (rad) + 1

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
  function applyTheme() { // night = the page background token is dark
    var p = hex("--color-paper", "#e9ecef");
    night = (0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]) / 255 < 0.35;
    aLand = night ? A_LAND_N : A_LAND; aOcean = night ? A_OCEAN_N : A_OCEAN;
    aStar = night ? A_STAR_N : A_STAR; aMoon = night ? A_MOON_N : A_MOON;
    hoverPeak = night ? HOVER_PEAK_N : HOVER_PEAK; aMax = night ? A_MAX_N : A_MAX;
    landFloor = night ? LAND_FLOOR_N : LAND_FLOOR;
    if (night && small) { aLand *= NIGHT_DIM_SMALL; aOcean *= NIGHT_DIM_SMALL; aMoon *= NIGHT_DIM_SMALL; }
    if (document.body && document.body.getAttribute("data-sky") === "dim") {
      aLand *= READ_DIM; aOcean *= READ_DIM; aMoon *= READ_DIM; aStar *= READ_DIM;
    }
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

  /* City lights live on the map (they turn with the Earth), not on the screen.
     Coasts get more; the polar caps (above ~65N, below ~55S) get none. */
  function buildCities() {
    var rand = rnd(1882), out = new Float32Array(MASK_W * MASK_H);
    for (var y = 10; y < 58; y++) {
      for (var x = 0; x < MASK_W; x++) {
        var i = y * MASK_W + x;
        if (!mask[i]) continue;
        var coast = !mask[i - MASK_W] || !mask[i + MASK_W] ||
          !mask[y * MASK_W + (x + 1) % MASK_W] || !mask[y * MASK_W + (x + MASK_W - 1) % MASK_W];
        if (rand() < (coast ? CITY_COAST : CITY_INLAND)) out[i] = 1 + rand() * 2 * Math.PI;
      }
    }
    return out;
  }

  /* Glyph atlas: rows = tint (ink, ink->moss x3, ink->copper x3), cols = glyph. */
  function buildAtlas() {
    var ink = hex("--color-ink", "#14181c"),
        moss = hex("--color-accent", "#3f6b52"),
        copper = hex("--color-copper", "#a4623a");
    var fs = small ? FONT_PX_SMALL : FONT_PX;
    cw = fs * 0.6; ch = Math.round(fs * 1.25);
    // night: every cell gets room for a glow baked in once here (cheaper than
    // shadowBlur per draw); pad is in device px on each side
    pad = night && GLOW_N > 0 ? Math.ceil(GLOW_N * dpr) + 2 : 0;
    aw = Math.ceil(cw * dpr) + 2 + 2 * pad; ah = Math.ceil(ch * dpr) + 2 + 2 * pad;
    atlas = document.createElement("canvas");
    atlas.width = aw * GLYPHS.length; atlas.height = ah * (TINTS * 2 - 1);
    var a = atlas.getContext("2d");
    a.font = "400 " + fs * dpr + 'px "IBM Plex Mono", ui-monospace, monospace';
    a.textAlign = "center"; a.textBaseline = "middle";
    for (var r = 0; r < TINTS * 2 - 1; r++) {
      var hue = r < TINTS ? moss : copper, k = r < TINTS ? r : r - TINTS + 1;
      a.fillStyle = mix(ink, hue, k / (TINTS - 1));
      if (pad) {
        a.shadowColor = a.fillStyle.replace("rgb(", "rgba(").replace(")", "," + GLOW_A_N + ")");
        a.shadowBlur = GLOW_N * dpr * 1.4;
      }
      for (var g = 0; g < GLYPHS.length; g++) {
        a.fillText(GLYPHS[g], g * aw + aw / 2, r * ah + ah / 2);
        if (pad) a.fillText(GLYPHS[g], g * aw + aw / 2, r * ah + ah / 2); // second pass: a fuller halo
      }
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
    var gridLeft = globe.cx - (cols * cw) / 2;
    var gridTop = globe.cy - (rows * ch) / 2;

    var lightLength = Math.hypot(LIGHT[0], LIGHT[1], LIGHT[2]);
    var lightX = LIGHT[0] / lightLength, lightY = LIGHT[1] / lightLength, lightZ = LIGHT[2] / lightLength;
    var cosTilt = Math.cos(TILT), sinTilt = Math.sin(TILT);
    var cosPitch = Math.cos(PITCH), sinPitch = Math.sin(PITCH);

    var screenX = [], screenY = [], mapRow = [], mapCol = [], shade = [], rim = [], gridCol = [], gridRow = [];

    for (var row = 0; row < rows; row++) {
      for (var col = 0; col < cols; col++) {
        var cellX = gridLeft + (col + 0.5) * cw;
        var cellY = gridTop + (row + 0.5) * ch;
        var offScreen = cellX < -cw || cellX > W + cw || cellY < -ch || cellY > H + ch;
        if (offScreen) continue;

        // Where this cell sits on a unit disc centred on the globe (y points up).
        var x = (cellX - globe.cx) / R;
        var y = -(cellY - globe.cy) / R;
        var distanceSquared = x * x + y * y;
        if (distanceSquared > 1) continue; // outside the planet

        // Lift the point onto the sphere. On a unit sphere, that point is also
        // the surface normal, which is what the lighting needs.
        var z = Math.sqrt(1 - distanceSquared);

        // Undo the axial tilt (a roll around the view axis)...
        var untiltedX = x * cosTilt - y * sinTilt;
        var untiltedY = x * sinTilt + y * cosTilt;
        // ...then the pitch that leans the north pole toward the viewer.
        var northward = untiltedY * cosPitch + z * sinPitch;
        var towardViewer = z * cosPitch - untiltedY * sinPitch;

        var latitude = Math.asin(Math.max(-1, Math.min(1, northward)));
        var longitude = Math.atan2(untiltedX, towardViewer);

        screenX.push(cellX); screenY.push(cellY); gridCol.push(col); gridRow.push(row);
        mapRow.push(Math.min(MASK_H - 1, Math.floor(((Math.PI / 2 - latitude) / Math.PI) * MASK_H)));
        mapCol.push((longitude / (2 * Math.PI) + 0.5) * MASK_W);
        shade.push(Math.max(0, x * lightX + y * lightY + z * lightZ)); // Lambert: normal . light
        var onLimb = distanceSquared > 0.93;
        var oceanDither = (col + row) % 2 === 0 && row % 2 === 0;
        rim.push(onLimb ? 2 : oceanDither ? 1 : 0);
      }
    }
    globe.n = screenX.length;
    globe.px = new Float32Array(screenX); globe.py = new Float32Array(screenY);
    globe.v = new Uint16Array(mapRow); globe.u = new Float32Array(mapCol);
    globe.shade = new Float32Array(shade); globe.rim = new Uint8Array(rim);
    globe.col = new Uint16Array(gridCol); globe.row = new Uint16Array(gridRow);
    globe.x0 = gridLeft; globe.y0 = gridTop; globe.cols = cols; globe.rows = rows;
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
        if (b > 0.07) { g = Math.min(8, 1 + Math.round(b * 6.5)); a = aMoon * (0.3 + 0.7 * Math.min(1, b)); }
        else if (r2 > 0.9) { g = 0; a = aMoon * 0.35; }  // outline of the night side
        else if (b > 0.02) { g = 0; a = aMoon * 0.25; }  // earthshine
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

  // Glyphs are grouped by opacity, so drawing a whole group costs a single
  // globalAlpha change. Returns false when the glyph is too faint to draw.
  function queueGlyph(alpha, tint, glyph, x, y) {
    var bucket = Math.round((alpha / aMax) * (ALPHA_BUCKETS - 1));
    if (bucket <= 0) return false;
    if (bucket >= ALPHA_BUCKETS) bucket = ALPHA_BUCKETS - 1;
    var atlasIndex = tint * GLYPHS.length + glyph;
    buckets[bucket].push(atlasIndex, x, y);
    return true;
  }

  // How strongly the pointer glow affects a point: 1 right under the pointer,
  // easing down to 0 at HOVER_R. The easing curve is smoothstep.
  function glowStrength(distance) {
    if (distance >= HOVER_R) return 0;
    var t = 1 - distance / HOVER_R;
    return t * t * (3 - 2 * t);
  }

  // One globe cell -> glyph (G.gi), alpha (G.a), tint (G.tint); false if blank.
  // f is the pointer glow at this cell (0..1).
  var G = { gi: 0, a: 0, tint: 0 };
  function cell(i, rot, f) {
    var g = globe, uu = Math.floor(g.u[i] + rot) % MASK_W;
    if (uu < 0) uu += MASK_W;
    var mi = g.v[i] * MASK_W + uu, land = mask[mi], s = g.shade[i], gi = -1, a = 0;
    if (land) { gi = 2 + Math.round(s * 5); a = aLand * (landFloor + (1 - landFloor) * s); }
    else if (g.rim[i] === 2 || (g.rim[i] && s > 0.2)) { gi = 0; a = aOcean * (0.4 + 0.6 * s); }
    else if (f > 0.2) gi = 0; // the glow reveals the rest of the ocean
    if (gi < 0) return false;
    G.tint = land ? LAND_TINT : 0;
    var c = land && night && cities && s < CITY_DUSK ? cities[mi] : 0;
    if (c) { // a city light: warm copper '.' or '·', slow deterministic twinkle
      var tw = reduce ? 1 : 1 - CITY_TWINKLE * (0.5 + 0.5 * Math.sin(tnow * (0.45 + (c % 1) * 0.8) + c));
      gi = c > 6.9 ? 4 : c > 3.4 ? 9 : 0; G.tint = 2 * (TINTS - 1); // mostly '.' and '·', a few '+' metros
      a = A_CITY * tw * Math.min(1, (CITY_DUSK - s) / (CITY_DUSK * 0.5));
    }
    if (f > 0) {
      a += ((land ? hoverPeak : hoverPeak * 0.6) - a) * f;
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
      a += (hoverPeak - a) * f;
      G.tint = TINTS - 1 + Math.max(MOON_TINT, Math.min(TINTS - 1, Math.ceil(f * (TINTS - 1))));
      gi = Math.min(8, gi + Math.round(f * 2));
    }
    G.gi = gi; G.a = a;
    return true;
  }

  function clearBuckets() { for (var i = 0; i < ALPHA_BUCKETS; i++) buckets[i].length = 0; }

  // Draw queued glyphs, one globalAlpha change per bucket.
  function flush(c) {
    var ng = GLYPHS.length;
    for (var b = 1; b < ALPHA_BUCKETS; b++) {
      var q = buckets[b];
      if (!q.length) continue;
      c.globalAlpha = (b / (ALPHA_BUCKETS - 1)) * aMax;
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
    var k = canHover ? ROT_STEPS : ROT_STEPS_TOUCH;
    return Math.round((((Date.now() / 1000) / ROT_PERIOD) % 1) * -MASK_W * k) / k;
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
      // night glyphs carry a glow halo of `pad` device px: clear that much
      // further out, repaint the ring of neighbours whose halos reach in, and
      // clip, so nothing outside the rect is drawn twice
      var X0 = Math.floor((B.x0 + c0 * cw) * dpr) - pad, Y0 = Math.floor((B.y0 + r0 * ch) * dpr) - pad;
      var X1 = Math.ceil((B.x0 + c1 * cw) * dpr) + pad, Y1 = Math.ceil((B.y0 + r1 * ch) * dpr) + pad;
      ctx.clearRect(X0, Y0, X1 - X0, Y1 - Y0);
      if (pad) {
        var ec = Math.ceil(pad / dpr / cw) + 1, er = Math.ceil(pad / dpr / ch) + 1;
        c0 = Math.max(0, c0 - ec); c1 = Math.min(B.cols, c1 + ec);
        r0 = Math.max(0, r0 - er); r1 = Math.min(B.rows, r1 + er);
        var clip = [X0, Y0, X1 - X0, Y1 - Y0];
      }
    } else B.glow = cur;
    for (i = 0; i < B.n; i++) {
      var col = B.col[i], row = B.row[i];
      if (col < c0 || col >= c1 || row < r0 || row >= r1) continue;
      var f = lit ? glowStrength(Math.hypot(B.px[i] - mx, B.py[i] - my)) * h : 0;
      if (fn(i, rot, f)) queueGlyph(G.a, G.tint, G.gi, B.px[i], B.py[i]);
    }
    if (clip) {
      ctx.save(); ctx.beginPath(); ctx.rect(clip[0], clip[1], clip[2], clip[3]); ctx.clip();
      flush(ctx);
      ctx.restore();
    }
  }

  function frame(now, dt, full) {
    var i, h = ptr.h, mx = ptr.x, my = ptr.y, g = globe;
    var t = (now - t0) / 1000, rot = rotation();
    tnow = Date.now() / 1000; // wall clock, so city lights carry across pages

    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.globalCompositeOperation = night && ADDITIVE_N ? "lighter" : "source-over";
    clearBuckets();
    full = full || rot !== g.rot;
    if (full) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      for (i = 0; i < stars.length; i++) stars[i].bw = 0;
      g.rot = rot;
    }
    // stars: clear every previous glyph box first (before the bodies, which
    // may flush early at night), then queue the new ones below
    for (i = 0; i < stars.length; i++) {
      var st = stars[i];
      if (st.bw) ctx.clearRect(st.bx, st.by, st.bw, ah);
      st.bw = 0;
    }
    paintBody(g, rot, full, h, mx, my, cell);
    if (moon) paintBody(moon, 0, full, h, mx, my, moonCell);

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
      var sa = aStar * st.a * tw, stint = 0, tox = 0, toy = 0;
      if (h > 0.005) {
        var qx = st.x - mx, qy = st.y - my, sd = Math.hypot(qx, qy), sf = glowStrength(sd) * h;
        if (sf > 0) {
          sa += (hoverPeak - sa) * sf;
          stint = TINTS - 1 + Math.min(TINTS - 1, Math.ceil(sf * (TINTS - 1)));
          if (sd > 0.01) { tox = (qx / sd) * STAR_PUSH * sf; toy = (qy / sd) * STAR_PUSH * sf; }
        }
      }
      if (dt) { var e = 1 - Math.exp(-dt * 6); st.ox += (tox - st.ox) * e; st.oy += (toy - st.oy) * e; }
      if (queueGlyph(sa, stint, st.g, sx, sy)) {
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
    if (dtms >= 1000 / (active ? FPS_ACTIVE : canHover ? FPS_IDLE : FPS_TOUCH) - 2) {
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
    applyTheme();
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
    for (var i = 0; i < ALPHA_BUCKETS; i++) buckets.push([]);
    mask = decodeMask();
    if (CITY_ON) cities = buildCities();
    mqReduce = matchMedia("(prefers-reduced-motion: reduce)");
    mqHover = matchMedia("(hover: hover) and (pointer: fine)");
    reduce = mqReduce.matches;
    t0 = performance.now();
    layout();
    canvas.classList.add("on");
    try { sessionStorage.setItem("u-seen", "1"); } catch (e) {} // later pages skip the fade-in
    // night/day switch (header toggle, or the OS theme while no choice is stored):
    // rebuild synchronously so a view-transition snapshot already has the new sky
    document.addEventListener("themechange", function () { try { layout(); } catch (e) { fail(); } });
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
    var run = function () { try { init(); } catch (e) { fail(); } };
    // wait for load + an idle moment so the sky never competes with first paint / input
    var go = function () {
      var idle = function () {
        if (window.requestIdleCallback) requestIdleCallback(run, { timeout: 1200 }); else setTimeout(run, 200);
      };
      if (document.readyState === "complete") idle(); else window.addEventListener("load", idle, { once: true });
    };
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
