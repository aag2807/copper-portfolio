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
	class = "field flex-1 !py-2.5",
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
			class = "bg-paper border border-rule px-3 py-2 text-[13px] text-ink flex items-center gap-2 font-mono group",
		},
			h("span", { class = "text-accent text-[10px]" }, "▸"),
			h("span", { class = "flex-1" }, v),
			h("button", {
				class = "opacity-50 group-hover:opacity-100 px-2 py-0.5 text-copper border border-transparent hover:border-copper text-[10px] font-mono uppercase tracking-wider transition-all",
				title = "remove item",
				onClick = function() removeAt(i) end,
			}, "del")
		))
	end
end)

local root = h("div", { class = "flex flex-col gap-4" },
	h("div", { class = "label text-accent" }, "[0x02] todos // mutable signal"),
	h("div", { class = "flex flex-row gap-2" },
		input,
		h("button", {
			class = "btn btn-sm btn-ink",
			onClick = addItem,
		}, "append")
	),
	list,
	h("div", { class = "mono-sm pt-3 border-t border-rule" }, function()
		return #items() .. " item(s) in list"
	end)
)

dom.mount(root, "#app")
