find_program(CCACHE ccache)

option(ENABLE_CCACHE "Use CCache build object caching" ON)

if(CCACHE AND ENABLE_CCACHE)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})

		# Clang doesn't like 'unused' arguments, such as when ccache still passed through the '-I' options while requesting no preprocessor.
		if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
		endif()
		if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
		endif()
endif()
