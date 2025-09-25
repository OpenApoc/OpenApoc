# project name, and type
PROJECT(OpenApoc_Library CXX C)

find_package(Boost REQUIRED COMPONENTS locale)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set (LIBRARY_SOURCE_FILES
	colour.cpp
	strings.cpp
	voxel.cpp
	backtrace.cpp)
source_group(library\\sources FILES ${LIBRARY_SOURCE_FILES})
set (LIBRARY_HEADER_FILES
	colour.h
	enum_traits.h
	rect.h
	sp.h
	strings.h
	strings_format.h
	vec.h
	resource.h
	voxel.h
	line.h
	xorshift.h
	backtrace.h)
source_group(library\\headers FILES ${LIBRARY_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBRARY_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBRARY_HEADER_FILES})

add_library(OpenApoc_Library STATIC ${LIBRARY_SOURCE_FILES}
		${LIBRARY_HEADER_FILES})

target_link_libraries(OpenApoc_Library PUBLIC ${Boost_LIBRARIES}
		Threads::Threads fmt::fmt)

target_include_directories(OpenApoc_Library PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_Library PUBLIC ${GLM_INCLUDE_DIR})
target_include_directories(OpenApoc_Library PUBLIC ${Boost_INCLUDE_DIRS})
if(NOT WIN32)
	target_compile_definitions(OpenApoc_Library PUBLIC PTHREADS_AVAILABLE)
endif()

target_compile_definitions(OpenApoc_Library PUBLIC _USE_MATH_DEFINES)


option(ENABLE_BACKTRACE "Enable gathering backtraces on some errors to aid debugging (Requires libunwind on linux, no extra dependencies on windows or macos)" ON)

if(ENABLE_BACKTRACE)
	if(WIN32)
			target_link_libraries(OpenApoc_Library PUBLIC dbghelp)
	elseif(APPLE)
		# MacOS always has libunwind with no extra libs needed
		target_compile_definitions(OpenApoc_Library PUBLIC -DBACKTRACE_LIBUNWIND)
	else()
		find_package(PkgConfig)
		pkg_check_modules(PC_UNWIND libunwind)
		if (NOT PC_UNWIND_FOUND)
			#Ubuntu 12.04 libunwind doesn't have a pkgconfig - try 'current' paths anyway
			find_path(UNWIND_INCLUDE_DIR libunwind.h)
			if (NOT UNWIND_INCLUDE_DIR)
				message(FATAL_ERROR "Libunwind not found")
			endif()
			target_link_libraries(OpenApoc_Library PUBLIC unwind dl)
			#HACK - this assumes the library path is already searched?
		else()
			find_path(UNWIND_INCLUDE_DIR libunwind.h HINTS ${PC_UNWIND_INCLUDEDIR})
			target_link_libraries(OpenApoc_Library PUBLIC ${PC_UNWIND_LINK_LIBRARIES} ${CMAKE_DL_LIBS})
		endif()
		target_compile_definitions(OpenApoc_Library PUBLIC -DBACKTRACE_LIBUNWIND)
		target_include_directories(OpenApoc_Library PUBLIC ${UNWIND_INCLUDE_DIR})
	endif()
endif()

if (USE_PCH)
	target_precompile_headers(OpenApoc_Library PUBLIC library_pch.h)
endif()