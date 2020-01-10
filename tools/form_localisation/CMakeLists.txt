# project name, and type
PROJECT(OpenApoc_FormLocalisation CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set (FORMLOCALISATION_SOURCE_FILES
	main.cpp)

source_group(formlocalisation\\sources FILES ${FORMLOCALISATION_SOURCE_FILES})

list(APPEND ALL_SOURCE_FILES ${FORMLOCALISATION_SOURCE_FILES})

add_executable(OpenApoc_FormLocalisation ${FORMLOCALISATION_SOURCE_FILES}
		${FORMLOCALISATION_HEADER_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_FormLocalisation OpenApoc_LibPugixml)

target_include_directories(OpenApoc_FormLocalisation PUBLIC ${CMAKE_SOURCE_DIR})
