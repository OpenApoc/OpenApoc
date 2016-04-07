CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

option(BUILD_EXTRACTOR "Build the initial gamestate extractor tool" ON)
option(BUILD_EDITOR "Build the gamestate editor tool" OFF)

if(BUILD_EXTRACTOR)
		add_subdirectory(extractors)
endif()

if(BUILD_EDITOR)
		add_subdirectory(editor)
endif()
