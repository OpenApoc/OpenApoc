# project name, and type
PROJECT(OpenApoc_DataExtractor CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set (DATAEXTRACTOR_SOURCE_FILES
	common/tacp.cpp
	common/strtab.cpp
	common/ufo2p.cpp
	extract_agent_equipment.cpp
	extract_agent_types.cpp
	extract_battlescape_map.cpp
	extract_battlescape_map_parts.cpp
	extract_battle_shared_resources.cpp
	extract_facilities.cpp
	extract_organisations.cpp
	extract_vehicle_equipment.cpp
	extract_city_map.cpp
	extract_buildings.cpp
	extract_vehicles.cpp
	extract_city_scenery.cpp
	extract_base_layouts.cpp
	extract_bulletsprites.cpp
	extractors.cpp
	extract_research.cpp
	extract_unit_animation_packs.cpp
	extract_unit_image_packs.cpp
	main.cpp)

source_group(dataextractor\\sources FILES ${DATAEXTRACTOR_SOURCE_FILES})

set (DATAEXTRACTOR_HEADER_FILES
	common/aequipment.h
	common/audio.h
	common/baselayout.h
	common/facilities.h
	common/organisations.h
	common/ufopaedia.h
	common/vequipment.h
	common/datachunk.h
	common/strtab.h
	common/canonstring.h
	common/crc32.h
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
target_link_libraries(OpenApoc_DataExtractor OpenApoc_Forms)
target_link_libraries(OpenApoc_DataExtractor OpenApoc_GameState)

set_property(TARGET OpenApoc_DataExtractor PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_DataExtractor PROPERTY CXX_STANDARD_REQUIRED ON)
