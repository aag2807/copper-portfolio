-- Plain Lua, no DOM: everything goes to the console.
print(_VERSION .. ", compiled to WebAssembly, running in this tab")

-- Lua 5.4 has real integers next to floats
print("7 // 2 =", 7 // 2, "   7 / 2 =", 7 / 2)
print("math.type(1), math.type(1.0):", math.type(1), math.type(1.0))

-- tables are the only data structure; sort takes a comparator
local langs = {
	{ name = "Go", year = 2009 },
	{ name = "C", year = 1972 },
	{ name = "Lua", year = 1993 },
	{ name = "C#", year = 2000 },
	{ name = "TypeScript", year = 2012 },
}
table.sort(langs, function(a, b) return a.year < b.year end)
for i, l in ipairs(langs) do
	print(string.format("%d. %-11s %d", i, l.name, l.year))
end

-- closures keep their own state
local function counter()
	local n = 0
	return function() n = n + 1; return n end
end
local next_id = counter()
print("ids:", next_id(), next_id(), next_id())

-- patterns, not regex
local words = {}
for w in ("hello from a hand-written C server"):gmatch("%a+") do
	words[#words + 1] = w:upper()
end
print(table.concat(words, " "))
