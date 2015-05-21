cmake_minimum_required(VERSION 2.8)

option(BACKTRACE_ON_ERROR "Print backtrace on logging an error (Requires libunwind on linux)" ON)

INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	if (COMPILER_SUPPORTS_CXX0X)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
		message(WARNING "Your C++ compiler only supports a draft C++11 standard. This /may/ not work...")
	else()
		message(FATAL_ERROR "Your C++ compiler does not support C++11.")
	endif()
endif()

unset(FRAMEWORK_SOURCES)
unset(FRAMEWORK_INCLUDE_DIRS)
unset(FRAMEWORK_LIBRARIES)
unset(FRAMEWORK_ALLEGRO_LIBRARIES)

# Font, Audio and Window modularity is still TODO
list(APPEND FRAMEWORK_ALLEGRO_LIBRARIES allegro allegro_ttf allegro_font allegro_audio allegro_acodec)

aux_source_directory(framework FRAMEWORK_SOURCES)

include(renderer)
list(APPEND FRAMEWORK_SOURCES ${RENDERER_SOURCES})
list(APPEND FRAMEWORK_INCLUDE_DIRS ${RENDERER_INCLUDE_DIRS})
list(APPEND FRAMEWORK_LIBRARIES ${RENDERER_LIBRARIES})

include(imageloader)
list(APPEND FRAMEWORK_SOURCES ${IMAGELOADER_SOURCES})
list(APPEND FRAMEWORK_INCLUDE_DIRS ${IMAGELOADER_INCLUDE_DIRS})
list(APPEND FRAMEWORK_LIBRARIES ${IMAGELOADER_LIBRARIES})

include(soundbackend)
list(APPEND FRAMEWORK_SOURCES ${SOUNDBACKEND_SOURCES})
list(APPEND FRAMEWORK_INCLUDE_DIRS ${SOUNDBACKEND_INCLUDE_DIRS})
list(APPEND FRAMEWORK_LIBRARIES ${SOUNDBACKEND_LIBRARIES})

find_package(PkgConfig)

if(BACKTRACE_ON_ERROR)
		pkg_check_modules(PC_UNWIND libunwind)
		if (NOT PC_UNWIND_FOUND)
				#Ubuntu 12.04 libunwind doesn't have a pkgconfig - try 'current' paths anyway
				find_path(UNWIND_INCLUDE_DIR libunwind.h HINTS ${FRAMEWORK_INCLUDE_DIRS})
				if (NOT UNWIND_INCLUDE_DIR)
						message(FATAL_ERROR "Libunwind not found")
				endif()
				list(APPEND FRAMEWORK_LIBRARIES unwind dl)
				#HACK - this assumes the library path is already searched?
		else()
			find_path(UNWIND_INCLUDE_DIR libunwind.h HINTS ${PC_UNWIND_INCLUDEDIR})
			list(APPEND FRAMEWORK_INCLUDE_DIRS ${UNWIND_INCLUDE_DIR})
			list(APPEND FRAMEWORK_LIBRARIES ${PC_UNWIND_LIBRARIES} dl)
		endif()
		add_definitions(-DBACKTRACE_LIBUNWIND)
		#FIXME: Add Windows support for cmake? (BACKTRACE_WINDOWS?)
endif()

pkg_check_modules(PC_PHYSFS REQUIRED physfs>=2.0.0)

find_path(PHYSFS_INCLUDE_DIR physfs.h HINTS ${PC_PHYSFS_INCLUDEDIR})
list(APPEND FRAMEWORK_INCLUDE_DIRS ${PHYSFS_INCLUDE_DIR})

foreach (PHYSFS_LIBRARY ${PC_PHYSFS_LIBRARIES})
	find_library(PHYSFS_LIBRARY_PATH ${PHYSFS_LIBRARY} HINTS
			${PC_PHYSFS_LIBRARY_DIRS})
	if (NOT PHYSFS_LIBRARY_PATH)
			message(FATAL_ERROR "Failed to find physfs library ${PHYSFS_LIBRARY} in ${PC_PHYSFS_LIBRARY_DIRS}")
	endif()
	list(APPEND PHYSFS_LIBRARIES ${PHYSFS_LIBRARY_PATH})
endforeach(PHYSFS_LIBRARY)

list(APPEND FRAMEWORK_LIBRARIES ${PHYSFS_LIBRARIES})

pkg_check_modules(PC_ICU icu-uc)
pkg_check_modules(PC_ICU_IO icu-io)
if (NOT PC_ICU_FOUND OR NOT PC_ICU_IO_FOUND)
		message(WARNING "Failed to find ICU pkgconfig - trying to include/link anyway...")
	#Ubuntu 12.04 icu doesn't have a pkgconfig - try 'current' paths anyway
	find_path(ICU_INCLUDE_DIR unicode/unistr.h HINTS ${FRAMEWORK_INCLUDE_DIRS})
	find_path(ICU_IO_INCLUDE_DIR unicode/ustdio.h HINTS ${FRAMEWORK_INCLUDE_DIRS})
	if (NOT ICU_INCLUDE_DIR OR NOT ICU_IO_INCLUDE_DIR)
		message(FATAL_ERROR "libicu not found")
	endif()
	#HACK - this assumes the library path is already searched?
	list(APPEND FRAMEWORK_LIBRARIES icuuc)
	list(APPEND FRAMEWORK_LIBRARIES icudata)
	list(APPEND FRAMEWORK_LIBRARIES icuio)
else()
	find_path(ICU_INCLUDE_DIR unicode/unistr.h HINTS ${PC_ICU_INCLUDEDIR})
	list(APPEND FRAMEWORK_INCLUDE_DIRS ${ICU_INCLUDE_DIR})
	find_path(ICU_IO_INCLUDE_DIR unicode/ustdio.h HINTS ${PC_ICU_IO_INCLUDEDIR})
	list(APPEND FRAMEWORK_INCLUDE_DIRS ${ICU_IO_INCLUDE_DIR})

	list(APPEND FRAMEWORK_LIBRARIES ${PC_ICU_LIBRARIES} ${PC_ICU_IO_LIBRARIES})
	list(APPEND FRAMEWORK_LIBRARIES ${ICU_LIBRARIES})
endif()
set(ALLEGRO_VERSIONS 5.1 5 5.0)

foreach (ALLEGRO_MODULE ${FRAMEWORK_ALLEGRO_LIBRARIES})
	message("Searching for allegro module ${ALLEGRO_MODULE}")
	unset(ALLEGRO_MODULE_NAMES)
	foreach (ALLEGRO_VERSION ${ALLEGRO_VERSIONS})
			list(APPEND ALLEGRO_MODULE_NAMES
					${ALLEGRO_MODULE}-${ALLEGRO_VERSION})
	endforeach(ALLEGRO_VERSION)
	pkg_search_module(PC_ALLEGRO_MODULE-${ALLEGRO_MODULE} REQUIRED ${ALLEGRO_MODULE_NAMES})

	foreach (ALLEGRO_LIBRARY_NAME ${PC_ALLEGRO_MODULE-${ALLEGRO_MODULE}_LIBRARIES})
			find_library(ALLEGRO_LIBRARY-${ALLEGRO_MODULE} ${ALLEGRO_LIBRARY_NAME} HINTS
					${PC_ALLEGRO_MODULE-${ALLEGRO_MODULE}_LIBRARY_DIRS})
			if (NOT ALLEGRO_LIBRARY-${ALLEGRO_MODULE})
					message(FATAL_ERROR "Failed to find allegro library ${ALLEGRO_LIBRARY_NAME} in ${PC_ALLEGRO_MODULE-${ALLEGRO_MODULE}_LIBRARY_DIRS}")
			endif()
			list(APPEND ALLEGRO_LIBRARIES
					${ALLEGRO_LIBRARY-${ALLEGRO_MODULE}})
	endforeach(ALLEGRO_LIBRARY_NAME)
endforeach(ALLEGRO_MODULE)

list(APPEND FRAMEWORK_LIBRARIES ${ALLEGRO_LIBRARIES})
