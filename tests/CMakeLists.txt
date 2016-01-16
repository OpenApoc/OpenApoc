find_package(Boost REQUIRED COMPONENTS locale)

add_executable(test_rect test_rect.cpp
		${CMAKE_SOURCE_DIR}/library/strings.cpp
		${CMAKE_SOURCE_DIR}/framework/logger.cpp)
target_link_libraries(test_rect ${Boost_LIBRARIES})
target_include_directories(test_rect SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_compile_definitions(test_rect PRIVATE -DUNIT_TEST)
target_link_libraries(test_rect ${FRAMEWORK_LIBRARIES})
add_test(NAME test_rect COMMAND ${EXECUTABLE_OUTPUT_PATH}/test_rect)
set_property(TARGET test_rect PROPERTY CXX_STANDARD 11)
set_property(TARGET test_rect PROPERTY CXX_STANDARD_REQUIRED ON)
