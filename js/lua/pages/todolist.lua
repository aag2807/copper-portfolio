local h = require("h").h
local reactive = require("reactive")

local items, setItems = reactive.signal({ "buy milk", "write more lua" })
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

local input = h("input", {
	class = "border border-slate-400 rounded px-2 py-1",
	placeholder = "new todo...",
	value = draft,
})
dom.on(input, "input", function(e) setDraft(e.target.value) end)

local list = h("ul", { class = "list-disc ml-6" })
reactive.effect(function()
	list.innerHTML = ""
	for _, v in ipairs(items()) do
		dom.append(list, h("li", {}, v))
	end
end)

local root = h("div", { class = "flex flex-col gap-3 py-6" },
	h("h2", { class = "text-2xl text-slate-800" }, "Todos"),
	h("div", { class = "flex flex-row gap-2" },
		input,
		h("button", {
			class = "px-3 py-1 bg-slate-300 rounded",
			onClick = addItem,
		}, "Add")
	),
	list
)

dom.mount(root, "#app")
