# project name, and type
PROJECT(OpenApoc_Library CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

find_package(Boost REQUIRED COMPONENTS locale)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (LIBRARY_SOURCE_FILES
	colour.cpp
	strings.cpp
	voxel.cpp)
source_group(library\\sources FILES ${LIBRARY_SOURCE_FILES})
set (LIBRARY_HEADER_FILES
	colour.h
	enum_traits.h
	rect.h
	sp.h
	strings.h
	strings_format.h
	vec.h
	resource.h
	voxel.h
	line.h
	xorshift.h)
source_group(library\\headers FILES ${LIBRARY_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBRARY_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBRARY_HEADER_FILES})

add_library(OpenApoc_Library STATIC ${LIBRARY_SOURCE_FILES}
		${LIBRARY_HEADER_FILES})

target_link_libraries(OpenApoc_Library PUBLIC ${Boost_LIBRARIES}
		Threads::Threads)

target_include_directories(OpenApoc_Library PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_Library PUBLIC ${GLM_INCLUDE_DIR})
target_include_directories(OpenApoc_Library PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(OpenApoc_Library PUBLIC ${CMAKE_SOURCE_DIR}/library/compat)
if(NOT WIN32)
	target_compile_definitions(OpenApoc_Library PUBLIC PTHREADS_AVAILABLE)
endif()
