# project name, and type
PROJECT(OpenApoc CXX)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# check compiler
INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
	message(FATAL_ERROR "Your C++ compiler does not support C++11.")
endif()

# find openapoc sources
AUX_SOURCE_DIRECTORY(framework sources)
AUX_SOURCE_DIRECTORY(game sources)
AUX_SOURCE_DIRECTORY(game/apocresources sources)
AUX_SOURCE_DIRECTORY(game/general sources)
AUX_SOURCE_DIRECTORY(game/resources sources)
AUX_SOURCE_DIRECTORY(library sources)
AUX_SOURCE_DIRECTORY(shaders sources)
AUX_SOURCE_DIRECTORY(transitions sources)
AUX_SOURCE_DIRECTORY(forms sources)

# declare allegro libraries
SET(REQUIRED_ALLEGRO_LIBRARIES allegro allegro_main allegro_font allegro_primitives allegro_ttf allegro_image allegro_audio allegro_acodec)

# declare tinyxml library
SET(REQUIRED_TINYXML_LIBRARIES tinyxml2)

# check dependencies and include libraries
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_ALLEGRO REQUIRED allegro-5)
PKG_CHECK_MODULES(PC_TINYXML2 REQUIRED tinyxml2)

FIND_PATH(ALLEGRO_INCLUDE_DIR allegro5/allegro.h HINTS ${PC_ALLEGRO_INCLUDEDIR})
FIND_PATH(TINYXML2_INCLUDE_DIR tinyxml2.h HINTS ${PC_TINYXML2_INCLUDEDIR})

INCLUDE_DIRECTORIES(${ALLEGRO_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIR})

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} framework/main.cpp ${sources})

#FIXME: Check libraries exist?
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${REQUIRED_ALLEGRO_LIBRARIES})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${PC_TINYXML2_LIBRARIES})

# apoc data copy
SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )
ADD_CUSTOM_COMMAND( TARGET OpenApoc
	            POST_BUILD
		    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH}/data)
