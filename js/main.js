const { LuaFactory } = require("wasmoon");

const BASE = "/static/lua";

async function boot() {
  const mod = document.body.dataset.luaModule;
  if (!mod) return;

  const manifest = await fetch(`${BASE}/manifest.json`).then((r) => r.json());

  const factory = new LuaFactory();

  const library = await Promise.all(
    manifest.library.map(async (path) => [
      path,
      await fetch(`${BASE}/${path}`).then((r) => r.text()),
    ]),
  );

  for (const [path, content] of library) {
    await factory.mountFile(path, content);
  }

  const pagePath = `${mod}.lua`;
  const pageSrc = await fetch(`${BASE}/${pagePath}`).then((r) => r.text());
  await factory.mountFile(pagePath, pageSrc);

  const lua = await factory.createEngine();
  lua.global.set("dom", makeBridge());

  await lua.doString(`package.path = "./?.lua;./?/init.lua"`);
  await lua.doFile(pagePath);
}

function makeBridge() {
  return {
    create: (tag) => document.createElement(tag),
    text: (s) => document.createTextNode(s),
    append: (p, c) => p.appendChild(c),
    setAttr: (el, k, v) => el.setAttribute(k, v),
    setText: (n, t) => {
      n.nodeValue = t;
    },
    on: (el, evt, fn) => el.addEventListener(evt, fn),
    mount: (el, sel) => document.querySelector(sel).appendChild(el),
    query: (sel) => document.querySelector(sel),
  };
}

boot().catch(console.error);
