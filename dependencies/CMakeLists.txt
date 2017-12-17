# project name, and type
PROJECT(OpenApoc_Dependencies C CXX)

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

target_include_directories(OpenApoc_LibSmacker PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_LibSmacker PRIVATE
		${CMAKE_SOURCE_DIR}/dependencies/libsmacker)

target_compile_definitions(OpenApoc_LibSmacker PUBLIC PTHREADS_AVAILABLE)

set (MINIZ_SOURCE_FILES
	miniz/miniz.c
	miniz/miniz_zip.c
	miniz/miniz_tdef.c
	miniz/miniz_tinfl.c)
source_group(miniz\\sources FILES ${MINIZ_SOURCE_FILES})

list(APPEND ALL_SOURCE_FILES ${MINIZ_SOURCE_FILES})

add_library(OpenApoc_Miniz STATIC ${MINIZ_SOURCE_FILES})

target_compile_definitions(OpenApoc_Miniz PUBLIC MINIZ_NO_TIME)
# We probably don't support any platforms without the 64-bit offset file
# ioctls
target_compile_definitions(OpenApoc_Miniz PUBLIC _LARGEFILE64_SOURCE=1)

target_include_directories(OpenApoc_Miniz PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_Miniz PRIVATE
		${CMAKE_SOURCE_DIR}/dependencies/miniz)

target_compile_definitions(OpenApoc_Miniz PUBLIC PTHREADS_AVAILABLE)

set (LIBPUGIXML_SOURCE_FILES
		pugixml/src/pugixml.cpp)

source_group(libpugixml\\sources FILES ${LIBPUGIXML_SOURCE_FILES})

set (LIBPUGIXML_HEADER_FILES
		pugixml/src/pugixml.hpp
		pugixml/src/pugiconfig.hpp)

source_group(libpugixml\\headers FILES ${LIBPUGIXML_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBPUGIXML_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBPUGIXML_HEADER_FILES})

add_library(OpenApoc_LibPugixml STATIC ${LIBPUGIXML_SOURCE_FILES}
		${LIBPUGIXML_HEADER_FILES})

target_compile_definitions(OpenApoc_LibPugixml PUBLIC PTHREADS_AVAILABLE)

set_property(TARGET OpenApoc_LibPugixml PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_LibPugixml PROPERTY CXX_STANDARD_REQUIRED ON)

set (LIBLODEPNG_SOURCE_FILES
		lodepng/lodepng.cpp)

source_group(libpugixml\\sources FILES ${LIBLODEPNG_SOURCE_FILES})

set (LIBLODEPNG_HEADER_FILES
		lodepng/lodepng.h)

source_group(libpugixml\\headers FILES ${LIBLODEPNG_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBLODEPNG_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBLODEPNG_HEADER_FILES})

add_library(OpenApoc_LibLodepng STATIC ${LIBLODEPNG_SOURCE_FILES}
		${LIBLODEPNG_HEADER_FILES})

target_compile_definitions(OpenApoc_LibLodepng PUBLIC PTHREADS_AVAILABLE)

set_property(TARGET OpenApoc_LibLodepng PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_LibLodepng PROPERTY CXX_STANDARD_REQUIRED ON)

# Only enable the subset of physfs we actually use

set(PHYSFS_ARCHIVE_7Z OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_GRP OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_HOG OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_MVL OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_QPAK OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_SLB OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_WAD OFF CACHE BOOL "")
set(PHYSFS_ARCHIVE_ISO9660 ON CACHE BOOL "")
set(PHYSFS_ARCHIVE_ISO9660_LOWERCASE ON CACHE BOOL "")
set(PHYSFS_ARCHIVE_ISO9660_OPENAPOC_WORKAROUND ON CACHE BOOL "")
set(PHYSFS_ARCHIVE_ZIP ON CACHE BOOL "")

set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "")
set(PHYSFS_BUILD_STATIC ON CACHE BOOL "")

set(PHYSFS_BUILD_TEST OFF CACHE BOOL "")

add_subdirectory(physfs)
