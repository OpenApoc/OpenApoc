CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

option(BUILD_EXTRACTOR "Build the initial gamestate extractor tool" ON)
option(BUILD_IMAGEDUMP "Build a tool that dumps apoc images as png files" ON)
option(BUILD_GAMESTATESERIALIZEGEN "Build a that regenerates the gamestate
serialization code" ON)
option(BUILD_EDITOR "Build the gamestate editor tool" OFF)
option(BUILD_DUMPEVERYTHING "Tool that dumps all known images" OFF)

if(BUILD_EXTRACTOR)
		add_subdirectory(extractors)
endif()

if(BUILD_EDITOR)
		add_subdirectory(editor)
endif()

if (BUILD_IMAGEDUMP)
		add_subdirectory(imagedump)
endif()

if (BUILD_DUMPEVERYTHING)
		add_subdirectory(dump_everything)
endif()


# GameState serialization code generator isn't optional
add_subdirectory(gamestate_serialize_gen)
