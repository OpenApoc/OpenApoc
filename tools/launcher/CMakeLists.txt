# project name, and type
PROJECT(OpenApoc_Launcher CXX C)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)


set (LAUNCHER_SOURCE_FILES
	main.cpp
	launcherwindow.cpp
	launcherwindow.ui)

set(sources ${sources} "apocicon.rc")

list(APPEND ALL_SOURCE_FILES ${LAUNCHER_SOURCE_FILES})

add_executable(OpenApoc_Launcher WIN32 ${LAUNCHER_SOURCE_FILES} ${sources})

target_link_libraries(OpenApoc_Launcher Qt5::Widgets)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

target_link_libraries(OpenApoc_Launcher OpenApoc_Library)
target_link_libraries(OpenApoc_Launcher OpenApoc_Framework)

