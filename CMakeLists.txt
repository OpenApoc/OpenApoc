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

SET(RENDERER "OpenGL" CACHE STRING "Renderer implementation
(Supported: OpenGL Allegro")

SET(IMAGELOADER "Allegro" CACHE STRING "Image loader implementation
(Supported: Allegro")

if (${RENDERER} STREQUAL "OpenGL")
		find_package(OpenGL)
		if (NOT OPENGL_FOUND)
				message(FATAL_ERROR "OpenGL libraries not found")
		endif()
		list(APPEND sources "framework/render/ogl_renderer.cpp")
elseif(${RENDERER} STREQUAL "Allegro")
		list(APPEND sources "framework/render/allegro_renderer.cpp")
else()
		message(FATAL_ERROR "Invalid renderer implementation selected (Must be
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
SET(REQUIRED_ALLEGRO_LIBRARIES allegro allegro_font allegro_ttf
		allegro_primitives allegro_audio allegro_acodec)
SET(ALLEGRO_VERSIONS 5 5.0)

if (${IMAGELOADER} STREQUAL "Allegro")
		list(APPEND REQUIRED_ALLEGRO_LIBRARIES allegro_image)
		list(APPEND sources "framework/imageloader/allegro_image.cpp")
else()
		message(FATAL_ERROR "Invalid image load implementation selected (Must
		be one of \"Allegro\"")
endif()

# declare tinyxml library
SET(REQUIRED_TINYXML_LIBRARIES tinyxml2)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} framework/main.cpp ${sources})

# check dependencies and include libraries
FIND_PACKAGE(PkgConfig)
PKG_SEARCH_MODULE(PC_ALLEGRO REQUIRED allegro-5 allegro-5.0)
PKG_CHECK_MODULES(PC_TINYXML2 REQUIRED tinyxml2)

foreach (ALLEGRO_MODULE ${REQUIRED_ALLEGRO_LIBRARIES})
		unset(ALLEGRO_MODULE_NAMES)
		foreach (ALLEGRO_VERSION ${ALLEGRO_VERSIONS})
				list(APPEND ALLEGRO_MODULE_NAMES
						${ALLEGRO_MODULE}-${ALLEGRO_VERSION})
		endforeach(ALLEGRO_VERSION)
		PKG_SEARCH_MODULE(PC_ALLEGRO_MODULE-${ALLEGRO_MODULE} REQUIRED ${ALLEGRO_MODULE_NAMES})
		list(APPEND ALLEGRO_LIBRARIES ${PC_ALLEGRO_MODULE-${ALLEGRO_MODULE}_LIBRARIES})
endforeach(ALLEGRO_MODULE)

FIND_PATH(ALLEGRO_INCLUDE_DIR allegro5/allegro.h HINTS ${PC_ALLEGRO_INCLUDEDIR})
FIND_PATH(TINYXML2_INCLUDE_DIR tinyxml2.h HINTS ${PC_TINYXML2_INCLUDEDIR})

#Require the glm matrix/vector maths library
FIND_PATH(GLM_INCLUDE_DIR glm/glm.hpp)
MARK_AS_ADVANCED(GLM_INCLUDE_DIR)
if (NOT GLM_INCLUDE_DIR)
		message(FATAL_ERROR "GLM headers not found")
endif()

#All platforms that use CMake (Non-windows) have case sensitive filesystems
ADD_DEFINITIONS(-DDATA_DIRECTORY="${CMAKE_INSTALL_PREFIX}/share/OpenApoc")

INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})


#FIXME: Check libraries exist?
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${PC_TINYXML2_LIBRARIES})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${OPENGL_gl_LIBRARY})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${ALLEGRO_LIBRARIES})

# apoc data copy
SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )
ADD_CUSTOM_COMMAND( TARGET OpenApoc
	            POST_BUILD
		    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH}/data)

	install(TARGETS ${CMAKE_PROJECT_NAME}
			RUNTIME DESTINATION bin)
	install(DIRECTORY data/ DESTINATION share/OpenApoc)
