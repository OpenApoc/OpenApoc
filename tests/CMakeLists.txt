PROJECT (OpenApoc_Tests CXX C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set (TEST_LIST test_rect test_voxel test_tilemap)

foreach(TEST ${TEST_LIST})
		add_executable(${TEST} ${TEST}.cpp)
		# Just add all libs?
		target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework
				OpenApoc_Forms OpenApoc_GameState OpenApoc_GameUI)
		target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
		add_test(NAME ${TEST} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST})
		set_property(TARGET ${TEST} PROPERTY CXX_STANDARD 11)
		set_property(TARGET ${TEST} PROPERTY CXX_STANDARD_REQUIRED ON)
endforeach()
