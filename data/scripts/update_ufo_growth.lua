local OA = OpenApoc
local GS = OpenApoc.GameState

function updateUfoGrowth()
	local week = GS.gameTime:getWeek()
	local ufo_growth = {id = 'UFO_GROWTH_' .. tostring(week)}
	ufo_growth.object = GS.ufo_growth_lists[ufo_growth.id]
	if not ufo_growth.object then
		ufo_growth.id = 'UFO_GROWTH_DEFAULT'
		ufo_growth.object = GS.ufo_growth_lists[ufo_growth.id]
	end
	local limit = {id = 'UFO_GROWTH_LIMIT'}
	limit.object = GS.ufo_growth_lists[limit.id]

	if ufo_growth.object then
		local city = {id = 'CITYMAP_ALIEN'}
		city.object = GS.cities[city.id]

		--set a list of limits for vehicle types
		local vehicleLimits = {}
		--increase value by limit
		for _, vt in ipairs(limit.object.vehicleTypeList) do
			vehicleLimits[vt.first] = (vehicleLimits[vt.first] or 0) + vt.second
		end
		--substract existing vehicles
		for vehicle_id, vehicle_object in pairs(GS.vehicles) do
			if vehicle_object.owner == 'ORG_ALIEN' and vehicle_object.city == 'CITYMAP_ALIEN' then
				vehicleLimits[vehicle_object.type.id] = (vehicleLimits[vehicle_object.type.id] or 0) - 1
			end
		end

		for _, vt in ipairs(ufo_growth.object.vehicleTypeList) do
			local vt_object = GS.vehicle_types[vt.first]
			if vt_object then
				local toAdd = math.min(vt.second, vehicleLimits[vt.first])
				for i=1, toAdd do
					local pos = {
						x = GS.rng:randBoundsExclusive(20, 120),
						y = GS.rng:randBoundsExclusive(20, 120),
						z = city.object.size.z-1
					}
					city.object:placeVehicleAtPosition(vt.first, 'ORG_ALIEN', pos)
				end
			end
		end
	end
end
