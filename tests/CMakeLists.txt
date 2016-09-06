PROJECT (OpenApoc_Tests CXX C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set (TEST_LIST test_rect test_voxel test_tilemap test_rng test_images)

foreach(TEST ${TEST_LIST})
		add_executable(${TEST} ${TEST}.cpp)
		# Just add all libs?
		target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
				OpenApoc_GameState)
		target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
		add_test(NAME ${TEST} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST})
		set_property(TARGET ${TEST} PROPERTY CXX_STANDARD 11)
		set_property(TARGET ${TEST} PROPERTY CXX_STANDARD_REQUIRED ON)
endforeach()

# test_serialize requires args to needs to be slightly separate	
set(TEST test_serialize)
add_executable(${TEST} ${TEST}.cpp)
# Just add all libs?
target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
		OpenApoc_GameState)
target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
add_test(NAME ${TEST}_difficulty1 COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST}
		${CMAKE_SOURCE_DIR}/data/difficulty1_patched)
add_test(NAME ${TEST}_difficulty2 COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST}
		${CMAKE_SOURCE_DIR}/data/difficulty2_patched)
add_test(NAME ${TEST}_difficulty3 COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST}
		${CMAKE_SOURCE_DIR}/data/difficulty3_patched)
add_test(NAME ${TEST}_difficulty4 COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST}
		${CMAKE_SOURCE_DIR}/data/difficulty4_patched)
add_test(NAME ${TEST}_difficulty5 COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST}
		${CMAKE_SOURCE_DIR}/data/difficulty5_patched)

set_property(TARGET ${TEST} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${TEST} PROPERTY CXX_STANDARD_REQUIRED ON)
