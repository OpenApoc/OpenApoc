# project name, and type
PROJECT(OpenApoc CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

find_package(Boost REQUIRED COMPONENTS locale)

set (LIBRARY_SOURCE_FILES strings.cpp)
source_group(library\\sources FILES ${LIBRARY_SOURCE_FILES})
set (LIBRARY_HEADER_FILES colour.h rect.h sp.h strings.h vec.h)
source_group(library\\headers FILES ${LIBRARY_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${LIBRARY_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${LIBRARY_HEADER_FILES})

#Require the glm matrix/vector maths library
FIND_PATH(GLM_INCLUDE_DIR glm/glm.hpp)
MARK_AS_ADVANCED(GLM_INCLUDE_DIR)
if (NOT GLM_INCLUDE_DIR)
		message(FATAL_ERROR "GLM headers not found")
endif()

add_library(OpenApoc_Library STATIC ${LIBRARY_SOURCE_FILES}
		${LIBRARY_HEADER_FILES})

target_link_libraries(OpenApoc_Library PUBLIC ${Boost_LIBRARIES})

target_include_directories(OpenApoc_Library PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(OpenApoc_Library PUBLIC ${GLM_INCLUDE_DIR})
target_include_directories(OpenApoc_Library PUBLIC ${Boost_INCLUDE_DIRS})

set_property(TARGET OpenApoc_Library PROPERTY CXX_STANDARD 11)
set_property(TARGET OpenApoc_Library PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_definitions(OpenApoc_Library PUBLIC PTHREADS_AVAILABLE)
