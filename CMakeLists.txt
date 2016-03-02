# project name, and type
PROJECT(OpenApoc CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/framework/cmake/")

include(framework)

# check compiler
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-pthread" COMPILER_SUPPORTS_pthread)
if(COMPILER_SUPPORTS_pthread)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

option(LTO "Build using link-time-optimisations" OFF)

CHECK_CXX_COMPILER_FLAG("-flto"
	COMPILER_SUPPORTS_LTO)
CHECK_CXX_COMPILER_FLAG("-flto=4"
	COMPILER_SUPPORTS_LTO4)

if (LTO)
		if (COMPILER_SUPPORTS_LTO4)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=4")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto=4")
		elseif(COMPILER_SUPPORTS_LTO)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
		else()
		message(WARNING "LTO not supported by this compiler")
		endif()
endif(LTO)

find_package(Boost REQUIRED COMPONENTS locale)


# find openapoc sources
AUX_SOURCE_DIRECTORY(game sources)
AUX_SOURCE_DIRECTORY(game/apocresources sources)
AUX_SOURCE_DIRECTORY(game/base sources)
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

target_link_libraries(${CMAKE_PROJECT_NAME} ${Boost_LIBRARIES})
target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
# Assume all targets using cmake have pthreads
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC PTHREADS_AVAILABLE)

set_property(TARGET OpenApoc PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc PROPERTY CXX_STANDARD_REQUIRED ON)

# check dependencies and include libraries
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_TINYXML2 REQUIRED tinyxml2)


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

file(GLOB_RECURSE FORMAT_SOURCES ${CMAKE_SOURCE_DIR}/*.cpp
		${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.h)

file(GLOB_RECURSE DEPENDENCY_SOURCES ${CMAKE_SOURCE_DIR}/dependencies/*.cpp
		${CMAKE_SOURCE_DIR}/dependencies/*.c ${CMAKE_SOURCE_DIR}/dependencies/*.h)

if (DEPENDENCY_SOURCES)
	list(REMOVE_ITEM FORMAT_SOURCES ${DEPENDENCY_SOURCES})
endif()

add_custom_target(format)
add_custom_target(format-sources)
add_custom_target(format-xml)

add_dependencies(format format-sources)
add_dependencies(format format-xml)

set(CLANG_FORMAT clang-format CACHE STRING "clang-format executable name in PATH")

foreach(FORMAT_SOURCE ${FORMAT_SOURCES})
	string(MAKE_C_IDENTIFIER ${FORMAT_SOURCE} FORMAT_SOURCE_STRIPPED)
	add_custom_target(format-source-${FORMAT_SOURCE_STRIPPED} ${CLANG_FORMAT} -i ${FORMAT_SOURCE} DEPENDS ${FORMAT_SOURCE} .clang-format)
	add_dependencies(format-sources
			format-source-${FORMAT_SOURCE_STRIPPED})
endforeach()

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
set(CLANG_TIDY_FIX OFF tidy CACHE BOOL "Pass -fix to clang-tidy (DANGEROUS)")

find_package(Git)
execute_process(COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} describe --tags --all --long
		OUTPUT_VARIABLE OPENAPOC_VERSION_STRING_RAW
		OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE "heads/" "branch/" OPENAPOC_VERSION_STRING_RAW ${OPENAPOC_VERSION_STRING_RAW})
# Remove 'special' character (e.g. '/') as that screws up filenames etc. Abuse
# MAKE_C_IDENTIFIER for this instead of a list of 'banned' chars
string(MAKE_C_IDENTIFIER "${OPENAPOC_VERSION_STRING_RAW}" OPENAPOC_VERSION_STRING )

configure_file(version.h.in version.h)

if (CLANG_TIDY_FIX)
		set(CLANG_TIDY_OPTIONS "-fix")
endif()

add_custom_target(tidy ${CLANG_TIDY} ${FORMAT_SOURCES} -p ${CMAKE_BINARY_DIR}
		${CLANG_TIDY_OPTIONS}
		DEPENDS ${FORMAT_SOURCES} ${CMAKE_BINARY_DIR}/compile_commands.json)


# Localisation

set(LANGUAGES en_GB fr_FR de_DE cs es it pl pt_BR ru_RU uk)

find_package(Gettext)
if (GETTEXT_FOUND)
		foreach(LANG ${LANGUAGES})
				GETTEXT_PROCESS_PO_FILES(${LANG} PO_FILES
						${CMAKE_SOURCE_DIR}/data/languages/ufo_string_${LANG}.po)
				GETTEXT_PROCESS_PO_FILES(${LANG} PO_FILES
						${CMAKE_SOURCE_DIR}/data/languages/paedia_string_${LANG}.po)
		endforeach()
		add_custom_target(update-translations DEPENDS pofiles)
		foreach (LANG ${LANGUAGES})
				add_custom_command(TARGET update-translations DEPENDS pofiles
						COMMAND ${CMAKE_COMMAND} -E copy
						${CMAKE_BINARY_DIR}/paedia_string_${LANG}.gmo
						${CMAKE_SOURCE_DIR}/data/languages/${LANG}/LC_MESSAGES/paedia_string.mo)
				add_custom_command(TARGET update-translations DEPENDS pofiles
						COMMAND ${CMAKE_COMMAND} -E copy
						${CMAKE_BINARY_DIR}/ufo_string_${LANG}.gmo
						${CMAKE_SOURCE_DIR}/data/languages/${LANG}/LC_MESSAGES/ufo_string.mo)
		endforeach()
endif(GETTEXT_FOUND)
