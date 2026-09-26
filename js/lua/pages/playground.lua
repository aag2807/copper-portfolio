-- pages/playground.lua: loaded into every fresh engine before your code runs.
-- It routes print() to the console pane and guards the top-level chunk with an
-- instruction limit. Event handlers are guarded separately, by a wall-clock
-- timeout on each JS -> Lua call.
local LIMIT = 20000000 -- VM instructions for the top-level chunk
local RUNNER = debug.getinfo(1, "S").source
local STOP = "stopped: instruction limit reached (possible infinite loop)"

local out = __out
__out = nil

function print(...)
	local parts = {}
	for i = 1, select("#", ...) do
		parts[i] = tostring((select(i, ...)))
	end
	out(table.concat(parts, "\t"))
end

-- Once tripped, the hook fires on every instruction, so a pcall inside the
-- loop can't swallow the stop and keep spinning. It never fires in this file.
local function stop()
	local caller = debug.getinfo(2, "S")
	if caller and caller.source == RUNNER then return end
	error(STOP, 2)
end

local function trip()
	debug.sethook(stop, "", 1)
	error(STOP, 2)
end

playground = {}

-- Runs the source the page left in __src. Returns nil, or the error message.
function playground.run()
	local src = __src
	__src = nil
	local chunk, err = load(src, "@playground.lua", "t")
	if not chunk then return err end
	debug.sethook(trip, "", LIMIT)
	local ok, e = pcall(chunk)
	debug.sethook()
	if not ok then return tostring(e) end
end
