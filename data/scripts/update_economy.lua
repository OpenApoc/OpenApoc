local OA = OpenApoc
local GS = OpenApoc.GameState

function updateEconomy()
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
