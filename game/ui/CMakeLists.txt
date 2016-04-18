# project name, and type
PROJECT(OpenApoc_GameUI CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set (GAMEUI_SOURCE_FILES
	base/basegraphics.cpp
	base/basescreen.cpp
	base/researchscreen.cpp
	base/researchselect.cpp
	base/vequipscreen.cpp
	city/basebuyscreen.cpp
	city/baseselectscreen.cpp
	city/buildingscreen.cpp
	city/infiltrationscreen.cpp
	city/scorescreen.cpp
	city/cityview.cpp
	debugtools/debugmenu.cpp
	debugtools/formpreview.cpp
	general/difficultymenu.cpp
	general/ingameoptions.cpp
	general/loadingscreen.cpp
	general/mainmenu.cpp
	general/messagebox.cpp
	general/optionsmenu.cpp
	tileview/tileview.cpp
	ufopaedia/ufopaediacategoryview.cpp
	ufopaedia/ufopaediaview.cpp
	boot.cpp)
source_group(gamestate\\sources FILES ${GAMEUI_SOURCE_FILES})
set (GAMEUI_HEADER_FILES
	base/basegraphics.h
	base/basescreen.h
	base/researchscreen.h
	base/researchselect.h
	base/vequipscreen.h
	city/basebuyscreen.h
	city/baseselectscreen.h
	city/buildingscreen.h
	city/infiltrationscreen.h
	city/scorescreen.h
	city/cityview.h
	debugtools/debugmenu.h
	debugtools/formpreview.h
	general/difficultymenu.h
	general/ingameoptions.h
	general/loadingscreen.h
	general/mainmenu.h
	general/messagebox.h
	general/optionsmenu.h
	tileview/tileview.h
	ufopaedia/ufopaediacategoryview.h
	ufopaedia/ufopaediaview.h
	boot.h)
source_group(gamestate\\headers FILES ${GAMEUI_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${GAMEUI_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${GAMEUI_HEADER_FILES})

add_library(OpenApoc_GameUI STATIC ${GAMEUI_SOURCE_FILES}
		${GAMEUI_HEADER_FILES})

target_link_libraries(OpenApoc_GameUI PUBLIC OpenApoc_Library)
target_link_libraries(OpenApoc_GameUI PUBLIC OpenApoc_Framework)
target_link_libraries(OpenApoc_GameUI PUBLIC OpenApoc_GameState)
target_link_libraries(OpenApoc_GameUI PUBLIC OpenApoc_Forms)

# version.h is used in the main menu, and that's generated in the bin dir
target_include_directories(OpenApoc_GameUI PRIVATE ${CMAKE_BINARY_DIR})

set_property(TARGET OpenApoc_GameUI PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_GameUI PROPERTY CXX_STANDARD_REQUIRED ON)
