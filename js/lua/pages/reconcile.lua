-- pages/reconcile.lua: match an internal ledger against a provider settlement
-- report, by reference, and explain every row that doesn't line up.
local h = require("h").h
local reactive = require("reactive")

-- @region reconcile
-- Money is integer cents end to end. In floating point 0.1 + 0.2 ~= 0.3, and a
-- reconciler that can drift by a cent is a reconciler nobody trusts.

local function money(cents)
	local sign = cents < 0 and "-" or ""
	local a = math.abs(cents)
	local whole = tostring(a // 100):reverse():gsub("(%d%d%d)", "%1,"):reverse():gsub("^,", "")
	return string.format("%s%s.%02d", sign, whole, a % 100)
end

-- "49.99" -> 4999. Parsed as text so no float ever touches the amount.
local function parse_money(s)
	s = s:gsub(",", "")
	local whole, frac = s:match("^%s*(%d+)%.?(%d?%d?)%s*$")
	if not whole then return nil end
	frac = (frac .. "00"):sub(1, 2)
	return tonumber(whole) * 100 + tonumber(frac)
end

-- Fictional provider pricing: 2.9% + 0.30, rounded half up, in cents.
local function fee(gross)
	return (gross * 29 + 500) // 1000 + 30
end

local function gap(d, currency)
	local a = math.abs(d)
	local size = a < 100 and string.format("%d cent%s", a, a == 1 and "" or "s")
		or string.format("%s %s", money(a), currency)
	return size .. (d > 0 and " more" or " less")
end

-- Group rows by reference; a reference seen twice is kept twice, never overwritten.
local function index(rows)
	local by = {}
	for _, r in ipairs(rows) do
		by[r.ref] = by[r.ref] or {}
		table.insert(by[r.ref], r)
	end
	return by
end

local function times(n)
	return n == 2 and "twice" or (n .. " times")
end

-- Pure: the same two inputs always give the same verdicts. Nothing here reads a
-- clock or mutates its inputs, so re-running a reconciliation is always safe.
local function reconcile(ledger, settlement)
	local L, S = index(ledger), index(settlement)
	local refs, seen = {}, {}
	for _, side in ipairs({ ledger, settlement }) do
		for _, r in ipairs(side) do
			if not seen[r.ref] then
				seen[r.ref] = true
				refs[#refs + 1] = r.ref
			end
		end
	end
	table.sort(refs)

	local results = {}
	for _, ref in ipairs(refs) do
		local l, s = L[ref], S[ref]
		local verdict, reason
		if not s then
			verdict = "Missing in settlement"
			reason = string.format("Charged %s %s on %s; the provider never settled it",
				money(l[1].amount), l[1].currency, l[1].date)
		elseif not l then
			verdict = "Missing in ledger"
			reason = string.format("Provider settled %s %s that we have no record of",
				money(s[1].amount), s[1].currency)
		elseif #s > 1 then
			verdict = "Duplicate in settlement"
			reason = string.format("Settled %s for %s %s: the customer may have been charged %s",
				times(#s), money(s[1].amount), s[1].currency, times(#s))
		elseif #l > 1 then
			verdict = "Duplicate in ledger"
			reason = string.format("Booked %s in our ledger, settled once", times(#l))
		else
			l, s = l[1], s[1]
			if l.currency ~= s.currency then
				verdict = "Currency mismatch"
				reason = string.format("Booked as %s %s, settled as %s %s",
					money(l.amount), l.currency, money(s.amount), s.currency)
			elseif l.amount ~= s.amount then
				verdict = "Amount mismatch"
				reason = string.format("Settled %s than the ledger: %s vs %s %s",
					gap(s.amount - l.amount, l.currency), money(s.amount), money(l.amount), l.currency)
			elseif l.status ~= s.status then
				verdict = "Status mismatch"
				reason = string.format("%s in our ledger, settled as %s by the provider",
					(l.status:gsub("^%l", string.upper)), s.status)
			else
				verdict = "Matched"
				reason = string.format("%s %s, %s on both sides", money(l.amount), l.currency, l.status)
			end
		end
		results[#results + 1] = { ref = ref, verdict = verdict, reason = reason, ok = verdict == "Matched" }
	end
	return results
end

-- Totals per currency: what the provider settled minus what we booked.
local function summarize(ledger, settlement, results)
	local sum = { matched = 0, exceptions = 0, fees = {}, diff = {} }
	for _, r in ipairs(results) do
		if r.ok then sum.matched = sum.matched + 1 else sum.exceptions = sum.exceptions + 1 end
	end
	for _, r in ipairs(ledger) do
		sum.diff[r.currency] = (sum.diff[r.currency] or 0) - r.amount
	end
	for _, r in ipairs(settlement) do
		sum.diff[r.currency] = (sum.diff[r.currency] or 0) + r.amount
		sum.fees[r.currency] = (sum.fees[r.currency] or 0) + fee(r.amount)
	end
	return sum
end
-- @endregion

-- Sample data: fictional orders, fictional provider. Six discrepancies planted.
local SAMPLE_LEDGER = {
	{ ref = "ORD-1001", date = "Sep 01", amount = 12000, currency = "USD", status = "captured" },
	{ ref = "ORD-1002", date = "Sep 01", amount = 5000,  currency = "USD", status = "captured" },
	{ ref = "ORD-1003", date = "Sep 02", amount = 8950,  currency = "USD", status = "captured" },
	{ ref = "ORD-1004", date = "Sep 02", amount = 25000, currency = "USD", status = "captured" },
	{ ref = "ORD-1005", date = "Sep 03", amount = 3200,  currency = "USD", status = "refunded" },
	{ ref = "ORD-1006", date = "Sep 03", amount = 15075, currency = "USD", status = "captured" },
	{ ref = "ORD-1007", date = "Sep 04", amount = 7500,  currency = "USD", status = "captured" },
	{ ref = "ORD-1008", date = "Sep 04", amount = 1999,  currency = "USD", status = "captured" },
	{ ref = "ORD-1009", date = "Sep 05", amount = 42000, currency = "USD", status = "captured" },
}
local SAMPLE_SETTLEMENT = {
	{ ref = "ORD-1001", amount = 12000, currency = "USD", status = "captured" },
	{ ref = "ORD-1002", amount = 4999,  currency = "USD", status = "captured" }, -- one cent short
	{ ref = "ORD-1003", amount = 8950,  currency = "USD", status = "captured" },
	{ ref = "ORD-1004", amount = 25000, currency = "EUR", status = "captured" }, -- wrong currency
	{ ref = "ORD-1005", amount = 3200,  currency = "USD", status = "captured" }, -- we refunded it
	{ ref = "ORD-1007", amount = 7500,  currency = "USD", status = "captured" },
	{ ref = "ORD-1007", amount = 7500,  currency = "USD", status = "captured" }, -- settled twice
	{ ref = "ORD-1008", amount = 1999,  currency = "USD", status = "captured" },
	{ ref = "ORD-1009", amount = 42000, currency = "USD", status = "captured" },
	{ ref = "ORD-1010", amount = 6400,  currency = "USD", status = "captured" }, -- not in ledger
}

local CURRENCIES = { "USD", "EUR" }
local STATUSES = { "captured", "refunded" }

local function copy(rows)
	local out = {}
	for i, r in ipairs(rows) do
		local c = {}
		for k, v in pairs(r) do c[k] = v end
		out[i] = c
	end
	return out
end

local data = { ledger = copy(SAMPLE_LEDGER), settlement = copy(SAMPLE_SETTLEMENT) }

-- rev: any value changed (re-match). shape: rows added/removed (re-render inputs).
local rev, setRev = reactive.signal(0)
local shape, setShape = reactive.signal(0)
local function changed() setRev(rev() + 1) end

local results = reactive.computed(function()
	rev()
	return reconcile(data.ledger, data.settlement)
end)

local function verdictOf(ref)
	for _, r in ipairs(results()) do
		if r.ref == ref then return r end
	end
end

-- ---- input tables --------------------------------------------------------

local function textInput(row, key, label, opts)
	opts = opts or {}
	local el = h("input", {
		class = "rc-in" .. (opts.num and " rc-num" or ""),
		value = opts.num and money(row[key]) or row[key],
		["aria-label"] = label,
		inputmode = opts.num and "decimal" or "text",
		spellcheck = "false",
		autocomplete = "off",
	})
	dom.on(el, "input", function(e)
		local raw = e.target.value
		if opts.num then
			local cents = parse_money(raw)
			dom.setAttr(el, "aria-invalid", cents and "false" or "true")
			if not cents then return end
			row[key] = cents
		else
			row[key] = raw:gsub("^%s+", ""):gsub("%s+$", "")
		end
		if opts.after then opts.after() end
		changed()
	end)
	return el
end

local function selectInput(row, key, label, choices)
	local el = h("select", { class = "rc-in rc-sel", ["aria-label"] = label })
	for _, c in ipairs(choices) do
		local o = h("option", { value = c }, c)
		if c == row[key] then dom.setAttr(o, "selected", "selected") end
		dom.append(el, o)
	end
	dom.on(el, "change", function(e)
		row[key] = e.target.value
		changed()
	end)
	return el
end

-- Row mark: follows the verdict for this row's reference as the match re-runs.
local function mark(row, gen, current)
	local glyph = dom.text("")
	local el = h("span", { class = "rc-mark" }, glyph)
	reactive.effect(function()
		if gen ~= current() then return end -- row belongs to a discarded render
		local r = verdictOf(row.ref)
		local ok = r and r.ok
		dom.setAttr(el, "class", "rc-mark " .. (ok and "rc-mark-ok" or "rc-mark-bad"))
		dom.setAttr(el, "title", r and r.verdict or "")
		dom.setText(glyph, ok and "✓" or "!")
	end)
	return el
end

local function removeButton(rows, row, label)
	return h("button", {
		class = "rc-x",
		title = "Remove row",
		["aria-label"] = "Remove " .. label,
		onClick = function()
			for i, r in ipairs(rows) do
				if r == row then table.remove(rows, i) break end
			end
			setShape(shape() + 1)
			changed()
		end,
	}, "×")
end

local function nextRef(rows)
	local max = 1000
	for _, r in ipairs(rows) do
		local n = tonumber(r.ref:match("(%d+)$"))
		if n and n > max then max = n end
	end
	return string.format("ORD-%d", max + 1)
end

local function tableShell(headers)
	local tr = h("tr", {})
	for _, hd in ipairs(headers) do
		dom.append(tr, h("th", { class = hd.num and "rc-num" or "" }, hd.label))
	end
	local body = h("tbody", {})
	return h("table", { class = "rc-table" }, h("thead", {}, tr), body), body
end

local function panel(title, rows, tbl, onAdd)
	return h("div", { class = "flex flex-col" },
		h("div", { class = "flex items-center justify-between gap-3 px-5 py-3.5 border-b border-rule" },
			h("span", { class = "label" }, title),
			h("span", { class = "mono-sm" }, function()
				shape()
				return "Sample data · " .. #rows() .. " rows"
			end)
		),
		h("div", { class = "overflow-x-auto" }, tbl),
		h("div", { class = "px-5 py-3 border-t border-rule mt-auto" },
			h("button", { class = "rc-add", onClick = onAdd }, "+ Add row")
		)
	)
end

local ledgerTable, ledgerBody = tableShell({
	{ label = "" }, { label = "Reference" }, { label = "Date" }, { label = "Amount", num = true },
	{ label = "Currency" }, { label = "Status" }, { label = "" },
})
local settleTable, settleBody = tableShell({
	{ label = "" }, { label = "Reference" }, { label = "Gross", num = true }, { label = "Fee", num = true },
	{ label = "Net", num = true }, { label = "Currency" }, { label = "Status" }, { label = "" },
})

reactive.effect(function()
	local gen = shape()
	dom.clear(ledgerBody)
	for _, row in ipairs(data.ledger) do
		dom.append(ledgerBody, h("tr", {},
			h("td", { class = "rc-mark-cell" }, mark(row, gen, shape)),
			h("td", {}, textInput(row, "ref", "Ledger reference")),
			h("td", { class = "text-muted whitespace-nowrap" }, row.date),
			h("td", {}, textInput(row, "amount", "Ledger amount for " .. row.ref, { num = true })),
			h("td", {}, selectInput(row, "currency", "Ledger currency for " .. row.ref, CURRENCIES)),
			h("td", {}, selectInput(row, "status", "Ledger status for " .. row.ref, STATUSES)),
			h("td", { class = "rc-x-cell" }, removeButton(data.ledger, row, row.ref))
		))
	end
	dom.clear(settleBody)
	for _, row in ipairs(data.settlement) do
		local feeText, netText = dom.text(money(fee(row.amount))), dom.text(money(row.amount - fee(row.amount)))
		local function refresh()
			dom.setText(feeText, money(fee(row.amount)))
			dom.setText(netText, money(row.amount - fee(row.amount)))
		end
		dom.append(settleBody, h("tr", {},
			h("td", { class = "rc-mark-cell" }, mark(row, gen, shape)),
			h("td", {}, textInput(row, "ref", "Settlement reference")),
			h("td", {}, textInput(row, "amount", "Settled gross for " .. row.ref, { num = true, after = refresh })),
			h("td", { class = "rc-num text-muted" }, feeText),
			h("td", { class = "rc-num" }, netText),
			h("td", {}, selectInput(row, "currency", "Settlement currency for " .. row.ref, CURRENCIES)),
			h("td", {}, selectInput(row, "status", "Settlement status for " .. row.ref, STATUSES)),
			h("td", { class = "rc-x-cell" }, removeButton(data.settlement, row, row.ref))
		))
	end
end)

local function addRow(rows, extra)
	local row = { ref = nextRef(rows), amount = 0, currency = "USD", status = "captured" }
	for k, v in pairs(extra or {}) do row[k] = v end
	rows[#rows + 1] = row
	setShape(shape() + 1)
	changed()
end

-- ---- summary + results ---------------------------------------------------

local function stat(label, valueFn, cls)
	return h("div", { class = "px-5 py-4 flex flex-col gap-1.5" },
		h("span", { class = "label" }, label),
		h("span", { class = cls or "rc-stat" }, valueFn)
	)
end

local function perCurrency(map, signed)
	local keys = {}
	for k in pairs(map) do keys[#keys + 1] = k end
	table.sort(keys)
	local parts = {}
	for _, k in ipairs(keys) do
		local v = map[k]
		if not signed or v ~= 0 or #keys == 1 then
			parts[#parts + 1] = (signed and v > 0 and "+" or "") .. money(v) .. " " .. k
		end
	end
	return #parts > 0 and table.concat(parts, " · ") or ("0.00")
end

local summary = reactive.computed(function()
	return summarize(data.ledger, data.settlement, results())
end)

local summaryStrip = h("div", { class = "rule-grid grid-cols-2 lg:grid-cols-4" },
	stat("Matched", function() return summary().matched end, "rc-stat text-accent"),
	stat("Exceptions", function() return summary().exceptions end, "rc-stat text-copper"),
	stat("Settled minus booked", function() return perCurrency(summary().diff, true) end, "rc-stat-sm"),
	stat("Provider fees", function() return perCurrency(summary().fees, false) end, "rc-stat-sm")
)

local resultsBody = h("tbody", {})
local previous = {}
reactive.effect(function()
	local rs = results()
	dom.clear(resultsBody)
	local now = {}
	for _, r in ipairs(rs) do
		now[r.ref] = r.verdict
		local flipped = previous[r.ref] and previous[r.ref] ~= r.verdict
		dom.append(resultsBody, h("tr", { class = flipped and "rc-flip" or "" },
			h("td", { class = "whitespace-nowrap" }, r.ref),
			h("td", {}, h("span", { class = "rc-badge " .. (r.ok and "rc-ok" or "rc-bad") }, r.verdict)),
			h("td", { class = "rc-reason" }, r.reason)
		))
	end
	previous = now
end)

local resultsTable = h("table", { class = "rc-table rc-results" },
	h("thead", {}, h("tr", {},
		h("th", {}, "Reference"), h("th", {}, "Verdict"), h("th", {}, "Reason")
	)),
	resultsBody
)

local root = h("div", { class = "flex flex-col gap-6" },
	summaryStrip,
	h("div", { class = "rule-grid grid-cols-1 xl:grid-cols-2" },
		panel("Our ledger", function() return data.ledger end, ledgerTable, function()
			addRow(data.ledger, { date = "Sep 06" })
		end),
		panel("Provider settlement report", function() return data.settlement end, settleTable, function()
			addRow(data.settlement)
		end)
	),
	h("div", { class = "rule-grid grid-cols-1" },
		h("div", { class = "flex flex-col" },
			h("div", { class = "flex flex-wrap items-center justify-between gap-3 px-5 py-3.5 border-b border-rule" },
				h("span", { class = "label" }, "Results · one row per reference"),
				h("button", {
					class = "btn btn-sm btn-ghost !py-2",
					onClick = function()
						data.ledger, data.settlement = copy(SAMPLE_LEDGER), copy(SAMPLE_SETTLEMENT)
						previous = {}
						setShape(shape() + 1)
						changed()
					end,
				}, "Reset sample data")
			),
			h("div", { class = "overflow-x-auto" }, resultsTable)
		)
	)
)

dom.clear(dom.query("#app")) -- drop the "booting" placeholder
dom.mount(root, "#app")
