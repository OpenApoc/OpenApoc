# project name, and type
PROJECT(OpenApoc_LuaGamestateSupportGen CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

find_package(Boost REQUIRED COMPONENTS program_options)

set (LUAGAMESTATESUPPORTGEN_SOURCE_FILES
	main.cpp ../serialization_xml.cpp)

source_group(luagamestatesupportgen\\sources FILES ${LUAGAMESTATESUPPORTGEN_SOURCE_FILES})

set (GAMESTATESERIALIZEGEN_HEADER_FILES
	../serialization_xml.h)

source_group(luagamestatesupportgen\\headers FILES ${LUAGAMESTATESUPPORTGEN_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LUAGAMESTATESUPPORTGEN_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LUAGAMESTATESUPPORTGEN_HEADER_FILES})

add_executable(OpenApoc_LuaGamestateSupportGen ${LUAGAMESTATESUPPORTGEN_SOURCE_FILES}
		${LUAGAMESTATESUPPORTGEN_HEADER_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_LuaGamestateSupportGen OpenApoc_LibPugixml
		${Boost_LIBRARIES})

target_include_directories(OpenApoc_LuaGamestateSupportGen PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(OpenApoc_LuaGamestateSupportGen PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_LuaGamestateSupportGen PUBLIC ..)
