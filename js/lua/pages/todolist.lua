local h = require("h").h
local reactive = require("reactive")

local items, setItems = reactive.signal({ "wire up real mail relay", "swap gamedev slot images" })
local draft, setDraft = reactive.signal("")

local function addItem()
	local d = draft()
	if d == "" then return end
	local copy = {}
	for i, v in ipairs(items()) do copy[i] = v end
	copy[#copy + 1] = d
	setItems(copy)
	setDraft("")
end

local function removeAt(idx)
	local copy = {}
	for j, w in ipairs(items()) do
		if j ~= idx then copy[#copy + 1] = w end
	end
	setItems(copy)
end

local input = h("input", {
	class = "flex-1 bg-[#0A0A0B] border border-slate-800 focus:border-emerald-500/50 focus:outline-none rounded px-3 py-2 text-sm text-slate-100 font-mono placeholder:text-slate-700",
	placeholder = "new todo...",
})
dom.on(input, "input", function(e) setDraft(e.target.value) end)
dom.on(input, "keydown", function(e) if e.key == "Enter" then addItem() end end)
-- Reactive sync of the .value PROPERTY (not the attribute) so clearing draft
-- after add actually empties the live input.
reactive.effect(function() input.value = draft() end)

local list = h("ul", { class = "flex flex-col gap-1.5" })
reactive.effect(function()
	list.innerHTML = ""
	for i, v in ipairs(items()) do
		dom.append(list, h("li", {
			class = "bg-[#0A0A0B] border border-slate-800 rounded px-3 py-2 text-sm text-slate-200 flex items-center gap-2 font-mono group",
		},
			h("span", { class = "text-emerald-500 text-[10px]" }, "▸"),
			h("span", { class = "flex-1" }, v),
			h("button", {
				class = "opacity-40 group-hover:opacity-100 px-2 py-0.5 text-rose-400 hover:text-rose-300 hover:bg-rose-950/30 border border-transparent hover:border-rose-900 rounded text-[11px] font-mono uppercase tracking-wider transition-all active:scale-[0.95]",
				title = "remove item",
				onClick = function() removeAt(i) end,
			}, "del")
		))
	end
end)

local root = h("div", { class = "flex flex-col gap-4" },
	h("div", { class = "text-[10px] font-mono uppercase tracking-widest text-emerald-500" }, "[0x02] todos // mutable signal"),
	h("div", { class = "flex flex-row gap-2" },
		input,
		h("button", {
			class = "px-4 py-2 bg-emerald-600 hover:bg-emerald-500 text-neutral-950 font-mono text-xs uppercase tracking-wider font-bold rounded transition-colors active:scale-[0.98]",
			onClick = addItem,
		}, "append")
	),
	list,
	h("div", { class = "text-[10px] font-mono text-slate-600 pt-3 border-t border-slate-800" }, function()
		return #items() .. " item(s) in list"
	end)
)

dom.mount(root, "#app")
