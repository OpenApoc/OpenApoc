# project name, and type
PROJECT(OpenApoc_Framework CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

option(DIALOG_ON_ERROR "Pop up a dialog box showing errors" ON)


set (FRAMEWORK_SOURCE_FILES
	configfile.cpp
	data.cpp
	event.cpp
	font.cpp
	framework.cpp
	image.cpp
	jukebox.cpp
	logger.cpp
	luaframework.cpp
	palette.cpp
	physfs_fs.cpp
	renderer.cpp
	serialization/serialize.cpp
	serialization/providers/filedataprovider.cpp
	serialization/providers/providerwithchecksum.cpp
	serialization/providers/zipdataprovider.cpp
	sound.cpp
	stagestack.cpp
	video/smk.cpp
	logger_sdldialog.cpp
	logger_file.cpp
	options.cpp
	modinfo.cpp)

source_group(framework\\sources FILES ${FRAMEWORK_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${FRAMEWORK_SOURCE_FILES})

set (FRAMEWORK_HEADER_FILES
	configfile.h
	data.h
	event.h
	font.h
	framework.h
	filesystem.h
	fs.h
	image.h
	imageloader_interface.h
	jukebox.h
	keycodes.h
	logger.h
	luaframework.h
	musicloader_interface.h
	palette.h
	renderer.h
	renderer_interface.h
	sampleloader_interface.h
	serialization/serialize.h
	serialization/providers/filedataprovider.h
	serialization/providers/providerwithchecksum.h
	serialization/providers/zipdataprovider.h
	serialization/providers/serializationdataprovider.h
	sound.h
	sound_interface.h
	stage.h
	stagestack.h
	ThreadPool/ThreadPool.h
	video.h
	logger_sdldialog.h
	logger_file.h
	options.h
	modinfo.h)

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
	apocresources/cursor.h
	apocresources/loftemps.h
	apocresources/pck.h
	apocresources/rawimage.h)

source_group(framework\\apocresources\\headers FILES
		${APOCRESOURCES_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${APOCRESOURCES_HEADER_FILES})


set (IMAGELOADER_SOURCE_FILES
	imageloader/lodepng_image.cpp
	imageloader/pcx.cpp)

source_group(framework\\imageloader\\sources FILES
		${IMAGELOADER_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${IMAGELOADER_SOURCE_FILES})

set (IMAGELOADER_HEADER_FILES
	)

source_group(framework\\imageloader\\headers FILES
	${IMAGELOADER_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${IMAGELOADER_HEADER_FILES})

set (SAMPLELOADER_SOURCE_FILES
	sampleloader/rawsound.cpp
	sampleloader/wavsound.cpp)

source_group(framework\\sampleloader\\sources FILES
		${SAMPLELOADER_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${SAMPLELOADER_SOURCE_FILES})

set (SAMPLELOADER_HEADER_FILES)

source_group(framework\\sampleloader\\headers FILES
	${SAMPLELOADER_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${SAMPLELOADER_HEADER_FILES})

set (MUSICLOADER_SOURCE_FILES
	musicloader/music.cpp
	musicloader/vorbis.cpp)

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
	render/gles30_v2/ogles_3_0_renderer_v2.cpp
	render/gles30_v2/gleswrap_gles3.cpp)

source_group(framework\\renderer\\sources FILES
	${RENDERER_SOURCE_FILES})
	list(APPEND ALL_SOURCE_FILES ${RENDERER_SOURCE_FILES})

set (RENDERER_HEADER_FILES
	render/gl20/gl_2_0.hpp
	render/gl20/gl_2_0.inl
	render/gles30_v2/gleswrap_gles3.h
	render/gles30_v2/gleswrap.h
	render/gles30_v2/stb_rect_pack.h
	)

source_group(framework\\renderer\\headers FILES
	${RENDERER_HEADER_FILES})
	list(APPEND ALL_HEADER_FILES ${RENDERER_HEADER_FILES})

set (FILESYSTEM_SOURCE_FILES
	fs/physfs_archiver_cue.cpp)

source_group(framework\\fs\\sources FILES ${FILESYSTEM_SOURCE_FILES})
list(APPEND ALL_SOURCE_FILES ${FILESYSTEM_SOURCE_FILES})

set (FILESYSTEM_HEADER_FILES
	fs/physfs_archiver_cue.h)

source_group(framework\\fs\\headers FILES ${FILESYSTEM_HEADER_FILES})
list(APPEND ALL_HEADER_FILES ${FILESYSTEM_HEADER_FILES})

add_library(OpenApoc_Framework STATIC
	${FRAMEWORK_SOURCE_FILES} ${FRAMEWORK_HEADER_FILES}
	${APOCRESOURCES_SOURCE_FILES} ${APOCRESOURCES_HEADER_FILES}
	${IMAGELOADER_SOURCE_FILES} ${IMAGELOADER_HEADER_FILES}
	${SAMPLELOADER_SOURCE_FILES} ${SAMPLELOADER_HEADER_FILES}
	${MUSICLOADER_SOURCE_FILES} ${MUSICLOADER_HEADER_FILES}
	${SOUND_SOURCE_FILES} ${SOUND_HEADER_FILES}
	${RENDERER_SOURCE_FILES} ${RENDERER_HEADER_FILES}
	${FILESYSTEM_SOURCE_FILES} ${FILESYSTEM_HEADER_FILES})

target_compile_definitions(OpenApoc_Framework PUBLIC
		"-DRENDERERS=\"GLES_3_0:GL_2_0\"")

if(APPLE)
	target_compile_definitions(OpenApoc_Framework PRIVATE
			"-DGLESWRAP_PLATFORM_MACHO")
	# OSX doesn't seem to support the c++11 'thread_local' keyword
	target_compile_definitions(OpenApoc_Framework PRIVATE
			"-DBROKEN_THREAD_LOCAL")
elseif(WIN32)
	target_compile_definitions(OpenApoc_Framework PRIVATE "-DGLESWRAP_PLATFORM_WGL")
else()
	target_compile_definitions(OpenApoc_Framework PRIVATE "-DGLESWRAP_PLATFORM_GLX")
endif()

# We use boost::locale for utf conversions and boost::filesystem for
# serialization

set(BOOST_PACKAGES locale system program_options)
# Arbitrary version that I remember testing previously
set(BOOST_VERSION 1.50)

set(USE_STD_FILESYSTEM FALSE)

if(CMAKE_COMPILER_IS_GNUCC)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
			set(USE_STD_FILESYSTEM TRUE)
	endif()
endif()
if(MSVC)
	if (MSVC_VERSION GREATER_EQUAL 1910)
		set(USE_STD_FILESYSTEM TRUE)
	endif()
endif()

if(USE_STD_FILESYSTEM)
    message(STATUS "Using <filesystem>")
	target_compile_definitions(OpenApoc_Framework PUBLIC "-DUSE_STD_FILESYSTEM")
else()
	message(STATUS "Using <boost/filesystem>")
	set(BOOST_PACKAGES ${BOOST_PACKAGES} filesystem)
	# filesystem::relative() didn't exist until 1.60 from what I can see in the docs
	set(BOOST_VERSION 1.60)
	target_compile_definitions(OpenApoc_Framework PUBLIC "-DUSE_BOOST_FILESYSTEM")
endif()

find_package(Boost ${BOOST_VERSION} REQUIRED COMPONENTS ${BOOST_PACKAGES})
target_link_libraries(OpenApoc_Framework PUBLIC ${Boost_LIBRARIES})
target_include_directories(OpenApoc_Framework PUBLIC ${Boost_INCLUDE_DIRS})

find_package(Threads)
target_link_libraries(OpenApoc_Framework PUBLIC ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(OpenApoc_Framework PUBLIC ${GLM_INCLIDE_DIR})

#Require SDL2 for input/graphics/sound/pretty much everything
if(WIN32)
	find_package(SDL2 CONFIG REQUIRED)
	target_link_libraries(OpenApoc_Framework PUBLIC SDL2::SDL2 SDL2::SDL2main)
	#Workaround vcpkg bug
	set_target_properties(SDL2::SDL2 SDL2::SDL2main PROPERTIES
		MAP_IMPORTED_CONFIG_MINSIZEREL Release
		MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
	)
else()
	find_package(PkgConfig)
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
			target_link_libraries(OpenApoc_Framework PUBLIC ${SDL2_LIBRARY_PATH-${SDL2_LIB}})
	endforeach()
	target_include_directories(OpenApoc_Framework PUBLIC ${PC_SDL2_INCLUDE_DIRS})
endif()

find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)
find_library(VORBISFILE_LIBRARY vorbisfile)

target_include_directories(OpenApoc_Framework PRIVATE ${VORBIS_INCLUDE_DIR})
target_link_libraries(OpenApoc_Framework PRIVATE ${VORBISFILE_LIBRARY})

# Dialog is handled by SDL2
if(DIALOG_ON_ERROR)
	target_compile_definitions(OpenApoc_Framework PUBLIC -DERROR_DIALOG)
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
target_link_libraries(OpenApoc_Framework PUBLIC OpenApoc_LibSmacker)
target_link_libraries(OpenApoc_Framework PUBLIC OpenApoc_LibPugixml)
target_link_libraries(OpenApoc_Framework PUBLIC OpenApoc_LibLodepng)
target_link_libraries(OpenApoc_Framework PUBLIC OpenApoc_Miniz)
target_link_libraries(OpenApoc_Framework PUBLIC ${CMAKE_DL_LIBS})

target_link_libraries(OpenApoc_Framework PUBLIC physfs-static)

target_include_directories(OpenApoc_Framework PRIVATE
		${CMAKE_SOURCE_DIR}/dependencies/physfs/src)
target_include_directories(OpenApoc_Framework PUBLIC ${CMAKE_SOURCE_DIR})
