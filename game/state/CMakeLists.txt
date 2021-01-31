# project name, and type
PROJECT(OpenApoc_GameState CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (WIN32)
	find_package(Boost REQUIRED COMPONENTS date_time)
else()
	find_package(Boost REQUIRED)
endif()

set (GAMESTATE_SOURCE_FILES
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
	battle/battlemappart.cpp
	battle/battlescanner.cpp
	battle/battleunit.cpp
	battle/battleunitmission.cpp

	city/agentmission.cpp
	city/base.cpp
	city/building.cpp
	city/city.cpp
	city/economyinfo.cpp
	city/facility.cpp
	city/research.cpp
	city/scenery.cpp
	city/vehicle.cpp
	city/vehiclemission.cpp
	city/vequipment.cpp

	rules/battle/battlemap.cpp
	rules/battle/battlemapsector.cpp
	rules/battle/battlemapparttype.cpp
	rules/battle/battleunitanimationpack.cpp
	rules/battle/battleunitimagepack.cpp
	rules/battle/battlemaptileset.cpp
	rules/battle/damage.cpp

	rules/city/baselayout.cpp
	rules/city/facilitytype.cpp
	rules/city/scenerytiletype.cpp
	rules/city/ufogrowth.cpp
	rules/city/ufomissionpreference.cpp
	rules/city/ufoincursion.cpp
	rules/city/ufopaedia.cpp
	rules/city/vammotype.cpp
	rules/city/vehicletype.cpp
	rules/city/vequipmenttype.cpp

	rules/aequipmenttype.cpp
	rules/agenttype.cpp
	rules/doodadtype.cpp
	rules/supportedmappart.cpp

	shared/aequipment.cpp
	shared/agent.cpp
	shared/doodad.cpp
	shared/organisation.cpp
	shared/projectile.cpp

	gameevent.cpp
	gamestate.cpp
	gamestate_serialize.cpp
	gamestate_serialize_generated.cpp
	gametime.cpp
	luagamestate.cpp
	luagamestate_support.cpp
	luagamestate_support_generated.cpp
	message.cpp
	savemanager.cpp

	tilemap/collision.cpp
	tilemap/pathfinding.cpp
	tilemap/tile.cpp
	tilemap/tilemap.cpp
	tilemap/tileobject.cpp
	tilemap/tileobject_battlehazard.cpp
	tilemap/tileobject_battlemappart.cpp
	tilemap/tileobject_battleitem.cpp
	tilemap/tileobject_battleunit.cpp
	tilemap/tileobject_doodad.cpp
	tilemap/tileobject_projectile.cpp
	tilemap/tileobject_scenery.cpp
	tilemap/tileobject_shadow.cpp
	tilemap/tileobject_vehicle.cpp)
source_group(gamestate\\sources FILES ${GAMESTATE_SOURCE_FILES})
set (GAMESTATE_HEADER_FILES
	battle/ai/aidecision.h
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
	battle/battledoor.h
	battle/battleexplosion.h
	battle/battleforces.h
	battle/battlehazard.h
	battle/battleitem.h
	battle/battlemappart.h
	battle/battlescanner.h
	battle/battleunit.h
	battle/battleunitmission.h

	city/agentmission.h
	city/base.h
	city/building.h
	city/city.h
	city/economyinfo.h
	city/facility.h
	city/research.h
	city/scenery.h
	city/vehicle.h
	city/vehiclemission.h
	city/vequipment.h

	rules/battle/battlecommonimagelist.h
	rules/battle/battlecommonsamplelist.h
	rules/battle/battlemap.h
	rules/battle/battlemapsector.h
	rules/battle/battlemapparttype.h
	rules/battle/battleunitanimationpack.h
	rules/battle/battleunitimagepack.h
	rules/battle/battlemaptileset.h
	rules/battle/damage.h

	rules/city/baselayout.h
	rules/city/citycommonimagelist.h
	rules/city/citycommonsamplelist.h
	rules/city/facilitytype.h
	rules/city/scenerytiletype.h
	rules/city/ufogrowth.h
	rules/city/ufomissionpreference.h
	rules/city/ufoincursion.h
	rules/city/ufopaedia.h
	rules/city/vammotype.h
	rules/city/vehicletype.h
	rules/city/vequipmenttype.h

	rules/aequipmenttype.h
	rules/agenttype.h
	rules/doodadtype.h
	rules/supportedmappart.h

	shared/aequipment.h
	shared/agent.h
	shared/doodad.h
	shared/equipment.h
	shared/organisation.h
	shared/projectile.h

	gameevent.h
	gameeventtypes.h
	gamestate.h
	gamestate_serialize.h
	gamestate_serialize_generated.h
	gametime.h
	gametime_facet.h
	luagamestate.h
	luagamestate_support.h
	luagamestate_support_generated.h
	message.h
	savemanager.h
	stateobject.h

	tilemap/collision.h
	tilemap/tile.h
	tilemap/tilemap.h
	tilemap/tileobject.h
	tilemap/tileobject_battlehazard.h
	tilemap/tileobject_battlemappart.h
	tilemap/tileobject_battleitem.h
	tilemap/tileobject_battleunit.h
	tilemap/tileobject_doodad.h
	tilemap/tileobject_projectile.h
	tilemap/tileobject_scenery.h
	tilemap/tileobject_shadow.h
	tilemap/tileobject_vehicle.h)
source_group(gamestate\\headers FILES ${GAMESTATE_HEADER_FILES})

if (MSVC)
	set_source_files_properties(gamestate_serialize_generated.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

list(APPEND ALL_SOURCE_FILES ${GAMESTATE_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${GAMESTATE_HEADER_FILES})

add_library(OpenApoc_GameState STATIC ${GAMESTATE_SOURCE_FILES}
		${GAMESTATE_HEADER_FILES})

target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Library)
target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Framework)

target_link_libraries(OpenApoc_GameState PUBLIC ${Boost_LIBRARIES}
		Threads::Threads)
if (WIN32)
	target_link_libraries(OpenApoc_GameState PUBLIC bcrypt)
endif()

target_link_libraries(OpenApoc_GameState PUBLIC OpenApoc_Lua)

# This needs to be public as gamestate_serialize_generated.h needs to be in
# the path
target_include_directories(OpenApoc_GameState PUBLIC ${CMAKE_BINARY_DIR})

add_custom_command(OUTPUT
		gamestate_serialize_generated.h
		gamestate_serialize_generated.cpp
		COMMAND
		$<TARGET_FILE:OpenApoc_GamestateSerializeGen>
		-x ${CMAKE_SOURCE_DIR}/game/state/gamestate_serialize.xml
		-h gamestate_serialize_generated.h
		-o gamestate_serialize_generated.cpp
		DEPENDS
		OpenApoc_GamestateSerializeGen
		${CMAKE_SOURCE_DIR}/game/state/gamestate_serialize.xml)

add_custom_command(OUTPUT
		luagamestate_support_generated.h
		luagamestate_support_generated.cpp
		COMMAND
		$<TARGET_FILE:OpenApoc_LuaGamestateSupportGen>
		-x ${CMAKE_SOURCE_DIR}/game/state/gamestate_serialize.xml
		-h luagamestate_support_generated.h
		-o luagamestate_support_generated.cpp
		DEPENDS
		OpenApoc_LuaGamestateSupportGen
		${CMAKE_SOURCE_DIR}/game/state/gamestate_serialize.xml)

add_custom_target(generated-source
		DEPENDS
		gamestate_serialize_generated.h gamestate_serialize_generated.cpp
		luagamestate_support_generated.h luagamestate_support_generated.cpp)
