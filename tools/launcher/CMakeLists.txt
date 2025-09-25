# project name, and type
PROJECT(OpenApoc_Launcher CXX C)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

qt_standard_project_setup()

set (LAUNCHER_SOURCE_FILES
	main.cpp
	launcherwindow.cpp
	launcherwindow.ui)

set(sources ${sources} "apocicon.rc")

list(APPEND ALL_SOURCE_FILES ${LAUNCHER_SOURCE_FILES})

qt_add_executable(OpenApoc_Launcher ${LAUNCHER_SOURCE_FILES} ${sources})

target_link_libraries(OpenApoc_Launcher PRIVATE Qt6::Widgets)

target_link_libraries(OpenApoc_Launcher PRIVATE OpenApoc_Library)
target_link_libraries(OpenApoc_Launcher PRIVATE OpenApoc_Framework)

set_target_properties(OpenApoc_Launcher PROPERTIES
		WIN32_EXECUTABLE ON
		MACOSX_BUNDLE ON
)

set_target_properties(OpenApoc_Launcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
	find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
	if(NOT WINDEPLOYQT_EXECUTABLE)
		message(FATAL_ERROR "windeployqt not found")
	endif()
	add_custom_command(TARGET OpenApoc_Launcher POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E
			env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
			--verbose 0
			--no-compiler-runtime
			--no-opengl-sw
			$<TARGET_FILE:OpenApoc_Launcher>
			COMMENT "Deploying Qt..."
	)
endif(WIN32)

