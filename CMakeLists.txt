cmake_minimum_required(VERSION 2.8)
project(OpenApoc)

#c++11 is required
# FIXME: Do compiler version check for c++11 support?
set(CMAKE_CXX_FLAGS "-std=c++11 -W -Wall")

set (OPENAPOC_SOURCE_FILES
	framework/event.cpp
	framework/framework.cpp
	framework/main.cpp
	framework/network.cpp
	framework/stagestack.cpp
	framework/data.cpp
	game/apocresources/apocfont.cpp
	game/apocresources/cursor.cpp
	game/apocresources/language.cpp
	game/apocresources/music.cpp
	game/apocresources/palette.cpp
	game/apocresources/pck.cpp
	game/apocresources/rawsound.cpp
	game/boot.cpp
	game/general/mainmenu.cpp
	library/angle.cpp
	library/animation.cpp
	library/box.cpp
	library/configfile.cpp
	library/line.cpp
	library/memory.cpp
	library/spritesheet.cpp
	library/tilemap.cpp
	library/vector2.cpp
	shaders/greyscale.cpp
	shaders/scanlines.cpp
	transitions/fadein.cpp
	transitions/fadeout.cpp
	transitions/strips.cpp
	transitions/tiled.cpp
)

set (REQUIRED_ALLEGRO_LIBRARIES allegro allegro_font allegro_primitives
		allegro_ttf allegro_image allegro_audio allegro_acodec)

find_package(PkgConfig)
pkg_check_modules(PC_ALLEGRO REQUIRED allegro-5.0)

find_path(ALLEGRO_INCLUDE_DIR allegro5/allegro.h
	HINTS ${PC_ALLEGRO_INCLUDEDIR})

include_directories(${ALLEGRO_INCLUDE_DIR})
add_executable(OpenApoc ${OPENAPOC_SOURCE_FILES})


#FIXME: Check libraries exist?
foreach (LIB ${REQUIRED_ALLEGRO_LIBRARIES})
	target_link_libraries(OpenApoc ${LIB})
endforeach()

