# project name, and type
PROJECT(OpenApoc CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/framework/cmake/")

include(framework)

# check compiler
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCCompilerFlag)
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
option (EXTRA_SUGGESTIONS "Enable extra 'suggestion' warning flags" OFF)

function(ADD_OPTIONAL_COMPILER_FLAG FLAG)
		CHECK_CXX_COMPILER_FLAG("${FLAG}" CXX_COMPILER_SUPPORTS_${FLAG})
		CHECK_C_COMPILER_FLAG("${FLAG}" C_COMPILER_SUPPORTS_${FLAG})
		if (CXX_COMPILER_SUPPORTS_${FLAG})
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
		endif(CXX_COMPILER_SUPPORTS_${FLAG})
		if (C_COMPILER_SUPPORTS_${FLAG})
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}" PARENT_SCOPE)
		endif(C_COMPILER_SUPPORTS_${FLAG})
endfunction(ADD_OPTIONAL_COMPILER_FLAG)

if (EXTRA_SUGGESTIONS)
		# Cmake has buggy check_cxx_compiler_flag() before 3.1 (interprets the arg 
		# as a regex) which breaks at least Wno-c++98-compat
		CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-attribute=pure")
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-attribute=const")
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-attribute=noreturn")
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-attribute=format")
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-final-types")
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-final-methods")
		ADD_OPTIONAL_COMPILER_FLAG("-Wsuggest-override")
		#Clang has a -Weverything
		ADD_OPTIONAL_COMPILER_FLAG("-Weverything")
		#But that's pretty annoying, so disable some stuff we don't carea bout
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-c++98-compat")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-c++98-compat-pedantic")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-gnu-zero-variadic-macro-arguments")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-shadow")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-weak-vtables")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-padded")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-exit-time-destructors")
		ADD_OPTIONAL_COMPILER_FLAG("-Wno-global-constructors")
endif(EXTRA_SUGGESTIONS)

if(EXTRA_WARNINGS)
	ADD_OPTIONAL_COMPILER_FLAG("-W")
	ADD_OPTIONAL_COMPILER_FLAG("-Wall")
	ADD_OPTIONAL_COMPILER_FLAG("-Wextra")
endif(EXTRA_WARNINGS)

CHECK_CXX_COMPILER_FLAG("-pthread" COMPILER_SUPPORTS_pthread)
if(COMPILER_SUPPORTS_CXX11)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

ADD_OPTIONAL_COMPILER_FLAG("-fvisibility=hidden")

option(LTO "Build using link-time-optimisations" OFF)

CHECK_CXX_COMPILER_FLAG("-flto"
	COMPILER_SUPPORTS_LTO)

if (LTO)
		if (NOT COMPILER_SUPPORTS_LTO)
				message(WARNING "LTO not supported by this compiler")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
		endif(NOT COMPILER_SUPPORTS_LTO)
endif(LTO)


# find openapoc sources
AUX_SOURCE_DIRECTORY(game sources)
AUX_SOURCE_DIRECTORY(game/apocresources sources)
AUX_SOURCE_DIRECTORY(game/general sources)
AUX_SOURCE_DIRECTORY(game/city sources)
AUX_SOURCE_DIRECTORY(game/rules sources)
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

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} framework/main.cpp ${sources}
		${FRAMEWORK_SOURCES})

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

INCLUDE_DIRECTORIES(SYSTEM ${TINYXML2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM ${GLM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM ${FRAMEWORK_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

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

set(CLANG_FORMAT_COMMANDS clang-format clang-format-3.8 clang-format-3.7
		clang-format-3.6)

foreach(POSSIBLE_CLANG_FORMAT_COMMAND ${CLANG_FORMAT_COMMANDS})
		unset(COMMAND_OUTPUT)
		execute_process(COMMAND which ${POSSIBLE_CLANG_FORMAT_COMMAND}
				OUTPUT_VARIABLE COMMAND_OUTPUT ERROR_QUIET
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		if (COMMAND_OUTPUT)
				message("Found clang-format ${COMMAND_OUTPUT}")
				SET(CLANG_FORMAT_COMMAND ${COMMAND_OUTPUT} CACHE PATH
						"clang-format executable")
				break()
		endif()
endforeach()

file(GLOB_RECURSE FORMAT_SOURCES ${CMAKE_SOURCE_DIR}/*.cpp
		${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.h)

add_custom_target(format)
add_custom_target(format-sources)
add_custom_target(format-xml)

add_dependencies(format format-sources)
add_dependencies(format format-xml)

if (CLANG_FORMAT_COMMAND)
	foreach(FORMAT_SOURCE ${FORMAT_SOURCES})
		string(MAKE_C_IDENTIFIER ${FORMAT_SOURCE} FORMAT_SOURCE_STRIPPED)
		add_custom_target(format-source-${FORMAT_SOURCE_STRIPPED} ${CLANG_FORMAT_COMMAND} -i ${FORMAT_SOURCE} DEPENDS ${FORMAT_SOURCE} .clang-format)
		add_dependencies(format-sources
				format-source-${FORMAT_SOURCE_STRIPPED})
	endforeach()
else()
	message("No clang-format found, disabling format-source target")
endif()

file(GLOB_RECURSE XML_FILES ${CMAKE_SOURCE_DIR}/data/*.xml
		${CMAKE_SOURCE_DIR}/data/*.XML)

execute_process(COMMAND which xmllint OUTPUT_VARIABLE WHICH_XMLLINT_OUTPUT ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
if (WHICH_XMLLINT_OUTPUT)
	SET(XMLLINT_COMMAND ${WHICH_XMLLINT_OUTPUT} CACHE PATH
		"xmllint executable")
	message("Found xmllint ${WHICH_XMLLINT_OUTPUT}")
endif()

if (XMLLINT_COMMAND)
		foreach(XML_FILE ${XML_FILES})
			string(MAKE_C_IDENTIFIER ${XML_FILE} XML_FILE_STRIPPED)
			add_custom_target(format-xml-${XML_FILE_STRIPPED}
					${XMLLINT_COMMAND} --format --encode UTF-8 ${XML_FILE}
					--output ${XML_FILE}
					DEPENDS ${XML_FILE})
			add_dependencies(format-xml format-xml-${XML_FILE_STRIPPED})
		endforeach()
else()
	message("xmllint not found, disabling format-xml target")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CLANG_TIDY clang-tidy CACHE STRING "clang-tidy executable name in PATH")

find_package(Git)
execute_process(COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} describe --tags --all --long
		OUTPUT_VARIABLE OPENAPOC_VERSION_STRING_RAW
		OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE "heads/" "branch/" OPENAPOC_VERSION_STRING_RAW ${OPENAPOC_VERSION_STRING_RAW})
# Remove 'special' character (e.g. '/') as that screws up filenames etc. Abuse
# MAKE_C_IDENTIFIER for this instead of a list of 'banned' chars
string(MAKE_C_IDENTIFIER "${OPENAPOC_VERSION_STRING_RAW}" OPENAPOC_VERSION_STRING )

configure_file(version.h.in version.h)

add_custom_target(tidy ${CLANG_TIDY} ${FORMAT_SOURCES} -p ${CMAKE_BINARY_DIR}
		DEPENDS ${FORMAT_SOURCES} ${CMAKE_BINARY_DIR}/compile_commands.json)
