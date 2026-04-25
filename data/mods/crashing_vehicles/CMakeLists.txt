# Pack the mod's source XML into a single zip file that the engine loads via
# modinfo.xml's <statepath>. Uses 'cmake -E tar --format=zip' so no external
# zip tool or library dependency is required.

set(MOD_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MOD_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/crashing_vehicles_gamestate)

set(MOD_SOURCES
	${MOD_SRC_DIR}/gamestate.xml
	${MOD_SRC_DIR}/gamestate/vehicle_types.xml)

add_custom_command(OUTPUT ${MOD_OUTPUT}
	COMMAND ${CMAKE_COMMAND} -E tar cf ${MOD_OUTPUT} --format=zip
		gamestate.xml gamestate
	DEPENDS ${MOD_SOURCES}
	WORKING_DIRECTORY ${MOD_SRC_DIR}
	COMMENT "Packing crashing_vehicles mod"
	VERBATIM)

add_custom_target(mod-crashing-vehicles ALL DEPENDS ${MOD_OUTPUT})
