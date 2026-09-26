// Lua 5.4 tokenizer + highlighter for the /playground editor. One pass over
// the source, no regex backtracking over the whole input, never throws:
// unterminated strings stop at the end of the line, unterminated long
// strings/comments run to the end of the input.

const KEYWORDS = new Set(
  "and break do else elseif end for function goto if in local not or repeat return then until while".split(" "),
);
const CONSTANTS = new Set(["nil", "true", "false"]);
const BUILTINS = new Set(
  (
    "print require pairs ipairs next tostring tonumber type error pcall xpcall assert " +
    "setmetatable getmetatable rawget rawset rawequal rawlen select load loadfile dofile " +
    "collectgarbage string table math os io coroutine debug utf8 package _G _ENV _VERSION self dom"
  ).split(" "),
);

// token type -> CSS class (null = plain text)
const CLASS = {
  keyword: "k",
  constant: "v",
  number: "n",
  string: "s",
  escape: "e",
  comment: "c",
  function: "f",
  method: "m",
  builtin: "b",
  operator: "o",
  punct: null,
  ident: null,
  space: null,
  other: null,
};

const IDENT = /[A-Za-z_][A-Za-z0-9_]*/y;
const NUMBER = /0[xX][0-9a-fA-F]*(?:\.[0-9a-fA-F]*)?(?:[pP][+-]?\d+)?|(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?/y;
const SPACE = /[ \t\r\n\f\v]+/y;
const OPS2 = new Set(["..", "==", "~=", "<=", ">=", "//", "<<", ">>", "::"]);
const OPERATOR_CHARS = "+-*/%^#&~|<>=";

const isDigit = (c) => c >= 48 && c <= 57;
const isIdentStart = (c) => (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c === 95;

// "[" followed by n "=" and "[" -> n, else -1
function longLevel(src, i) {
  let j = i + 1;
  while (src.charCodeAt(j) === 61) j++; // =
  return src.charCodeAt(j) === 91 ? j - i - 1 : -1; // [
}

function longEnd(src, open, level) {
  const close = "]" + "=".repeat(level) + "]";
  const at = src.indexOf(close, open + level + 2);
  return at < 0 ? src.length : at + close.length;
}

// Length of an escape sequence starting at the backslash at i.
function escapeEnd(src, i, n) {
  const c = src[i + 1];
  if (c === undefined) return n;
  if (c === "z") {
    SPACE.lastIndex = i + 2;
    return SPACE.test(src) ? SPACE.lastIndex : i + 2;
  }
  if (c === "\r" && src[i + 2] === "\n") return i + 3;
  if (c === "x") {
    let j = i + 2;
    while (j < i + 4 && /[0-9a-fA-F]/.test(src[j] || "")) j++;
    return j;
  }
  if (c === "u" && src[i + 2] === "{") {
    const close = src.indexOf("}", i + 3);
    const nl = src.indexOf("\n", i + 3);
    if (close >= 0 && (nl < 0 || close < nl)) return close + 1;
    return i + 2;
  }
  if (isDigit(src.charCodeAt(i + 1))) {
    let j = i + 1;
    while (j < i + 4 && isDigit(src.charCodeAt(j))) j++;
    return j;
  }
  return i + 2;
}

// Calls emit(type, start, end) for each token, in order, covering the input.
function scan(src, emit) {
  const n = src.length;
  let i = 0;
  let prev = ""; // text of the last significant (non-space, non-comment) token

  while (i < n) {
    const c = src.charCodeAt(i);
    const ch = src[i];

    // whitespace
    if (c === 32 || c === 9 || c === 10 || c === 13 || c === 12 || c === 11) {
      SPACE.lastIndex = i;
      SPACE.test(src);
      emit("space", i, SPACE.lastIndex);
      i = SPACE.lastIndex;
      continue;
    }

    // comments: --[==[ long ]==] or -- to end of line
    if (ch === "-" && src[i + 1] === "-") {
      let end;
      const level = src[i + 2] === "[" ? longLevel(src, i + 2) : -1;
      if (level >= 0) end = longEnd(src, i + 2, level);
      else {
        end = src.indexOf("\n", i);
        if (end < 0) end = n;
      }
      emit("comment", i, end);
      i = end;
      continue;
    }

    // long strings [==[ ... ]==]
    if (ch === "[") {
      const level = longLevel(src, i);
      if (level >= 0) {
        const end = longEnd(src, i, level);
        emit("string", i, end);
        i = end;
        prev = "str";
        continue;
      }
    }

    // quoted strings, escapes emitted separately
    if (ch === '"' || ch === "'") {
      let seg = i;
      let j = i + 1;
      while (j < n) {
        const d = src[j];
        if (d === ch) {
          j++;
          break;
        }
        if (d === "\n") break; // unterminated: stop at end of line
        if (d === "\\") {
          if (j > seg) emit("string", seg, j);
          const e = escapeEnd(src, j, n);
          emit("escape", j, e);
          seg = j = e;
          continue;
        }
        j++;
      }
      if (j > seg) emit("string", seg, j);
      i = j;
      prev = "str";
      continue;
    }

    // numbers
    if (isDigit(c) || (ch === "." && isDigit(src.charCodeAt(i + 1)))) {
      NUMBER.lastIndex = i;
      NUMBER.test(src);
      let end = NUMBER.lastIndex;
      // swallow a malformed tail (3rd, 0x1g) so it doesn't read as an identifier
      while (end < n && /[A-Za-z0-9_]/.test(src[end])) end++;
      emit("number", i, end);
      i = end;
      prev = "num";
      continue;
    }

    // names
    if (isIdentStart(c)) {
      IDENT.lastIndex = i;
      IDENT.test(src);
      const end = IDENT.lastIndex;
      const word = src.slice(i, end);
      let type = "ident";
      if (KEYWORDS.has(word)) type = "keyword";
      else if (CONSTANTS.has(word)) type = "constant";
      else if (prev === ":") type = "method";
      else {
        // a call: name(  name "s"  name 's'  name {  name [[  (same line)
        let k = end;
        while (src[k] === " " || src[k] === "\t") k++;
        const nx = src[k];
        const called =
          nx === "(" || nx === '"' || nx === "'" || nx === "{" || (nx === "[" && longLevel(src, k) >= 0);
        if (prev !== "." && BUILTINS.has(word)) type = "builtin";
        else if (called) type = "function";
      }
      emit(type, i, end);
      i = end;
      prev = type === "keyword" ? word : "name";
      continue;
    }

    // operators and punctuation
    const two = src.slice(i, i + 2);
    const three = two === ".." ? src.slice(i, i + 3) : "";
    if (three === "...") {
      emit("operator", i, i + 3);
      i += 3;
      prev = three;
      continue;
    }
    if (OPS2.has(two)) {
      emit(two === "::" ? "punct" : "operator", i, i + 2);
      i += 2;
      prev = two;
      continue;
    }
    if (OPERATOR_CHARS.includes(ch)) {
      emit("operator", i, i + 1);
      i++;
      prev = ch;
      continue;
    }
    if ("()[]{},;.:".includes(ch)) {
      emit("punct", i, i + 1);
      i++;
      prev = ch;
      continue;
    }

    // anything else (non-ASCII, @, $, !, ...): a run of it, plain
    let j = i + 1;
    while (j < n && src.charCodeAt(j) > 126) j++;
    emit("other", i, j);
    i = j;
    prev = "other";
  }
}

function tokenize(src) {
  const out = [];
  scan(src, (type, s, e) => out.push({ type, text: src.slice(s, e) }));
  return out;
}

const HTML_CHARS = /[&<>]/;
const escapeHtml = (s) =>
  HTML_CHARS.test(s) ? s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") : s;

// HTML for a <pre>: consecutive plain tokens are merged into one text run.
function highlight(src) {
  let html = "";
  let plainFrom = -1;
  scan(src, (type, s, e) => {
    const cls = CLASS[type];
    if (!cls) {
      if (plainFrom < 0) plainFrom = s;
      return;
    }
    if (plainFrom >= 0) {
      html += escapeHtml(src.slice(plainFrom, s));
      plainFrom = -1;
    }
    html += `<span class="${cls}">${escapeHtml(src.slice(s, e))}</span>`;
  });
  if (plainFrom >= 0) html += escapeHtml(src.slice(plainFrom));
  return html;
}

// The same, split into one HTML string per source line (a token spanning lines,
// such as a long comment, is closed at each newline and reopened on the next
// line), so the editor can replace only the lines that changed.
function highlightLines(src) {
  const lines = [];
  let line = "";
  const add = (cls, text) => {
    let from = 0;
    for (let nl = text.indexOf("\n"); ; nl = text.indexOf("\n", from)) {
      const part = nl < 0 ? text.slice(from) : text.slice(from, nl);
      if (part) line += cls ? `<span class="${cls}">${escapeHtml(part)}</span>` : escapeHtml(part);
      if (nl < 0) return;
      lines.push(line);
      line = "";
      from = nl + 1;
    }
  };
  let plainFrom = -1;
  scan(src, (type, s, e) => {
    const cls = CLASS[type];
    if (!cls) {
      if (plainFrom < 0) plainFrom = s;
      return;
    }
    if (plainFrom >= 0) {
      add(null, src.slice(plainFrom, s));
      plainFrom = -1;
    }
    add(cls, src.slice(s, e));
  });
  if (plainFrom >= 0) add(null, src.slice(plainFrom));
  lines.push(line);
  return lines;
}

module.exports = { tokenize, highlight, highlightLines };
