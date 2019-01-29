-- let's not get too verbose here
local OA = OpenApoc
local GS = OpenApoc.GameState
local FW = OpenApoc.Framework

local function pickRandom(t)
	return t[GS.rng:randBoundsInclusive(1, #t)]
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

function math.clamp(v, max, min)
	return math.min(math.max(v, min), max)
end
function math.round(v)
	return math.floor(v + 0.5)
end


--print("OpenApoc table:")
--print("{")
--table.dump(OpenApoc, 1)
--print("}")

local oldNewGameHook = OpenApoc.hook.newGame
OA.hook.newGame = function()
	--call preexisting hook if any
	if oldNewGameHook then oldNewGameHook() end

	--seed the rng on game start
	if FW.Config.getBool("OpenApoc.NewFeature.SeedRng") == true then
		local seed = os.time()
		GS.rng:setState(seed, -seed)
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
		print("no buildings without bases!")
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


local oldUpdateEconomyHook = OpenApoc.hook.updateEconomy
OpenApoc.hook.updateEconomy = function()
	if oldUpdateEconomyHook then oldUpdateEconomyHook() end

	--this is based on Wongs guide
	local function updateEconomyInfo(eco, itemType)
		if eco.currentPrice == 0 then
			eco.currentPrice = eco.basePrice
		end

		local week = GS.gameTime:getWeek()
		if eco.weekAvailable > week then
			return false
		end

		local soldThisWeek = math.max(0, eco.currentStock - eco.lastStock)
		eco.lastStock = eco.currentStock

		--item manufactured by xcom use a different formula
		if itemType.manufacturer.id == GS.player.id then
			--update stock
			local r = GS.rng:randBoundsInclusive(0, 100)
			if r < 30 then
				eco.currentStock = eco.lastStock * 80 / 100
			elseif r < 60 then
				eco.currentStock = eco.lastStock * 66 / 100
			end

			if soldThisWeek > 2 * eco.maxStock then
				eco.currentPrice = math.round(eco.currentPrice * GS.rng:randBoundsReal(0.85, 0.95))
			elseif soldThisWeek > eco.maxStock then
				eco.currentPrice = math.round(eco.currentPrice * GS.rng:randBoundsReal(0.9, 0.95))
			elseif soldThisWeek > eco.maxStock/2 then
				eco.currentPrice = math.round(eco.currentPrice * GS.rng:randBoundsReal(0.95, 0.97))
			end
			eco.currentPrice = math.round(math.clamp(eco.currentPrice, eco.basePrice / 2, eco.basePrice))
		elseif eco.weekAvailable ~= 0 then
			local averageStock = math.round((eco.minStock + eco.maxStock)/2)
			eco.currentStock = math.clamp(GS.rng:randBoundsInclusive(0, averageStock + eco.lastStock), eco.minStock, eco.maxStock)
			if week > 1 then
				if eco.currentStock > averageStock then
					eco.currentPrice = eco.currentPrice * GS.rng:randBoundsReal(0.97, 1.0)
				elseif eco.currentStock < averageStock then
					eco.currentPrice = eco.currentPrice * GS.rng:randBoundsReal(1.0, 1.03)
				end
				eco.currentPrice = math.round(math.clamp(eco.currentPrice, eco.basePrice / 2, eco.basePrice * 2))
			end
		end
		return week ~= 1 and week == eco.weekAvailable
	end

	local newItems = {}
	for _, map in ipairs({GS.vehicle_types, GS.vehicle_equipment,
	                      GS.vehicle_ammo, GS.agent_equipment}) do
		for id, object in pairs(map) do
			--the actual economy info is in GameState.economy
			local eco = GS.economy[id]
			if eco then
				if updateEconomyInfo(eco, object) then
					table.insert(newItems, {id=id, object=object})
				end
			end
		end
	end
	if #newItems > 0 then
		print('NEW ITEMS:')
		for _, stateRef in pairs(newItems) do
			print(stateRef.id)
		end
	end
end
