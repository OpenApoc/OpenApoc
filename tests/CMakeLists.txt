PROJECT (OpenApoc_Tests CXX C)

set (TEST_LIST test_rect test_voxel test_tilemap test_rng test_images
		test_unicode test_colour test_backtrace test_strings
		test_gravlift_lineoffire)

foreach(TEST ${TEST_LIST})
		add_executable(${TEST} ${TEST}.cpp)
		# Just add all libs?
		target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
				OpenApoc_GameState)
		target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
		add_test(NAME ${TEST} COMMAND ${TEST}
				--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)
endforeach()

# test_serialize requires args so needs to be slightly separate
set(TEST test_serialize)
add_executable(${TEST} ${TEST}.cpp)
# Just add all libs?
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST}_difficulty0 COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)

# test_lab_assignment requires args (same as test_serialize)
set(TEST test_lab_assignment)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST} COMMAND ${TEST}
		${CMAKE_SOURCE_DIR}/data/mods/base/data/submods/org.openapoc.base/difficulty0
		${CMAKE_SOURCE_DIR}/data/mods/base/base_gamestate
		--Framework.CD=${CD_PATH} --Framework.Data=${CMAKE_SOURCE_DIR}/data)

# test_gravlift_lof and test_gravlift_ai_battle are manual investigation
# harnesses for issue #1330 (grav-lift line-of-fire/sight). They boot a real
# battle in one of the base game's grav-lift tilesets to exercise the real
# voxel/AI code paths, which needs the fully extracted tileset/map/gamestate
# data CI's minimal cd.iso does not provide. They are deliberately not
# add_test()-ed; build and run them by hand (see the header comment in each
# .cpp for arguments).
set(TEST test_gravlift_lof)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)

set(TEST test_gravlift_ai_battle)
add_executable(${TEST} ${TEST}.cpp)
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
