cmake_minimum_required(VERSION 2.8)
project(OpenApoc)

#c++11 is required
# FIXME: Do compiler version check for c++11 support?
set(CMAKE_CXX_FLAGS "-std=c++11 -W -Wall")

set (OPENAPOC_SOURCE_FILES
	framework/network.cpp
	framework/stagestack.cpp
	framework/event.cpp
	framework/framework.cpp
	framework/main.cpp
	framework/data.cpp
	game/apocresources/cursor.cpp
	game/apocresources/music.cpp
	game/apocresources/palette.cpp
	game/apocresources/pck.cpp
	game/apocresources/rawsound.cpp
	game/apocresources/apocfont.cpp
	game/general/mainmenu.cpp
	game/general/optionsmenu.cpp
	game/boot.cpp
	game/resources/gamecore.cpp
	game/resources/ttffont.cpp
	library/angle.cpp
	library/animation.cpp
	library/line.cpp
	library/memory.cpp
	library/tilemap.cpp
	library/vector2.cpp
	library/box.cpp
	library/configfile.cpp
	library/spritesheet.cpp
	shaders/greyscale.cpp
	shaders/scanlines.cpp
	transitions/fadeacross.cpp
	transitions/fadein.cpp
	transitions/fadeout.cpp
	transitions/strips.cpp
	transitions/tiled.cpp
	forms/checkbox.cpp
	forms/control.cpp
	forms/form.cpp
	forms/graphic.cpp
	forms/graphicbutton.cpp
	forms/label.cpp
	forms/textbutton.cpp
	forms/hscrollbar.cpp
	forms/vscrollbar.cpp
	forms/list.cpp
)

set (REQUIRED_ALLEGRO_LIBRARIES allegro allegro_font allegro_primitives
		allegro_ttf allegro_image allegro_audio allegro_acodec)

find_package(PkgConfig)
pkg_check_modules(PC_ALLEGRO REQUIRED allegro-5.0)

pkg_check_modules(PC_TINYXML2 REQUIRED tinyxml2)

find_path(ALLEGRO_INCLUDE_DIR allegro5/allegro.h
	HINTS ${PC_ALLEGRO_INCLUDEDIR})

find_path(TINYXML2_INCLUDE_DIR tinyxml2.h
	HINTS ${PC_TINYXML2_INCLUDEDIR})

include_directories(${ALLEGRO_INCLUDE_DIR})
include_directories(${TINYXML2_INCLUDE_DIR})
add_executable(OpenApoc ${OPENAPOC_SOURCE_FILES})

target_link_libraries(OpenApoc ${PC_TINYXML2_LIBRARIES})

#FIXME: Check libraries exist?
foreach (LIB ${REQUIRED_ALLEGRO_LIBRARIES})
	target_link_libraries(OpenApoc ${LIB})
endforeach()

