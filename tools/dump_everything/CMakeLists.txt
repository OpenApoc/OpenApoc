# project name, and type
PROJECT(OpenApoc_DumpEverything CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set (DUMPEVERYTHING_SOURCE_FILES
	dump_everything.cpp)

list(APPEND ALL_SOURCE_FILES ${DUMPEVERYTHING_SOURCE_FILES})

add_executable(OpenApoc_DumpEverything ${DUMPEVERYTHING_SOURCE_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_DumpEverything OpenApoc_Library)
target_link_libraries(OpenApoc_DumpEverything OpenApoc_Framework)
