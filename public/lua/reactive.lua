local M = {}
local current_effect = nil

function M.signal(initial)
	local value = initial
	local subs = setmetatable({}, { __mode = "k" }) -- weak keys

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
		for fn in pairs(subs) do
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
