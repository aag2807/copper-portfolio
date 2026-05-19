local h = require("h").h
local reactive = require("reactive")

local count, setCount = reactive.signal(0)

local root = h("div", { class = "flex flex-col gap-3 py-6" },
	h("h2", { class = "text-2xl text-slate-800" }, "Counter"),
	h("p", { class = "text-slate-700" }, function() return "Count: " .. count() end),
	h("div", { class = "flex flex-row gap-2" },
		h("button", {
			class = "px-3 py-1 bg-slate-300 rounded",
			onClick = function() setCount(count() - 1) end,
		}, "-"),
		h("button", {
			class = "px-3 py-1 bg-slate-300 rounded",
			onClick = function() setCount(count() + 1) end,
		}, "+")
	)
)

dom.mount(root, "#app")
