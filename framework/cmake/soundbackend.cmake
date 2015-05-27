cmake_minimum_required(VERSION 2.8)

option (BUILD_ALLEGRO_SOUNDBACKEND "Build an allegro audio backend" ON)
option (BUILD_NULL_SOUNDBACKEND "Build an null (non-functional) audio backend" ON)

set (SOUNDBACKENDS "")
set (SOUNDBACKEND_SOURCES "")
set (SOUNDBACKEND_LIBRARIES "")
set (SOUNDBACKEND_INCLUDES "")

if(BUILD_ALLEGRO_SOUNDBACKEND)
		list(APPEND SOUNDBACKENDS allegro)
		list(APPEND SOUNDBACKEND_SOURCES "framework/sound/allegro_backend.cpp")
		list(APPEND FRAMEWORK_ALLEGRO_LIBRARIES allegro_audio)
endif()

if(BUILD_NULL_SOUNDBACKEND)
		list(APPEND SOUNDBACKENDS null)
		list(APPEND SOUNDBACKEND_SOURCES "framework/sound/null_backend.cpp")
endif()

if (NOT SOUNDBACKENDS)
		message(FATAL_ERROR "No sound backends specified")
endif()

message("Building sound backends: ${SOUNDBACKENDS}")
