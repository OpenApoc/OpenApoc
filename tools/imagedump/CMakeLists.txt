# project name, and type
PROJECT(OpenApoc_ImageDump CXX C)

set (IMAGEDUMP_SOURCE_FILES
	imagedump.cpp)

list(APPEND ALL_SOURCE_FILES ${IMAGEDUMP_SOURCE_FILES})

add_executable(OpenApoc_ImageDump ${IMAGEDUMP_SOURCE_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_ImageDump OpenApoc_Library)
target_link_libraries(OpenApoc_ImageDump OpenApoc_Framework)
