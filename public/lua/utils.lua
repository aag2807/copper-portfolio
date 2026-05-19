local M = {}

function async(callback)
	return function(...)
		local co = coroutine.create(callback)
		local safe, result = coroutine.resume(co, ...)

		return Promise.create(function(resolve, reject)
			local function step()
				if coroutine.status(co) == "dead" then
					local send = safe and resolve or reject
					return send(result)
				end

				safe, result = coroutine.resume(co)

				if safe and result == Promise.resolve(result) then
					result:finally(step)
				else
					step()
				end
			end

			result:finally(step)
		end)
	end
end

M.async = async

return M
