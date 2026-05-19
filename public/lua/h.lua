local reactive = require("reactive")
local M = {}

function M.h(tag, props, ...)
	local el = dom.create(tag)

	if props then
		for k, v in pairs(props) do
			if type(v) == "function" and k:sub(1, 2) == "on" then
				dom.on(el, k:sub(3):lower(), v) -- event handler
			elseif type(v) == "function" then
				reactive.effect(function()
					dom.setAttr(el, k, tostring(v()))
				end)
			else
				dom.setAttr(el, k, tostring(v))
			end
		end
	end

	for _, c in ipairs({ ... }) do
		if type(c) == "function" then
			local node = dom.text("")
			dom.append(el, node)
			reactive.effect(function()
				dom.setText(node, tostring(c()))
			end)
		elseif type(c) == "string" or type(c) == "number" then
			dom.append(el, dom.text(tostring(c)))
		elseif c then
			dom.append(el, c)
		end
	end

	return el
end

return M
