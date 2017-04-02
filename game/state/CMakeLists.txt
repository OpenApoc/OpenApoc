# project name, and type
PROJECT(OpenApoc_GameState CXX C)

include(cotire)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (GAMESTATE_SOURCE_FILES
	aequipment.cpp
	agent.cpp
	gameevent.cpp
	gamestate.cpp
	gamestate_serialize.cpp
	gamestate_serialize_generated.cpp
	gametime.cpp
	message.cpp
	organisation.cpp
	research.cpp
	savemanager.cpp
	ufopaedia.cpp
	base/base.cpp
	base/facility.cpp
	battle/ai/aidecision.cpp
	battle/ai/tacticalai.cpp
	battle/ai/tacticalaivanilla.cpp
	battle/ai/unitai.cpp
	battle/ai/unitaibehavior.cpp
	battle/ai/unitaidefault.cpp
	battle/ai/unitaihardcore.cpp
	battle/ai/unitaihelper.cpp
	battle/ai/unitailowmorale.cpp
	battle/ai/unitaivanilla.cpp
	battle/battle.cpp
	battle/battledoor.cpp
	battle/battleexplosion.cpp
	battle/battleforces.cpp
	battle/battlehazard.cpp
	battle/battleitem.cpp
	battle/battlemap.cpp
	battle/battlemappart.cpp
	battle/battlemappart_type.cpp
	battle/battlemapsector.cpp
	battle/battlescanner.cpp
	battle/battleunit.cpp
	battle/battleunitanimationpack.cpp
	battle/battleunitimagepack.cpp
	battle/battleunitmission.cpp
	battle/battlemaptileset.cpp
	city/baselayout.cpp
	city/building.cpp
	city/city.cpp
	city/doodad.cpp
	city/projectile.cpp
	city/scenery.cpp
	city/vehicle.cpp
	city/vehiclemission.cpp
	city/vequipment.cpp
	rules/aequipment_rules.cpp
	rules/damage_rules.cpp
	rules/doodad_type.cpp
	rules/facility_type.cpp
	rules/scenery_tile_type.cpp
	rules/ufo_growth.cpp
	rules/ufo_incursion.cpp
	rules/vammo_rules.cpp
	rules/vehicle_type_rules.cpp
	rules/vequipment_rules.cpp
	tileview/collision.cpp
	tileview/pathfinding.cpp
	tileview/tile.cpp
	tileview/tileobject.cpp
	tileview/tileobject_battlehazard.cpp
	tileview/tileobject_battlemappart.cpp
	tileview/tileobject_battleitem.cpp
	tileview/tileobject_battleunit.cpp
	tileview/tileobject_doodad.cpp
	tileview/tileobject_projectile.cpp
	tileview/tileobject_scenery.cpp
	tileview/tileobject_shadow.cpp
	tileview/tileobject_vehicle.cpp)
source_group(gamestate\\sources FILES ${GAMESTATE_SOURCE_FILES})
set (GAMESTATE_HEADER_FILES
	aequipment.h
	agent.h
	gameevent.h
	gameevent_types.h
	gamestate.h
	gamestate_serialize_generated.h
	gametime.h
	gametime_facet.h
	message.h
	organisation.h
	research.h
	savemanager.h
	stateobject.h
	ufopaedia.h
	base/base.h
	base/facility.h
	battle/ai/aidecision.h
	battle/ai/aitype.h
	battle/ai/tacticalai.h
	battle/ai/tacticalaivanilla.h
	battle/ai/unitai.h
	battle/ai/unitaibehavior.h
	battle/ai/unitaidefault.h
	battle/ai/unitaihardcore.h
	battle/ai/unitaihelper.h
	battle/ai/unitailowmorale.h
	battle/ai/unitaivanilla.h
	battle/battle.h
	battle/battlecommonimagelist.h
	battle/battlecommonsamplelist.h
	battle/battledoor.h
	battle/battleexplosion.h
	battle/battleforces.h
	battle/battlehazard.h
	battle/battleitem.h
	battle/battlemap.h
	battle/battlemappart.h
	battle/battlemappart_type.h
	battle/battlemapsector.h
	battle/battlemaptileset.h
	battle/battlescanner.h
	battle/battleunit.h
	battle/battleunitanimationpack.h
	battle/battleunitimagepack.h
	battle/battleunitmission.h
	city/baselayout.h
	city/building.h
	city/city.h
	city/doodad.h
	city/projectile.h
	city/scenery.h
	city/vehicle.h
	city/vehiclemission.h
	city/vequipment.h
	rules/aequipment_type.h
	rules/damage.h
	rules/doodad_type.h
	rules/facility_type.h
	rules/scenery_tile_type.h
	rules/ufo_growth.h
	rules/ufo_incursion.h
	rules/vammo_type.h
	rules/vehicle_type.h
	rules/vequipment_type.h
	tileview/collision.h
	tileview/tile.h
	tileview/tileobject.h
	tileview/tileobject_battlehazard.h
	tileview/tileobject_battlemappart.h
	tileview/tileobject_battleitem.h
	tileview/tileobject_battleunit.h
	tileview/tileobject_doodad.h
	tileview/tileobject_projectile.h
	tileview/tileobject_scenery.h
	tileview/tileobject_shadow.h
	tileview/tileobject_vehicle.h)
source_group(gamestate\\headers FILES ${GAMESTATE_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${GAMESTATE_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${GAMESTATE_HEADER_FILES})

add_library(OpenApoc_GameState STATIC ${GAMESTATE_SOURCE_FILES}
		${GAMESTATE_HEADER_FILES})

target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Library)
target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Framework)

# This needs to be public as gamestate_serialize_generated.h needs to be in
# the path
target_include_directories(OpenApoc_GameState PUBLIC ${CMAKE_BINARY_DIR})

set_property(TARGET OpenApoc_GameState PROPERTY CXX_STANDARD 11)

add_custom_command(OUTPUT
		gamestate_serialize_generated.h
		gamestate_serialize_generated.cpp
		COMMAND
		${CMAKE_BINARY_DIR}/bin/OpenApoc_GamestateSerializeGen
		-x ${CMAKE_SOURCE_DIR}/game/state/gamestate_serialize.xml
		-h gamestate_serialize_generated.h
		-o gamestate_serialize_generated.cpp
		DEPENDS
		${CMAKE_BINARY_DIR}/bin/OpenApoc_GamestateSerializeGen
		${CMAKE_SOURCE_DIR}/game/state/gamestate_serialize.xml)

set_target_properties(OpenApoc_GameState PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT
	"gamestate_pch.h")

if(ENABLE_COTIRE)
	cotire(OpenApoc_GameState)
endif()
