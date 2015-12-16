cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig)

option (BUILD_LODEPNG_IMAGELOADER "Build a lodepng image loader backend" ON)
option (BUILD_PCX_IMAGELOADER "Build a pcx image loader backend" ON)

set (IMAGELOADERS "")
set (IMAGELOADER_SOURCES "")
set (IMAGELOADER_LIBRARIES "")
set (IMAGELOADER_INCLUDES "")

if (BUILD_LODEPNG_IMAGELOADER)
		list(APPEND IMAGELOADERS lodepng)
		list(APPEND IMAGELOADER_SOURCES "framework/imageloader/lodepng_image.cpp" "framework/imageloader/lodepng.cpp")
endif()

if (BUILD_PCX_IMAGELOADER)
		list(APPEND IMAGELOADERS pcx)
		list(APPEND IMAGELOADER_SOURCES "framework/imageloader/pcx.cpp")
endif()

if (NOT IMAGELOADERS)
		message(FATAL_ERROR "No image loaders specified")
endif()

message("Building image loaders: ${IMAGELOADERS}")
