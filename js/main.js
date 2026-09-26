const { LuaFactory } = require("wasmoon");

const BASE = "/static/lua";
// Self-hosted: without an argument wasmoon fetches glue.wasm from unpkg.com.
const WASM = "/static/framework/glue.wasm";

const fetchText = (url) =>
  fetch(url).then((r) => {
    if (!r.ok) throw new Error(`${url}: HTTP ${r.status}`);
    return r.text();
  });

async function boot() {
  const mod = document.body.dataset.luaModule;
  if (!mod) return;

  const manifest = await fetch(`${BASE}/manifest.json`).then((r) => r.json());

  const factory = new LuaFactory(WASM);

  const library = await Promise.all(
    manifest.library.map(async (path) => [path, await fetchText(`${BASE}/${path}`)]),
  );

  for (const [path, content] of library) {
    await factory.mountFile(path, content);
  }

  const pagePath = `${mod}.lua`;
  const pageSrc = await fetchText(`${BASE}/${pagePath}`);
  await factory.mountFile(pagePath, pageSrc);

  // Any source panel on the page shows the exact bytes just mounted.
  showSource(pagePath, pageSrc);

  const pg = document.querySelector("[data-lua-playground]");
  if (pg) return playground(pg, factory, pagePath);

  const lua = await factory.createEngine();
  window.__lua = lua;
  lua.global.set("dom", makeBridge());

  await lua.doString(`package.path = "./?.lua;./?/init.lua"`);
  await lua.doFile(pagePath);
}

// root: scope mount/query to one element (a selector that isn't found inside
// it mounts into the root itself). onError: report handler errors instead of
// throwing them into the page.
function makeBridge({ root = null, onError = null } = {}) {
  const find = (sel) => (root ? root.querySelector(sel) : document.querySelector(sel));
  return {
    create: (tag) => document.createElement(tag),
    text: (s) => document.createTextNode(s),
    append: (p, c) => p.appendChild(c),
    setAttr: (el, k, v) => el.setAttribute(k, v),
    setText: (n, t) => {
      n.nodeValue = t;
    },
    clear: (el) => el.replaceChildren(),
    on: (el, evt, fn) =>
      el.addEventListener(
        evt,
        onError
          ? (e) => {
              try {
                fn(e);
              } catch (err) {
                onError(err);
              }
            }
          : fn,
      ),
    mount: (el, sel) => (find(sel) || root || document.body).appendChild(el),
    query: (sel) => find(sel),
  };
}

// ---- source panels -----------------------------------------------------------
// <pre data-lua-source="pages/x.lua" data-region="name"> gets the lines between
// "-- @region name" and "-- @endregion" of the running module, highlighted.

const KEYWORDS = new Set(
  "and break do else elseif end false for function goto if in local nil not or repeat return then true until while".split(" "),
);
const escapeHtml = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

function highlightLua(src) {
  const re =
    /(--\[\[[\s\S]*?\]\]|--[^\n]*)|("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*'|\[\[[\s\S]*?\]\])|\b(\d+(?:\.\d+)?)\b|\b([A-Za-z_]\w*)\b/g;
  let out = "";
  let last = 0;
  for (let m; (m = re.exec(src)); ) {
    out += escapeHtml(src.slice(last, m.index));
    const [tok, comment, str, num, word] = m;
    const cls = comment ? "c" : str ? "s" : num ? "n" : word && KEYWORDS.has(word) ? "k" : null;
    out += cls ? `<span class="${cls}">${escapeHtml(tok)}</span>` : escapeHtml(tok);
    last = re.lastIndex;
  }
  return out + escapeHtml(src.slice(last));
}

function showSource(path, src) {
  document.querySelectorAll(`[data-lua-source="${path}"]`).forEach((el) => {
    const lines = src.replace(/\n$/, "").split("\n");
    let from = 0;
    let to = lines.length;
    const region = el.dataset.region;
    if (region) {
      const start = lines.findIndex((l) => l.trim() === `-- @region ${region}`);
      const end = lines.findIndex((l, i) => i > start && l.trim() === "-- @endregion");
      if (start >= 0 && end > start) [from, to] = [start + 1, end];
    }
    el.innerHTML = highlightLua(lines.slice(from, to).join("\n"));
    const range = document.querySelector(`[data-lua-source-range="${path}"]`);
    if (range) range.textContent = `lines ${from + 1}–${to} of ${lines.length}`;
  });
}

// ---- playground ----------------------------------------------------------------

const PRESETS = {
  counter: { label: "Counter", src: `${BASE}/pages/counter.lua` },
  todos: { label: "Todos", src: `${BASE}/pages/todolist.lua` },
  hello: { label: "Hello, Lua", src: `${BASE}/playground/hello.lua` },
  ledger: { label: "Ledger", src: `${BASE}/playground/ledger.lua` },
};
const HANDLER_TIMEOUT_MS = 1500;
const MAX_CONSOLE_LINES = 500;

// localStorage can throw (private mode, blocked storage): every access is guarded.
const store = {
  get(k) {
    try {
      return localStorage.getItem(k);
    } catch (e) {
      return null;
    }
  },
  set(k, v) {
    try {
      localStorage.setItem(k, v);
    } catch (e) {}
  },
  del(k) {
    try {
      localStorage.removeItem(k);
    } catch (e) {}
  },
};

async function playground(pg, factory, runnerPath) {
  const $ = (sel) => pg.querySelector(sel);
  const editor = $("[data-pg-editor]");
  const gutter = $("[data-pg-gutter]");
  const sandbox = $("#sandbox");
  const out = $("[data-pg-console]");
  const status = $("[data-pg-status]");
  const saved = $("[data-pg-saved]");
  const presetButtons = pg.querySelectorAll("[data-pg-preset]");

  const originals = {};
  let current = store.get("pg:preset");
  if (!PRESETS[current]) current = "counter";
  let engine = null;
  let running = false;

  // -- console
  let lineCount = 0;
  function log(text, kind = "out") {
    if (lineCount >= MAX_CONSOLE_LINES) {
      if (lineCount === MAX_CONSOLE_LINES) {
        const t = document.createElement("div");
        t.className = "pg-line pg-info";
        t.textContent = `output truncated at ${MAX_CONSOLE_LINES} lines`;
        out.appendChild(t);
        lineCount++;
      }
      return;
    }
    lineCount++;
    const line = document.createElement("div");
    line.className = `pg-line pg-${kind}`;
    if (kind === "err") {
      const tag = document.createElement("span");
      tag.className = "pg-err-tag";
      tag.textContent = "error";
      line.append(tag, document.createTextNode(text));
    } else {
      line.textContent = text;
    }
    out.appendChild(line);
    out.scrollTop = out.scrollHeight;
  }
  function clearConsole() {
    out.replaceChildren();
    lineCount = 0;
  }

  // -- editor chrome
  let errorLine = 0;
  function renderGutter() {
    const n = editor.value.split("\n").length;
    let html = "";
    for (let i = 1; i <= n; i++) html += i === errorLine ? `<span class="pg-gutter-err">${i}</span>\n` : `${i}\n`;
    gutter.innerHTML = html;
    gutter.scrollTop = editor.scrollTop;
  }
  function markError(msg) {
    const m = /playground\.lua:(\d+):/.exec(msg);
    errorLine = m ? Number(m[1]) : 0;
    renderGutter();
  }
  function renderSaved() {
    const edited = editor.value !== originals[current];
    saved.textContent = edited ? "Edited · saved in this browser" : "Unmodified";
    saved.classList.toggle("text-copper", edited);
  }

  let saveTimer = 0;
  editor.addEventListener("input", () => {
    if (errorLine) errorLine = 0;
    renderGutter();
    clearTimeout(saveTimer);
    saveTimer = setTimeout(() => {
      if (editor.value === originals[current]) store.del(`pg:src:${current}`);
      else store.set(`pg:src:${current}`, editor.value);
      renderSaved();
    }, 250);
  });
  editor.addEventListener("scroll", () => {
    gutter.scrollTop = editor.scrollTop;
  });

  // Tab inserts two spaces; Esc then Tab leaves the editor; Enter keeps indent.
  let escaped = false;
  function insert(text) {
    editor.focus();
    if (!document.execCommand || !document.execCommand("insertText", false, text)) {
      editor.setRangeText(text, editor.selectionStart, editor.selectionEnd, "end");
      editor.dispatchEvent(new Event("input"));
    }
  }
  editor.addEventListener("keydown", (e) => {
    if (e.key === "Escape") {
      escaped = true;
      return;
    }
    if (e.key === "Tab" && !escaped && !e.shiftKey && !e.ctrlKey && !e.metaKey && !e.altKey) {
      e.preventDefault();
      insert("  ");
    } else if (e.key === "Enter" && !e.shiftKey && !e.ctrlKey && !e.metaKey && !e.altKey) {
      const before = editor.value.slice(0, editor.selectionStart);
      const indent = /^[ \t]*/.exec(before.slice(before.lastIndexOf("\n") + 1))[0];
      if (indent) {
        e.preventDefault();
        insert("\n" + indent);
      }
    }
    escaped = false;
  });

  // -- run
  async function run() {
    if (running) return;
    running = true;
    const src = editor.value;
    clearConsole();
    markError("");
    status.textContent = "Running…";

    if (engine) {
      sandbox.replaceChildren(); // detach the old engine's DOM before closing it
      engine.global.close();
      engine = null;
    }
    sandbox.replaceChildren();

    try {
      const lua = await factory.createEngine({ functionTimeout: HANDLER_TIMEOUT_MS });
      engine = lua;
      const onError = (err) => {
        const msg = /timeout exceeded/.test(String(err && err.message))
          ? `stopped: event handler ran longer than ${HANDLER_TIMEOUT_MS / 1000} s (possible infinite loop)`
          : String((err && err.message) || err);
        log(msg, "err");
        markError(msg);
      };
      lua.global.set("dom", makeBridge({ root: sandbox, onError }));
      lua.global.set("__out", (s) => log(String(s)));
      await lua.doString(`package.path = "./?.lua;./?/init.lua"`);
      await lua.doFile(runnerPath);
      lua.global.set("__src", src);

      const t0 = performance.now();
      const err = await lua.doString("return playground.run()");
      const ms = performance.now() - t0;

      const version = lua.global.get("_VERSION");
      status.textContent = `${version} · ran in ${ms < 10 ? ms.toFixed(1) : Math.round(ms)} ms`;
      if (err) {
        log(err, "err");
        markError(err);
        status.textContent = `${version} · stopped with an error after ${Math.round(ms)} ms`;
      }
    } catch (e) {
      log(String((e && e.message) || e), "err");
      status.textContent = "Engine error";
    } finally {
      running = false;
    }

    if (!sandbox.childNodes.length) {
      const empty = document.createElement("p");
      empty.className = "pg-empty";
      empty.textContent = "Nothing mounted. This code writes to the console below.";
      sandbox.appendChild(empty);
    }
    if (!lineCount) log("(no output)", "info");
  }

  // -- presets
  async function load(id, { reset = false } = {}) {
    current = id;
    store.set("pg:preset", id);
    presetButtons.forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.pgPreset === id)));
    if (!(id in originals)) originals[id] = await fetchText(PRESETS[id].src);
    if (reset) store.del(`pg:src:${id}`);
    editor.value = store.get(`pg:src:${id}`) ?? originals[id];
    editor.scrollTop = 0;
    errorLine = 0;
    renderGutter();
    renderSaved();
    $("[data-pg-file]").textContent = PRESETS[id].src.replace(`${BASE}/`, "");
    await run();
  }

  presetButtons.forEach((b) =>
    b.addEventListener("click", () => {
      if (b.dataset.pgPreset !== current) load(b.dataset.pgPreset);
    }),
  );
  $("[data-pg-run]").addEventListener("click", run);
  $("[data-pg-reset]").addEventListener("click", () => load(current, { reset: true }));
  $("[data-pg-clear]").addEventListener("click", clearConsole);
  document.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && (e.ctrlKey || e.metaKey)) {
      e.preventDefault();
      run();
    }
  });

  editor.disabled = false;
  await load(current);
}

boot().catch(console.error);
