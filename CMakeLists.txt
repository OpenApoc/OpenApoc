# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

# project name, and type
PROJECT(OpenApoc CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(ccache)

# check compiler
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckCCompilerFlag)

# Sanitize doesn't seem to work with pch on gcc
string(TOLOWER "${CMAKE_BUILD_TYPE}" lower_build_type)

option(LTO "Build using link-time-optimisations" OFF)
option(MSVC_PDB "Always generate PDB files" OFF)
option(ENABLE_TESTS "Build some unit tests" ON)
option(EXTRACT_DATA "Run the data extractor as part of the default target" ON)

set(CD_PATH ${CMAKE_SOURCE_DIR}/data/cd.iso CACHE STRING "Path to cd.iso (used
by test and extractor")

if (${EXTRACT_DATA} AND NOT EXISTS ${CD_PATH})
		message(FATAL_ERROR "CD_PATH \"${CD_PATH}\" points at non-existant file")
endif()

message("Using CD at \"${CD_PATH}\"")

set(CMAKE_C_FLAGS_SANITIZE "-O1 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS_SANITIZE "-O1 -g -fsanitize=address")

set(CMAKE_C_FLAGS_QUICK "-O1 -g0")
set(CMAKE_CXX_FLAGS_QUICK "-O1 -g0")


CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden"
	CXX_COMPILER_SUPPORTS_HIDDEN_VISIBILITY)
CHECK_C_COMPILER_FLAG("-fvisibility=hidden"
	C_COMPILER_SUPPORTS_HIDDEN_VISIBILITY)

if (CXX_COMPILER_SUPPORTS_HIDDEN_VISIBILITY)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()
if (C_COMPILER_SUPPORTS_HIDDEN_VISIBILITY)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()

if (LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result OUTPUT output)
	message("Enabling LTO")
	if (result)
			set (CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
			message("LTO supported")
	else()
		message(WARNING "LTO not supported by this compiler :" ${output})
	endif()
endif(LTO)

# MSVC has default flags that CMake doesn't set
if (MSVC)
	add_definitions(-DUNICODE -D_UNICODE)
	if (MSVC_VERSION GREATER_EQUAL 1910)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /permissive-")
	endif()
	if (MSVC_PDB)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
	endif()
endif()

# Clang whines about unused arguments, which is annoying (for example) when
# using ccache and the clang exe never sees a non-preprocessed file, yet still
# gets passed the -I include arguments
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
endif()
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
endif()

set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/dependencies/glm)
MARK_AS_ADVANCED(GLM_INCLUDE_DIR)

add_subdirectory(library)
add_subdirectory(framework)
add_subdirectory(forms)
add_subdirectory(game/state)
add_subdirectory(game/ui)
add_subdirectory(tools)
add_subdirectory(dependencies)

if (WIN32)
	set(sources ${sources} "apocicon.rc")
endif()


if (APPLE)
set(MACOSX_BUNDLE_BUNDLE_NAME OpenApoc)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} MACOSX_BUNDLE game/main.cpp ${sources}
		${FRAMEWORK_SOURCES})
else()
ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} game/main.cpp ${sources}
		${FRAMEWORK_SOURCES})
endif()


target_link_libraries(${CMAKE_PROJECT_NAME} OpenApoc_Library)
target_link_libraries(${CMAKE_PROJECT_NAME} OpenApoc_Framework)
target_link_libraries(${CMAKE_PROJECT_NAME} OpenApoc_Forms)
target_link_libraries(${CMAKE_PROJECT_NAME} OpenApoc_GameState)
target_link_libraries(${CMAKE_PROJECT_NAME} OpenApoc_GameUI)

# check dependencies and include libraries


#All Non-windows platforms have case sensitive filesystems
if (NOT WIN32)
	ADD_DEFINITIONS(-DDATA_DIRECTORY="${CMAKE_INSTALL_PREFIX}/share/OpenApoc")
endif()

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

# apoc data copy
SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )


if (NOT APPLE)
		install(TARGETS ${CMAKE_PROJECT_NAME}
				RUNTIME DESTINATION bin)
		install(DIRECTORY ${CMAKE_SOURCE_DIR}data/ DESTINATION share/OpenApoc)
endif()

if(ENABLE_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

file(GLOB_RECURSE FORMAT_SOURCES
		${CMAKE_SOURCE_DIR}/library/*.cpp
		${CMAKE_SOURCE_DIR}/library/*.c
		${CMAKE_SOURCE_DIR}/library/*.h
		${CMAKE_SOURCE_DIR}/framework/*.cpp
		${CMAKE_SOURCE_DIR}/framework/*.c
		${CMAKE_SOURCE_DIR}/framework/*.h
		${CMAKE_SOURCE_DIR}/forms/*.cpp
		${CMAKE_SOURCE_DIR}/forms/*.c
		${CMAKE_SOURCE_DIR}/forms/*.h
		${CMAKE_SOURCE_DIR}/game/*.cpp
		${CMAKE_SOURCE_DIR}/game/*.c
		${CMAKE_SOURCE_DIR}/game/*.h
		${CMAKE_SOURCE_DIR}/tools/*.cpp
		${CMAKE_SOURCE_DIR}/tools/*.c
		${CMAKE_SOURCE_DIR}/tools/*.h
		${CMAKE_SOURCE_DIR}/tests/*.cpp
		${CMAKE_SOURCE_DIR}/tests/*.c
		${CMAKE_SOURCE_DIR}/tests/*.h
		)

add_custom_target(format)
add_custom_target(format-sources)
add_dependencies(format format-sources)

set(CLANG_FORMAT clang-format CACHE STRING "clang-format executable name in PATH")

foreach(FORMAT_SOURCE ${FORMAT_SOURCES})
	add_custom_command(TARGET format-sources COMMAND ${CLANG_FORMAT} -i ${FORMAT_SOURCE} DEPENDS ${FORMAT_SOURCE} .clang-format)
endforeach()

file(GLOB_RECURSE XML_FILES ${CMAKE_SOURCE_DIR}/data/*.xml)
file(GLOB_RECURSE FONT_FILES ${CMAKE_SOURCE_DIR}/data/fonts/*.font)
file(GLOB_RECURSE FORM_FILES ${CMAKE_SOURCE_DIR}/data/forms/*.form)
file(GLOB_RECURSE ALIAS_FILES ${CMAKE_SOURCE_DIR}/data/aliases/*.alias)

add_custom_target(format-xml)
add_custom_target(format-forms)
add_custom_target(format-fonts)
add_custom_target(format-aliases)

add_dependencies(format format-xml)
add_dependencies(format format-forms)
add_dependencies(format format-fonts)
add_dependencies(format format-aliases)

execute_process(COMMAND which xmllint OUTPUT_VARIABLE WHICH_XMLLINT_OUTPUT ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
if (WHICH_XMLLINT_OUTPUT)
	SET(XMLLINT_COMMAND ${WHICH_XMLLINT_OUTPUT} CACHE PATH
		"xmllint executable")
	message("Found xmllint ${WHICH_XMLLINT_OUTPUT}")
endif()

if (XMLLINT_COMMAND)
		foreach(XML_FILE ${XML_FILES})
			add_custom_command(TARGET format-xml COMMAND
				${XMLLINT_COMMAND} --format --encode UTF-8
				${XML_FILE}	--output ${XML_FILE}
				DEPENDS ${XML_FILE})
		endforeach()
		foreach(FONT_FILE ${FONT_FILES})
			add_custom_command(TARGET format-fonts COMMAND
				${XMLLINT_COMMAND} --format --encode UTF-8
				${FONT_FILE} --output ${FONT_FILE}
				DEPENDS ${FONT_FILE})
		endforeach()
		foreach(FORM_FILE ${FORM_FILES})
			add_custom_command(TARGET format-forms COMMAND
				${XMLLINT_COMMAND} --format --encode UTF-8
				${FORM_FILE} --output ${FORM_FILE}
				DEPENDS ${FORM_FILE})
		endforeach()
		foreach(ALIAS_FILE ${ALIAS_FILES})
			add_custom_command(TARGET format-aliases COMMAND
				${XMLLINT_COMMAND} --format --encode UTF-8
				${ALIAS_FILE} --output ${ALIAS_FILE}
				DEPENDS ${ALIAS_FILE})
		endforeach()
else()
	message("xmllint not found, disabling format-xml target")
endif()

set(CLANG_TIDY clang-tidy CACHE STRING "clang-tidy executable name in PATH")
set(CLANG_TIDY_FIXES_FILE clang-tidy-fixes.yaml CACHE STRING "clang-tidy
exported fixes file")

find_package(Git)
execute_process(COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} describe
		--tags --long --always
		OUTPUT_VARIABLE OPENAPOC_VERSION_STRING_RAW
		OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE "heads/" "branch/" OPENAPOC_VERSION_STRING_RAW ${OPENAPOC_VERSION_STRING_RAW})
# Remove 'special' character (e.g. '/') as that screws up filenames etc. Abuse
# MAKE_C_IDENTIFIER for this instead of a list of 'banned' chars
string(MAKE_C_IDENTIFIER "${OPENAPOC_VERSION_STRING_RAW}" OPENAPOC_VERSION_STRING )

configure_file(version.h.in version.h)

list(APPEND CLANG_TIDY_OPTIONS
		-export-fixes=${CLANG_TIDY_FIXES_FILE})


add_custom_target(tidy ${CLANG_TIDY}
		${FORMAT_SOURCES} -p ${CMAKE_BINARY_DIR} ${CLANG_TIDY_OPTIONS}
		DEPENDS ${FORMAT_SOURCES} ${CMAKE_BINARY_DIR}/compile_commands.json)

# Localisation

set(LANGUAGES en_GB fr_FR de_DE cs es it pl pt_BR ru_RU uk)

find_package(Gettext)
if (GETTEXT_FOUND)
		foreach(LANG ${LANGUAGES})
				GETTEXT_PROCESS_PO_FILES(${LANG} PO_FILES
						${CMAKE_SOURCE_DIR}/data/languages/ufo_string_${LANG}.po)
		endforeach()
		add_custom_target(update-translations DEPENDS pofiles)
		foreach (LANG ${LANGUAGES})
				add_custom_command(TARGET update-translations DEPENDS pofiles
						COMMAND ${CMAKE_COMMAND} -E copy
						${CMAKE_BINARY_DIR}/ufo_string_${LANG}.gmo
						${CMAKE_SOURCE_DIR}/data/languages/${LANG}/LC_MESSAGES/ufo_string.mo)
		endforeach()
endif(GETTEXT_FOUND)
