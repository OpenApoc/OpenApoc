-- let's not get too verbose here
local OA = OpenApoc
local GS = OpenApoc.GameState
local FW = OpenApoc.Framework
local CFG = OpenApoc.Framework.Config

function pickRandom(t)
	return t[GS.rng:randBoundsInclusive(1, #t)]
end
function math.clamp(v, min, max)
	return math.min(math.max(v, min), max)
end
function math.round(v)
	return math.floor(v + 0.5)
end

-- useful for debugging purposes
-- note that this won't work for openapoc objects
-- since they are not actual tables
function table.dump(t, nesting)
	nesting = nesting or 0
	for k, v in pairs(t) do
		if type(v) == 'table' then
			print(string.rep("\t", nesting) .. tostring(k) .. " = {")
			dumpTable(v, nesting+1)
			print(string.rep("\t", nesting) .. "}")
		else
			print(string.rep("\t", nesting) .. tostring(k) .. " = " .. tostring(v))
		end
	end
end
--print("OpenApoc table:")
--print("{")
--table.dump(OpenApoc, 1)
--print("}")

dofile('scripts/update_economy.lua')

local oldUpdateEndOfWeekHook = OpenApoc.hook.updateEndOfWeek
OpenApoc.hook.updateEndOfWeek = function()
	if oldUpdateEndOfWeekHook then oldUpdateEndOfWeekHook() end

	updateEconomy()
end
