local M = {}
local current_effect = nil
local effects = {} -- strong-reference root so effects survive Lua GC

function M.signal(initial)
	local value = initial
	local subs = {}

	local function get()
		if current_effect then
			subs[current_effect] = true
		end
		return value
	end

	local function set(v)
		if v == value then
			return
		end
		value = v
		-- snapshot first: an effect that runs here may subscribe new effects
		local queue = {}
		for fn in pairs(subs) do
			queue[#queue + 1] = fn
		end
		for _, fn in ipairs(queue) do
			fn()
		end
	end

	return get, set
end

function M.effect(fn)
	local function run()
		local prev = current_effect
		current_effect = run
		fn()
		current_effect = prev
	end
	effects[#effects + 1] = run
	run()
	return run
end

function M.computed(fn)
	local get, set = M.signal(nil)
	M.effect(function()
		set(fn())
	end)
	return get
end

return M
