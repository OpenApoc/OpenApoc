-- let's not get too verbose here
local OA = OpenApoc
local GS = OpenApoc.GameState
local FW = OpenApoc.Framework

FW.LogInfo ("On load script called")

local gamestateDifficultyString = "submods/org.openapoc.base/difficulty" .. tostring(GS.difficulty)
FW.LogInfo("Loading difficulty patch" .. gamestateDifficultyString)

GS.appendGameState(GS, gamestateDifficultyString)

FW.LogInfo("Finished loading difficulty patch")

local gamestateLanguageString = "submods/org.openapoc.base/language/" .. tostring(FW.getLanguageCountry())
FW.LogInfo("Loading language patch" .. gamestateLanguageString)
local success = GS.appendGameState(GS, gamestateLanguageString)
if success == true then
  FW.LogInfo("Finished loading language patch")
else
  FW.LogInfo("No country-specific language patch found");
  gamestateLanguageString = "submods/org.openapoc.base/language/" .. tostring(FW.getLanguage())
  FW.LogInfo("Loading language patch" .. gamestateLanguageString)
  success = GS.appendGameState(GS, gamestateLanguageString)
  if success == true then
    FW.LogInfo("Finished loading langauge patch")
  else
    FW.LogInfo("No langauge patch found")
  end

end


