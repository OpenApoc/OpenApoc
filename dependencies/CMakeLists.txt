# project name, and type
PROJECT(OpenApoc_LibSmacker C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (LIBSMACKER_SOURCE_FILES
	libsmacker/smacker.c
	libsmacker/smk_bitstream.c
	libsmacker/smk_hufftree.c)
source_group(libsmacker\\sources FILES ${LIBSMACKER_SOURCE_FILES})
set (LIBSMACKER_HEADER_FILES
	libsmacker/smacker.h
	libsmacker/smk_bitstream.h
	libsmacker/smk_hufftree.h
	libsmacker/smk_malloc.h)
source_group(libsmacker\\headers FILES ${LIBSMACKER_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBSMACKER_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBSMACKER_HEADER_FILES})

add_library(OpenApoc_LibSmacker STATIC ${LIBSMACKER_SOURCE_FILES}
		${LIBSMACKER_HEADER_FILES})

target_include_directories(OpenApoc_Library PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_Library PRIVATE
		${CMAKE_SOURCE_DIR}/dependencies/libsmacker)

target_compile_definitions(OpenApoc_Library PUBLIC PTHREADS_AVAILABLE)
