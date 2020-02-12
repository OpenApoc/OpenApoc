# project name, and type
PROJECT(OpenApoc_GamestateLocalisation CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (GAMESTATELOCALISATION_SOURCE_FILES
	main.cpp)

source_group(gamestatelocalisation\\sources FILES ${GAMESTATELOCALISATION_SOURCE_FILES})

set (GAMESTATELOCALISATION_HEADER_FILES
	)

source_group(gamestatelocalisation\\headers FILES ${GAMESTATELOCALISATION_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${GAMESTATELOCALISATION_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${GAMESTATELOCALISATION_HEADER_FILES})

add_executable(OpenApoc_GamestateLocalisation ${GAMESTATELOCALISATION_SOURCE_FILES}
		${GAMESTATELOCALISATION_HEADER_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_GamestateLocalisation OpenApoc_Library)
target_link_libraries(OpenApoc_GamestateLocalisation OpenApoc_Framework)
target_link_libraries(OpenApoc_GamestateLocalisation OpenApoc_GameState)

set(BASE_GAMESTATE_POT_FILE ${CMAKE_SOURCE_DIR}/data/languages/base_gamestate.pot)
set(BASE_GAMESTATE_SOURCE_FILE ${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate)

add_custom_command(
OUTPUT ${BASE_GAMESTATE_POT_FILE}
COMMAND OpenApoc_GamestateLocalisation ${BASE_GAMESTATE_SOURCE_FILE} --Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data> ${BASE_GAMESTATE_POT_FILE}
DEPENDS ${BASE_GAMESTATE_SOURCE_FILE})

add_custom_target(extract-gamestate-translations
		DEPENDS ${BASE_GAMESTATE_POT_FILE})
