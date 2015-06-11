# project name, and type
PROJECT(OpenApoc CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/framework/cmake/")

include(framework)

# check compiler
INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	if (COMPILER_SUPPORTS_CXX0X)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
		message(FATAL_ERROR "Your C++ compiler does not support C++11.")
	endif()
endif()

option (EXTRA_WARNINGS "Enable extra warning flags (-W -Wall -Wextra)" ON)

if(EXTRA_WARNINGS)
	CHECK_CXX_COMPILER_FLAG("-W -Wall -Wextra" COMPILER_SUPPORTS_WARNINGS)
	if (COMPILER_SUPPORTS_WARNINGS)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra")
	endif(COMPILER_SUPPORTS_WARNINGS)
endif(EXTRA_WARNINGS)

CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden"
		COMPILER_SUPPORTS_HIDDEN_VISIBILITY)
if(COMPILER_SUPPORTS_HIDDEN_VISIBILITY)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif(COMPILER_SUPPORTS_HIDDEN_VISIBILITY)

option(MONOLITHIC_BUILD "Build all source in a single step" OFF)

option(LTO "Build using link-time-optimisations" OFF)

CHECK_CXX_COMPILER_FLAG("-flto"
	COMPILER_SUPPORTS_LTO)

if (LTO)
		if (NOT COMPILER_SUPPORTS_LTO)
				message(FATAL_ERROR "LTO not supported by this compiler")
		endif(NOT COMPILER_SUPPORTS_LTO)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
endif(LTO)


# find openapoc sources
AUX_SOURCE_DIRECTORY(game sources)
AUX_SOURCE_DIRECTORY(game/apocresources sources)
AUX_SOURCE_DIRECTORY(game/general sources)
AUX_SOURCE_DIRECTORY(game/city sources)
AUX_SOURCE_DIRECTORY(game/tileview sources)
AUX_SOURCE_DIRECTORY(game/resources sources)
AUX_SOURCE_DIRECTORY(game/ufopaedia sources)
AUX_SOURCE_DIRECTORY(game/debugtools sources)
AUX_SOURCE_DIRECTORY(library sources)
AUX_SOURCE_DIRECTORY(shaders sources)
AUX_SOURCE_DIRECTORY(transitions sources)
AUX_SOURCE_DIRECTORY(forms sources)

# declare tinyxml library
SET(REQUIRED_TINYXML_LIBRARIES tinyxml2)

if (MONOLITHIC_BUILD)
		add_custom_command(OUTPUT sources.cpp
				COMMAND ${CMAKE_SOURCE_DIR}/append_sources.sh ${sources} ${FRAMEWORK_SOURCES} >
				sources.cpp
				DEPENDS ${sources} ${FRAMEWORK_SOURCES})
		ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} sources.cpp)

else(MONOLITHIC_BUILD)
ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} framework/main.cpp ${sources}
		${FRAMEWORK_SOURCES})
endif(MONOLITHIC_BUILD)

# check dependencies and include libraries
FIND_PACKAGE(PkgConfig)
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
ADD_DEFINITIONS(-DDATA_DIRECTORY="${CMAKE_INSTALL_PREFIX}/share/OpenApoc")

INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FRAMEWORK_INCLUDE_DIRS})

foreach (TINYXML2_LIBRARY ${PC_TINYXML2_LIBRARIES})
		find_library(TINYXML2_LIBRARY_PATH ${TINYXML2_LIBRARY} HINTS
				${PC_TINYXML2_LIBRARY_DIRS})
		if (NOT TINYXML2_LIBRARY_PATH)
				message(FATAL_ERROR "Failed to find tinyxml2 library
				${TINYXML2_LIBRARY} in ${PC_TINYXML2_LIBRARY_DIRS}")
		endif()
		list(APPEND TINYXML2_LIBRARIES ${TINYXML2_LIBRARY_PATH})
endforeach(TINYXML2_LIBRARY)



TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${TINYXML2_LIBRARIES})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${FRAMEWORK_LIBRARIES})

# apoc data copy
SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )
ADD_CUSTOM_COMMAND( TARGET OpenApoc
	            POST_BUILD
		    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH}/data)

	install(TARGETS ${CMAKE_PROJECT_NAME}
			RUNTIME DESTINATION bin)
	install(DIRECTORY data/ DESTINATION share/OpenApoc)

enable_testing()
add_subdirectory(tests)
