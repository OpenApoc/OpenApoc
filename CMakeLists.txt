# project name, and type
PROJECT(OpenApoc CXX C)

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

SET(IMAGE_IMPLEMENTATION "OpenGL" CACHE STRING "Image implemtnation
(Supported: OpenGL Allegro")

if (${IMAGE_IMPLEMENTATION} STREQUAL "OpenGL")
		find_package(OpenGL)
		if (NOT OPENGL_FOUND)
				message(FATAL_ERROR "OpenGL libraries not found")
		endif()
		list(APPEND sources "framework/render/gl_image.cpp")
		list(APPEND sources "framework/render/gl_util.h")
elseif(${IMAGE_IMPLEMENTATION} STREQUAL "Allegro")
		list(APPEND sources "framework/render/allegro_image.cpp")
else()
		message(FATAL_ERROR "Invalid image implementation selected (Must be
		one of \"OpenGL\",\"Allegro\"")
endif()


# find openapoc sources
AUX_SOURCE_DIRECTORY(framework sources)
AUX_SOURCE_DIRECTORY(game sources)
AUX_SOURCE_DIRECTORY(game/apocresources sources)
AUX_SOURCE_DIRECTORY(game/general sources)
AUX_SOURCE_DIRECTORY(game/city sources)
AUX_SOURCE_DIRECTORY(game/tileview sources)
AUX_SOURCE_DIRECTORY(game/resources sources)
AUX_SOURCE_DIRECTORY(library sources)
AUX_SOURCE_DIRECTORY(shaders sources)
AUX_SOURCE_DIRECTORY(transitions sources)
AUX_SOURCE_DIRECTORY(forms sources)

# declare allegro libraries
SET(REQUIRED_ALLEGRO_LIBRARIES allegro-5 allegro_font-5 allegro_ttf-5
		allegro_image-5 allegro_primitives-5 allegro_audio-5 allegro_acodec-5)

# declare tinyxml library
SET(REQUIRED_TINYXML_LIBRARIES tinyxml2)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} framework/main.cpp ${sources})

# check dependencies and include libraries
FIND_PACKAGE(PkgConfig)
foreach (ALLEGRO_MODULE ${REQUIRED_ALLEGRO_LIBRARIES})
		PKG_CHECK_MODULES(PC_${ALLEGRO_MODULE} REQUIRED ${ALLEGRO_MODULE})
		INCLUDE_DIRECTORIES(${PC_${ALLEGRO_MODULE}_INCLUDEDIR})
		TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME}
				${PC_${ALLEGRO_MODULE}_LIBRARIES})
endforeach(ALLEGRO_MODULE)

PKG_CHECK_MODULES(PC_TINYXML2 REQUIRED tinyxml2)

FIND_PATH(ALLEGRO_INCLUDE_DIR allegro5/allegro.h HINTS ${PC_ALLEGRO_INCLUDEDIR})
FIND_PATH(TINYXML2_INCLUDE_DIR tinyxml2.h HINTS ${PC_TINYXML2_INCLUDEDIR})

#Require the glm matrix/vector maths library
FIND_PATH(GLM_INCLUDE_DIR glm/glm.hpp)
MARK_AS_ADVANCED(GLM_INCLUDE_DIR)
if (NOT GLM_INCLUDE_DIR)
		message(FATAL_ERROR "GLM headers not found")
endif()

#All platforms that use CMake (Non-windows) have case sensitive filesystems
ADD_DEFINITIONS(-DCASE_SENSITIVE_FILESYSTEM)

INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})


#FIXME: Check libraries exist?
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${PC_TINYXML2_LIBRARIES})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${OPENGL_gl_LIBRARY})

# apoc data copy
SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )
ADD_CUSTOM_COMMAND( TARGET OpenApoc
	            POST_BUILD
		    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH}/data)
