-- let's not get too verbose here
local OA = OpenApoc
local GS = OpenApoc.GameState
local FW = OpenApoc.Framework

FW.LogInfo ("On load script called")

local gamestateString = "submods/org.openapoc.base/difficulty" .. tostring(GS.difficulty)
FW.LogInfo("Loading difficulty patch" .. gamestateString)

GS.appendGameState(GS, gamestateString)

FW.LogInfo("Finished loading difficulty patch")
