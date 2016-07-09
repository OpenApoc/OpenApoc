PROJECT (OpenApoc_Tests CXX C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

add_executable(test_rect test_rect.cpp)
target_link_libraries(test_rect OpenApoc_Library OpenApoc_Framework)
target_compile_definitions(test_rect PRIVATE -DUNIT_TEST)
add_test(NAME test_rect COMMAND ${EXECUTABLE_OUTPUT_PATH}/test_rect)


set_property(TARGET test_rect PROPERTY CXX_STANDARD 11)
set_property(TARGET test_rect PROPERTY CXX_STANDARD_REQUIRED ON)
