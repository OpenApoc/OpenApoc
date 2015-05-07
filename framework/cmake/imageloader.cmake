cmake_minimum_required(VERSION 2.8)

option (BUILD_ALLEGRO_IMAGELOADER "Build an allegro image loader backend" ON)

set (IMAGELOADERS "")
set (IMAGELOADER_SOURCES "")
set (IMAGELOADER_LIBRARIES "")
set (IMAGELOADER_INCLUDES "")

if(BUILD_ALLEGRO_IMAGELOADER)
		list(APPEND IMAGELOADERS allegro)
		list(APPEND IMAGELOADER_SOURCES "framework/imageloader/allegro_image.cpp")
		list(APPEND FRAMEWORK_ALLEGRO_LIBRARIES allegro_image allegro allegro_physfs)
endif()

if (NOT IMAGELOADERS)
		message(FATAL_ERROR "No image loaders specified")
endif()

message("Building image loaders: ${IMAGELOADERS}")
