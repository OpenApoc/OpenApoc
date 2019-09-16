# project name, and type
PROJECT(OpenApoc_Forms CXX C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

set (FORMS_SOURCE_FILES
	checkbox.cpp
	control.cpp
	form.cpp
	graphicbutton.cpp
	graphic.cpp
	label.cpp
	listbox.cpp
	multilistbox.cpp
	radiobutton.cpp
	scrollbar.cpp
	textbutton.cpp
	textedit.cpp
	ticker.cpp
	tristatebox.cpp
	ui.cpp)
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
	listbox.h
	multilistbox.h
	radiobutton.h
	scrollbar.h
	textbutton.h
	textedit.h
	ticker.h
	tristatebox.h
	ui.h)

source_group(forms\\headers FILES ${LIBRARY_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBRARY_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBRARY_HEADER_FILES})

add_library(OpenApoc_Forms STATIC ${FORMS_SOURCE_FILES}
		${FORMS_HEADER_FILES})

target_link_libraries(OpenApoc_Forms PUBLIC OpenApoc_Framework
		OpenApoc_LibPugixml)

target_include_directories(OpenApoc_Forms PUBLIC ${CMAKE_SOURCE_DIR})
