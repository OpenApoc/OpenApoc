# project name, and type
PROJECT(OpenApoc_SerializationTool CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (SERIALIZATIONTOOL_SOURCE_FILES
	main.cpp)

source_group(serializationtool\\sources FILES ${SERIALIZATIONTOOL_SOURCE_FILES})

set (SERIALIZATIONTOOL_HEADER_FILES
	)

source_group(serializationtool\\headers FILES ${SERIALIZATIONTOOL_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${SERIALIZATIONTOOL_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${SERIALIZATIONTOOL_HEADER_FILES})

add_executable(OpenApoc_SerializationTool ${SERIALIZATIONTOOL_SOURCE_FILES}
		${SERIALIZATIONTOOL_HEADER_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_SerializationTool OpenApoc_Library)
target_link_libraries(OpenApoc_SerializationTool OpenApoc_Framework)
target_link_libraries(OpenApoc_SerializationTool OpenApoc_GameState)
