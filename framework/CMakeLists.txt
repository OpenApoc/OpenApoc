# project name, and type
PROJECT(OpenApoc_Framework CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

option(BACKTRACE_ON_ERROR "Print backtrace on logging an error (Requires libunwind on linux, no extra dependencies on windows)" ON)
option(DIALOG_ON_ERROR "Pop up a dialog box showing errors" ON)


set (FRAMEWORK_SOURCE_FILES 
	configfile.cpp
	data.cpp
	event.cpp
	font.cpp
	framework.cpp
	image.cpp
	logger.cpp
	palette.cpp
	physfs_fs.cpp
	renderer.cpp
	serialize.cpp
	sound.cpp
	stagestack.cpp
	trace.cpp
	ignorecase.c)

source_group(framework\\sources FILES ${FRAMEWORK_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${FRAMEWORK_SOURCE_FILES})

set (FRAMEWORK_HEADER_FILES
	configfile.h
	data.h
	event.h
	font.h
	framework.h
	fs.h
	ignorecase.h
	image.h
	imageloader_interface.h
	includes.h
	logger.h
	musicloader_interface.h
	palette.h
	renderer.h
	renderer_interface.h
	sampleloader_interface.h
	serialize.h
	sound.h
	sound_interface.h
	stage.h
	stagestack.h
	trace.h
	ThreadPool/ThreadPool.h)

source_group(framework\\headers FILES ${FRAMEWORK_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${FRAMEWORK_HEADER_FILES})

set (APOCRESOURCES_SOURCE_FILES
	apocresources/apocfont.cpp
	apocresources/apocpalette.cpp
	apocresources/cursor.cpp
	apocresources/loftemps.cpp
	apocresources/pck.cpp
	apocresources/rawimage.cpp)

source_group(framework\\apocresources\\sources FILES
		${APOCRESOURCES_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${APOCRESOURCES_SOURCE_FILES})

set (APOCRESOURCES_HEADER_FILES
	apocresources/apocfont.h
	apocresources/apocpalette.h
	apocresources/apocresource.h
	apocresources/cursor.h
	apocresources/loftemps.h
	apocresources/pck.h
	apocresources/rawimage.h)

source_group(framework\\apocresources\\headers FILES
		${APOCRESOURCES_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${APOCRESOURCES_HEADER_FILES})


set (IMAGELOADER_SOURCE_FILES
	imageloader/lodepng.cpp
	imageloader/lodepng_image.cpp
	imageloader/pcx.cpp)

source_group(framework\\imageloader\\sources FILES
		${IMAGELOADER_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${IMAGELOADER_SOURCE_FILES})

set (IMAGELOADER_HEADER_FILES
	imageloader/lodepng.h)

source_group(framework\\imageloader\\headers FILES
	${IMAGELOADER_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${IMAGELOADER_HEADER_FILES})

set (SAMPLELOADER_SOURCE_FILES
	sampleloader/rawsound.cpp)

source_group(framework\\sampleloader\\sources FILES
		${SAMPLELOADER_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${SAMPLELOADER_SOURCE_FILES})

set (SAMPLELOADER_HEADER_FILES)

source_group(framework\\sampleloader\\headers FILES
	${SAMPLELOADER_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${SAMPLELOADER_HEADER_FILES})

set (MUSICLOADER_SOURCE_FILES
	musicloader/music.cpp)

source_group(framework\\musicloader\\sources FILES
		${MUSICLOADER_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${MUSICLOADER_SOURCE_FILES})

set (MUSICLOADER_HEADER_FILES)

source_group(framework\\musicloader\\headers FILES
	${MUSICLOADER_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${MUSICLOADER_HEADER_FILES})

set (SOUND_SOURCE_FILES
	sound/null_backend.cpp
	sound/sdlraw_backend.cpp)

source_group(framework\\sound\\sources FILES
		${SOUND_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${SOUND_SOURCE_FILES})

set (SOUND_HEADER_FILES)

source_group(framework\\sound\\headers FILES
	${SOUND_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${SOUND_HEADER_FILES})

set (RENDERER_SOURCE_FILES
	render/gl20/ogl_2_0_renderer.cpp
	render/gl30/ogl_3_0_renderer.cpp
	render/gles30_v2/ogles_3_0_renderer_v2.cpp
	render/gles30_v2/gleswrap_gles3.cpp)

source_group(framework\\renderer\\sources FILES
	${RENDERER_SOURCE_FILES})
	list(APPEND ALL_SOURCE_FILES ${RENDERER_SOURCE_FILES})

set (RENDERER_HEADER_FILES
	render/gl20/gl_2_0.hpp
	render/gl20/gl_2_0.inl
	render/gl30/gl_3_0.hpp
	render/gl30/gl_3_0.inl
	render/gles30_v2/gleswrap_gles3.h
	render/gles30_v2/gleswrap.h
	render/gles30_v2/stb_rect_pack.h
	)

source_group(framework\\renderer\\headers FILES
	${RENDERER_HEADER_FILES})
	list(APPEND ALL_HEADER_FILES ${RENDERER_HEADER_FILES})

add_library(OpenApoc_Framework STATIC
	${FRAMEWORK_SOURCE_FILES} ${FRAMEWORK_HEADER_FILES}
	${APOCRESOURCES_SOURCE_FILES} ${APOCRESOURCES_HEADER_FILES}
	${IMAGELOADER_SOURCE_FILES} ${IMAGELOADER_HEADER_FILES}
	${SAMPLELOADER_SOURCE_FILES} ${SAMPLELOADER_HEADER_FILES}
	${MUSICLOADER_SOURCE_FILES} ${MUSICLOADER_HEADER_FILES}
	${SOUND_SOURCE_FILES} ${SOUND_HEADER_FILES}
	${RENDERER_SOURCE_FILES} ${RENDERER_HEADER_FILES})

set_property(TARGET OpenApoc_Framework PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_Framework PROPERTY CXX_STANDARD_REQUIRED ON)

target_compile_definitions(OpenApoc_Framework PUBLIC
		"-DRENDERERS=\"GLES_3_0:GL_3_0:GL_2_0\""
		"-DGLESWRAP_PLATFORM_GLX")

# We use boost::locale for utf conversions and boost::filesystem for
# serialization
find_package(Boost REQUIRED COMPONENTS locale filesystem system)
target_link_libraries(OpenApoc_Framework PUBLIC ${Boost_LIBRARIES})
target_include_directories(OpenApoc_Framework PUBLIC ${Boost_INCLUDE_DIRS})

find_package(Threads)
target_link_libraries(OpenApoc_Framework PUBLIC ${CMAKE_THREAD_LIBS_INIT})


#Require the glm matrix/vector maths library
FIND_PATH(GLM_INCLUDE_DIR glm/glm.hpp)
MARK_AS_ADVANCED(GLM_INCLUDE_DIR)
if (NOT GLM_INCLUDE_DIR)
		message(FATAL_ERROR "GLM headers not found")
endif()

target_include_directories(OpenApoc_Framework PUBLIC ${GLM_INCLIDE_DIR})

find_package(PkgConfig)

#Require SDL2 for input/graphics/sound/pretty much everything
pkg_check_modules(PC_SDL2 REQUIRED sdl2>=2.0)
if (NOT PC_SDL2_FOUND)
		message(FATAL_ERROR "sdl2 not found")
endif()
foreach (SDL2_LIB ${PC_SDL2_LIBRARIES})
		message("Searching for ${SDL2_LIB} in ${PC_SDL2_LIBRARY_DIRS}")
		find_library(SDL2_LIBRARY_PATH-${SDL2_LIB} ${SDL2_LIB} HINTS ${PC_SDL2_LIBRARY_DIRS})
		if (NOT SDL2_LIBRARY_PATH-${SDL2_LIB})
				message(FATAL_ERROR "sdl2 library ${SDL2_LIB} not found in ${PC_SDL2_LIBRARY_DIRS}")
		endif()
		message("Found ${SDL2_LIB} at ${SDL2_LIBRARY_PATH-${SDL2_LIB}}")
		target_link_libraries(OpenApoc_Framework PUBLIC ${SDL2_LIB})
endforeach()
target_include_directories(OpenApoc_Framework PUBLIC ${PC_SDL2_INCLUDE_DIRS})

# Dialog is handled by SDL2
if(DIALOG_ON_ERROR)
		target_compile_definitions(OpenApoc_Framework PUBLIC -DERROR_DIALOG)
endif()

# Backtrace required libunwind
if(BACKTRACE_ON_ERROR)
	pkg_check_modules(PC_UNWIND libunwind)
	if (NOT PC_UNWIND_FOUND)
		#Ubuntu 12.04 libunwind doesn't have a pkgconfig - try 'current' paths anyway
		find_path(UNWIND_INCLUDE_DIR libunwind.h HINTS ${FRAMEWORK_INCLUDE_DIRS})
		if (NOT UNWIND_INCLUDE_DIR)
				message(FATAL_ERROR "Libunwind not found")
		endif()
		target_link_libraries(OpenApoc_Framework PUBLIC unwind dl)
		#HACK - this assumes the library path is already searched?
	else()
		find_path(UNWIND_INCLUDE_DIR libunwind.h HINTS ${PC_UNWIND_INCLUDEDIR})
		target_link_libraries(OpenApoc_Framework PUBLIC ${PC_UNWIND_LIBRARIES} dl)
	endif()
	target_compile_definitions(OpenApoc_Framework PUBLIC -DBACKTRACE_LIBUNWIND)
	target_include_directories(OpenApoc_Framework PUBLIC ${UNWIND_INCLUDE_DIR})
endif()

# OpenGL is required for any renderer
find_package(OpenGL)
if (NOT OPENGL_FOUND)
		message(WARNING "OpenGL not found for GL_2_0 renderer")
else()
	target_link_libraries(OpenApoc_Framework PUBLIC ${OPENGL_gl_LIBRARY})
	target_include_directories(OpenApoc_Framework PUBLIC ${OPENGL_INCLUDE_DIR})
endif()
target_link_libraries(OpenApoc_Framework PUBLIC OpenApoc_Library)

# Physfs provides .iso reader & overlays etc.
pkg_check_modules(PC_PHYSFS REQUIRED physfs>=2.0.0)

find_path(PHYSFS_INCLUDE_DIR physfs.h HINTS ${PC_PHYSFS_INCLUDEDIR})
target_include_directories(OpenApoc_Framework PUBLIC ${PHYFSFS_INCLUDE_DIR})

foreach (PHYSFS_LIBRARY ${PC_PHYSFS_LIBRARIES})
	find_library(PHYSFS_LIBRARY_PATH ${PHYSFS_LIBRARY} HINTS
			${PC_PHYSFS_LIBRARY_DIRS})
	if (NOT PHYSFS_LIBRARY_PATH)
			message(FATAL_ERROR "Failed to find physfs library ${PHYSFS_LIBRARY} in ${PC_PHYSFS_LIBRARY_DIRS}")
	endif()
	target_link_libraries(OpenApoc_Framework PUBLIC ${PHYSFS_LIBRARY_PATH})
endforeach(PHYSFS_LIBRARY)

target_include_directories(OpenApoc_Framework PUBLIC ${CMAKE_SOURCE_DIR})
