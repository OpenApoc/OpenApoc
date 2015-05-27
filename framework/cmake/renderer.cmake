cmake_minimum_required(VERSION 2.8)

option (BUILD_GL_3_0_RENDERER "Build an OpenGL 3.0 renderer backend" ON)
option (BUILD_ALLEGRO_RENDERER "Build an allegro renderer backend" OFF)

set (RENDERERS "")
set (RENDERER_SOURCES "")
set (RENDERER_LIBRARIES "")
set (RENDERER_INCLUDE_DIRS "")

if(BUILD_GL_3_0_RENDERER)
		list(APPEND RENDERERS GL_3_0)
		list(APPEND RENDERER_SOURCES "framework/render/ogl_3_0_renderer.cpp")
		find_package(OpenGL)
		if (NOT OPENGL_FOUND)
				message(FATAL_ERROR "OpenGL not found for GL_3_0 renderer")
		endif()

		list(APPEND RENDERER_LIBRARIES ${OPENGL_gl_LIBRARY})
		list(APPEND RENDERER_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})

endif()

if(BUILD_ALLEGRO_RENDERER)
		list(APPEND RENDERERS allegro)
		list(APPEND RENDERER_SOURCES "framework/render/allegro_renderer.cpp")
		list(APPEND FRAMEWORK_ALLEGRO_LIBS allegro)
endif()

if (NOT RENDERERS)
		message(FATAL_ERROR "No renderers specified")
endif()

message("Building renderers: ${RENDERERS}")
