# project name, and type
PROJECT(OpenApoc_FormLocalisation CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set (FORMLOCALISATION_SOURCE_FILES
	main.cpp)

source_group(formlocalisation\\sources FILES ${FORMLOCALISATION_SOURCE_FILES})

list(APPEND ALL_SOURCE_FILES ${FORMLOCALISATION_SOURCE_FILES})

add_executable(OpenApoc_FormLocalisation ${FORMLOCALISATION_SOURCE_FILES}
		${FORMLOCALISATION_HEADER_FILES})

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_FormLocalisation OpenApoc_LibPugixml)

target_include_directories(OpenApoc_FormLocalisation PUBLIC ${CMAKE_SOURCE_DIR})

file(GLOB_RECURSE FORM_FILES ${CMAKE_SOURCE_DIR}/data/forms/*.form)

set(FORM_POT_FILE ${CMAKE_SOURCE_DIR}/data/languages/forms.pot)


add_custom_command(
		OUTPUT ${FORM_POT_FILE}
		COMMAND OpenApoc_FormLocalisation ${FORM_FILES} > ${FORM_POT_FILE}
		DEPENDS ${FORM_FILES})

add_custom_target(extract-form-translations
		DEPENDS ${FORM_POT_FILE})
