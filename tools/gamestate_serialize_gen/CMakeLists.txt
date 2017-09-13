# project name, and type
PROJECT(OpenApoc_GamestateSerializeGen CXX C)

# check cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

if (ANDROID)
    # We rely on gamestate_serialize_gen being already built
    message("Not building generator during cross compilation")

else()
    find_package(Boost REQUIRED COMPONENTS program_options)

    set (GAMESTATESERIALIZEGEN_SOURCE_FILES
	    main.cpp)

    source_group(gamestateserailizegen\\sources FILES ${GAMESTATESERIALIZEGEN_SOURCE_FILES})

    set (GAMESTATESERIALIZEGEN_HEADER_FILES)

    source_group(gamestateserailizegen\\headers FILES ${GAMESTATESERIALIZEGEN_HEADER_FILES})

    list(APPEND ALL_SOURCE_FILES ${GAMESTATESERIALIZEGEN_SOURCE_FILES})
    list(APPEND ALL_HEADER_FILES ${GAMESTATESERIALIZEGEN_HEADER_FILES})

    add_executable(OpenApoc_GamestateSerializeGen ${GAMESTATESERIALIZEGEN_SOURCE_FILES}
    		${GAMESTATESERIALIZEGEN_HEADER_FILES})

    set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )

    target_link_libraries(OpenApoc_GamestateSerializeGen OpenApoc_LibPugixml
    		${Boost_LIBRARIES})

    target_include_directories(OpenApoc_GamestateSerializeGen PUBLIC ${Boost_INCLUDE_DIRS})
    target_include_directories(OpenApoc_GamestateSerializeGen PUBLIC ${CMAKE_SOURCE_DIR})

    set_property(TARGET OpenApoc_GamestateSerializeGen PROPERTY CXX_STANDARD 11)
    set_property(TARGET OpenApoc_GamestateSerializeGen PROPERTY CXX_STANDARD_REQUIRED ON)
endif()