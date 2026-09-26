local h = require("h").h
local reactive = require("reactive")

local count, setCount = reactive.signal(0)

local root = h("div", { class = "flex flex-col gap-4 items-start" },
	h("div", { class = "label text-accent" }, "Counter · one Lua signal"),
	h("p", { class = "text-[38px] font-sans font-semibold text-ink tracking-[-.03em] leading-none" }, function()
		return "count = " .. count()
	end),
	h("div", { class = "flex flex-row gap-2 pt-2" },
		h("button", {
			class = "btn btn-sm btn-outline",
			onClick = function() setCount(count() - 1) end,
		}, "decrement"),
		h("button", {
			class = "btn btn-sm btn-ink",
			onClick = function() setCount(count() + 1) end,
		}, "increment"),
		h("button", {
			class = "btn btn-sm btn-ghost",
			onClick = function() setCount(0) end,
		}, "reset")
	),
	h("div", { class = "mono-sm pt-4 border-t border-rule w-full" }, function()
		return "signal updated " .. count() .. " time(s) this session"
	end)
)

dom.mount(root, "#app")
