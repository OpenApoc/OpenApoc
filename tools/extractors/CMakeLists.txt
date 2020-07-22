# project name, and type
PROJECT(OpenApoc_DataExtractor CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (DATAEXTRACTOR_SOURCE_FILES
	common/tacp.cpp
	common/strtab.cpp
	common/ufo2p.cpp
	extract_doodads.cpp
	extract_agent_equipment.cpp
	extract_agent_types.cpp
	extract_battlescape_map.cpp
	extract_battlescape_map_parts.cpp
	extract_battle_shared_resources.cpp
	extract_facilities.cpp
	extract_organisations.cpp
	extract_vehicle_equipment.cpp
	extract_city_map.cpp
	extract_economy.cpp
	extract_buildings.cpp
	extract_vehicles.cpp
	extract_city_scenery.cpp
	extract_base_layouts.cpp
	extract_bulletsprites.cpp
	extractors.cpp
	extract_research.cpp
	extract_unit_image_packs.cpp
	extract_unit_animation_packs.cpp
	extract_unit_animation_pack_unit.cpp
	extract_unit_animation_pack_bsk.cpp
	extract_unit_animation_pack_psi.cpp
	extract_unit_animation_pack_mega.cpp
	extract_unit_animation_pack_egg.cpp
	extract_unit_animation_pack_gun.cpp
	extract_unit_animation_pack_multi.cpp
	extract_unit_animation_pack_civ.cpp
	extract_unit_animation_pack_hyper.cpp
	extract_unit_animation_pack_spitter.cpp
	extract_unit_animation_pack_micro.cpp
	extract_unit_animation_pack_q.cpp
	extract_unit_animation_pack_popper.cpp
	extract_unit_animation_pack_chrysalis.cpp
	main.cpp)

source_group(dataextractor\\sources FILES ${DATAEXTRACTOR_SOURCE_FILES})

set (DATAEXTRACTOR_HEADER_FILES
	common/aequipment.h
	common/audio.h
	common/baselayout.h
	common/doodads.h
	common/economy.h
	common/facilities.h
	common/organisations.h
	common/ufopaedia.h
	common/vequipment.h
	common/datachunk.h
	common/strtab.h
	common/canonstring.h
	common/ufo2p.h
	common/scenerytile.h
	common/vehicle.h
	common/research.h
	common/bulletsprite.h
	common/building.h
	common/battlemap.h
	common/tacp.h
	extractors.h)

source_group(dataextractor\\headers FILES ${DATAEXTRACTOR_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${DATAEXTRACTOR_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${DATAEXTRACTOR_HEADER_FILES})

add_executable(OpenApoc_DataExtractor ${DATAEXTRACTOR_SOURCE_FILES}
		${DATAEXTRACTOR_HEADER_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_DataExtractor OpenApoc_Library)
target_link_libraries(OpenApoc_DataExtractor OpenApoc_Framework)
target_link_libraries(OpenApoc_DataExtractor OpenApoc_GameState)

option(EXTRACT_DATA "Run the DataExtractor during build" ON)

function (add_extractor TARGET_LIST EXTRACTOR_NAME)
	foreach (OUTPUT_NAME ${ARGN})
			list(APPEND EXTRACTOR_OUTPUT "${CMAKE_SOURCE_DIR}/${OUTPUT_NAME}")
	endforeach()
	add_custom_command(OUTPUT
		${EXTRACTOR_OUTPUT}
		COMMAND
		$<TARGET_FILE:OpenApoc_DataExtractor>
		--Framework.CD=${CD_PATH}
		--Framework.Data=${CMAKE_SOURCE_DIR}/data
		--Extractor.extract=${EXTRACTOR_NAME}
		DEPENDS OpenApoc_DataExtractor
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		VERBATIM)
	set(${TARGET_LIST} ${${TARGET_LIST}} ${EXTRACTOR_OUTPUT} PARENT_SCOPE)
endfunction(add_extractor)

SET(EXTRACTOR_TARGET_LIST "")

add_extractor(EXTRACTOR_TARGET_LIST "all" "data/mods/base/base_gamestate")

option(EXTRACT_DATA "Run the data extractor as part of the default target" ON)

if (${EXTRACT_DATA})
add_custom_target(extract-data ALL DEPENDS ${EXTRACTOR_TARGET_LIST})
else()
add_custom_target(extract-data DEPENDS ${EXTRACTOR_TARGET_LIST})
endif()

if (APPLE)
add_custom_command(TARGET extract-data POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/bin/OpenApoc.app/Contents/Resources/data)
endif()
