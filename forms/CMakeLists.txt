# project name, and type
PROJECT(OpenApoc CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set (FORMS_SOURCE_FILES
	checkbox.cpp
	control.cpp
	form.cpp
	graphicbutton.cpp
	graphic.cpp
	label.cpp
	list.cpp
	radiobutton.cpp
	scrollbar.cpp
	textbutton.cpp
	textedit.cpp)
source_group(forms\\sources FILES ${LIBRARY_SOURCE_FILES})

set (FORMS_HEADER_FILES
	checkbox.h
	control.h
	form.h
	forms_enums.h
	forms.h
	graphicbutton.h
	graphic.h
	label.h
	list.h
	radiobutton.h
	scrollbar.h
	textbutton.h
	textedit.h)

source_group(forms\\headers FILES ${LIBRARY_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBRARY_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBRARY_HEADER_FILES})

add_library(OpenApoc_Forms STATIC ${FORMS_SOURCE_FILES}
		${FORMS_HEADER_FILES})

target_link_libraries(OpenApoc_Forms PUBLIC OpenApoc_Framework)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_TINYXML2 REQUIRED tinyxml2)

foreach (TINYXML2_LIBRARY ${PC_TINYXML2_LIBRARIES})
	find_library(TINYXML2_LIBRARY_PATH ${TINYXML2_LIBRARY} HINTS
		${PC_TINYXML2_LIBRARY_DIRS})
	if (NOT TINYXML2_LIBRARY_PATH)
		message(FATAL_ERROR "Failed to find tinyxml2 library
		${TINYXML2_LIBRARY} in ${PC_TINYXML2_LIBRARY_DIRS}")
	endif()
	target_link_libraries(OpenApoc_Forms PUBLIC ${TINYXML2_LIBRARY_PATH})
endforeach(TINYXML2_LIBRARY)
target_include_directories(OpenApoc_Forms PUBLIC ${PC_TINYXML2_INCLUDEDIR})

target_include_directories(OpenApoc_Forms PUBLIC ${CMAKE_SOURCE_DIR})

set_property(TARGET OpenApoc_Forms PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_Forms PROPERTY CXX_STANDARD_REQUIRED ON)
