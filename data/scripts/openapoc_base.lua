-- let's not get too verbose here
local OA = OpenApoc
local GS = OpenApoc.GameState
local FW = OpenApoc.Framework
local CFG = OpenApoc.Framework.Config

function pickRandom(t)
	return t[GS.rng:randBoundsInclusive(1, #t)]
end
function math.clamp(v, max, min)
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

dofile('data/scripts/update_economy.lua')
dofile('data/scripts/update_ufo_growth.lua')

local oldNewGameHook = OpenApoc.hook.newGame
OA.hook.newGame = function()
	--call preexisting hook if any
	if oldNewGameHook then oldNewGameHook() end

	--seed the rng on game start
	if CFG.getBool('OpenApoc.NewFeature.SeedRng') == true then
		local seed = os.time()
		GS.rng:seed(seed)
	end
end

local oldApplyModsHook = OpenApoc.hook.applyMods
OA.hook.applyMods = function()
	if oldApplyModsHook then oldApplyModsHook() end

	GS.vehicle_types['VEHICLETYPE_GRIFFON_AFV'].type = CFG.getBool('OpenApoc.Mod.ATVTank') and OA.enum.VehicleType.Type.ATV or OA.enum.VehicleType.Type.Road
	GS.vehicle_types['VEHICLETYPE_WOLFHOUND_APC'].type = CFG.getBool('OpenApoc.Mod.ATVAPC') and OA.enum.VehicleType.Type.ATV or OA.enum.VehicleType.Type.Road

	if CFG.getBool('OpenApoc.Mod.BSKLauncherSound') then
		GS.agent_equipment['AEQUIPMENTTYPE_BRAINSUCKER_LAUNCHER'].fire_sfx = 'RAWSOUND:xcom3/rawsound/tactical/weapons/sucklaun.raw:22050'
	else
		GS.agent_equipment['AEQUIPMENTTYPE_BRAINSUCKER_LAUNCHER'].fire_sfx = 'RAWSOUND:xcom3/rawsound/tactical/weapons/powers.raw:22050'
	end

	local crashVehicles = CFG.getBool('OpenApoc.Mod.CrashingVehicles')
	for vt_id, vt_object in pairs(GS.vehicle_types) do
		if vt_object.type ~= OA.enum.VehicleType.Type.UFO then
			vt_object.crash_health = crashVehicles and e.second.health//7 or 0
		end
	end
end

local oldNewGamePostInitHook = OpenApoc.hook.newGamePostInit
OA.hook.newGamePostInit = function()
	if oldNewGamePostInitHook then oldNewGamePostInitHook() end

	local buildingsWithoutBases = {}

	local city = {id = 'CITYMAP_HUMAN'}
	city.object = GS.cities['CITYMAP_HUMAN']

	for k, v in pairs(city.object.buildings) do
		if not v.base_layout.object then
			--staterefs returned by OpenApoc are just tables
			--with id and object fields
			stateRef = {id = k, object = v}
			table.insert(buildingsWithoutBases, stateRef)
		end
	end
	--nowehere to spawn, return here
	if #buildingsWithoutBases == 0 then
		FW.LogWarning('no buildings without bases!')
		return
	end

	--add developers
	local names = {'Filmboy', 'Flacko', 'Istrebitel', 'Jarskih', 'JonnyH', 'Makus', 'PmProg', 'Redv', 'SupSuper'}
	local agent_type = 'AGENTTYPE_X-COM_AGENT_HUMAN'

	--look for organisations that have hirable
	--agent types matching ours
	local valid_organisations = {}
	for k,v in pairs(GS.organisations) do
		if v.hirableAgentTypes[agent_type] then
			table.insert(valid_organisations, {id=k, object=v})
		end
	end

	for i, name in ipairs(names) do
		--pick a random organisation for this agent
		local organisation = pickRandom(valid_organisations)

		--OpenApoc only accepts StateRefs as strings
		local agent = GS.agent_generator:createAgent(organisation.id, agent_type)

		--pick a random building to spawn from
		local building = pickRandom(buildingsWithoutBases)

		--put the agent in the building
		agent.object.homeBuilding = building.id
		agent.object.city = 'CITYMAP_HUMAN'
		agent.object:enterBuilding(building.id)

		agent.object.name = name
		agent.object.initial_stats.morale = 100
		agent.object.initial_stats.bravery = 100
		agent.object.modified_stats.morale = 100
		agent.object.modified_stats.bravery = 100
		agent.object.current_stats.morale = 100
		agent.object.current_stats.bravery = 100
	end
end


local oldUpdateEndOfWeekHook = OpenApoc.hook.updateEndOfWeek
OpenApoc.hook.updateEndOfWeek = function()
	if oldUpdateEndOfWeekHook then oldUpdateEndOfWeekHook() end

	updateUfoGrowth()
	updateEconomy()
end
