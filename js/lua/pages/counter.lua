local h = require("h").h
local reactive = require("reactive")

local count, setCount = reactive.signal(0)

local root = h("div", { class = "flex flex-col gap-4 items-start" },
	h("div", { class = "text-[10px] font-mono uppercase tracking-widest text-emerald-500" }, "[0x01] counter // wasmoon-bound"),
	h("p", { class = "text-3xl font-mono font-bold text-slate-100 tracking-tighter" }, function()
		return "count = " .. count()
	end),
	h("div", { class = "flex flex-row gap-2 pt-2" },
		h("button", {
			class = "px-4 py-2 bg-slate-900 border border-slate-800 hover:border-emerald-500/40 text-slate-200 font-mono text-xs uppercase tracking-wider rounded transition-colors active:scale-[0.98]",
			onClick = function() setCount(count() - 1) end,
		}, "decrement"),
		h("button", {
			class = "px-4 py-2 bg-emerald-600 hover:bg-emerald-500 text-neutral-950 font-mono text-xs uppercase tracking-wider font-bold rounded transition-colors active:scale-[0.98]",
			onClick = function() setCount(count() + 1) end,
		}, "increment"),
		h("button", {
			class = "px-4 py-2 bg-slate-900 border border-slate-800 hover:border-emerald-500/40 text-slate-400 font-mono text-xs uppercase tracking-wider rounded transition-colors active:scale-[0.98]",
			onClick = function() setCount(0) end,
		}, "reset")
	),
	h("div", { class = "text-[10px] font-mono text-slate-600 pt-4 border-t border-slate-800 w-full" }, function()
		return "signal updated " .. count() .. " time(s) this session"
	end)
)

dom.mount(root, "#app")
