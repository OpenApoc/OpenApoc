# project name, and type
PROJECT(OpenApoc_GameState CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set (GAMESTATE_SOURCE_FILES
	agent.cpp
	base/base.cpp
	base/facility.cpp
	city/baselayout.cpp
	city/building.cpp
	city/city.cpp
	city/doodad.cpp
	city/projectile.cpp
	city/scenery.cpp
	city/vehicle.cpp
	city/vehiclemission.cpp
	city/vequipment.cpp
	gamestate.cpp
	gamestate_serialize.cpp
	gametime.cpp
	organisation.cpp
	research.cpp
	rules/doodad_type.cpp
	rules/facility_type.cpp
	rules/scenery_tile_type.cpp
	rules/ufo_growth.cpp
	rules/ufo_incursion.cpp
	rules/vehicle_type_rules.cpp
	rules/vequipment_rules.cpp
	savemanager.cpp
	tileview/tile.cpp
	tileview/tileobject.cpp
	tileview/tileobject_doodad.cpp
	tileview/tileobject_projectile.cpp
	tileview/tileobject_scenery.cpp
	tileview/tileobject_shadow.cpp
	tileview/tileobject_vehicle.cpp
	tileview/voxel.cpp
	ufopaedia.cpp)
source_group(gamestate\\sources FILES ${GAMESTATE_SOURCE_FILES})
set (GAMESTATE_HEADER_FILES
	agent.h
	base/base.h
	base/facility.h
	city/baselayout.h
	city/building.h
	city/city.h
	city/doodad.h
	city/projectile.h
	city/scenery.h
	city/vehicle.h
	city/vehiclemission.h
	city/vequipment.h
	gamestate.h
	gametime.h
	organisation.h
	research.h
	rules/doodad_type.h
	rules/facility_type.h
	rules/resource_aliases.h
	rules/scenery_tile_type.h
	rules/ufo_growth.h
	rules/ufo_incursion.h
	rules/vehicle_type.h
	rules/vequipment_type.h
	savemanager.h
	stateobject.h
	tileview/tile.h
	tileview/tileobject.h
	tileview/tileobject_doodad.h
	tileview/tileobject_projectile.h
	tileview/tileobject_scenery.h
	tileview/tileobject_shadow.h
	tileview/tileobject_vehicle.h
	tileview/voxel.h
	ufopaedia.h)
source_group(gamestate\\headers FILES ${GAMESTATE_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${GAMESTATE_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${GAMESTATE_HEADER_FILES})

add_library(OpenApoc_GameState STATIC ${GAMESTATE_SOURCE_FILES}
		${GAMESTATE_HEADER_FILES})

target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Library)
target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Framework)

set_property(TARGET OpenApoc_GameState PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_GameState PROPERTY CXX_STANDARD_REQUIRED ON)



