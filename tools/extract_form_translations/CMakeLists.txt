# project name, and type
project(OpenApoc_ExtractFormTranslations CXX)

# check cmake version
cmake_minimum_required(VERSION 3.9)

find_package(Boost REQUIRED COMPONENTS program_options)

set(EXTRACT_FORM_TRANSLATIONS_SOURCE_FILES main.cpp)

source_group(serializationtool\\sources
             FILES ${EXTRACT_FORM_TRANSLATIONS_SOURCE_FILES})

set(EXTRACT_FORM_TRANSLATIONS_HEADER_FILES)

source_group(serializationtool\\headers
             FILES ${EXTRACT_FORM_TRANSLATIONS_HEADER_FILES})

list(APPEND ALL_SOURCE_FILES ${EXTRACT_FORM_TRANSLATIONS_SOURCE_FILES})
list(APPEND ALL_HEADER_FILES ${EXTRACT_FORM_TRANSLATIONS_HEADER_FILES})

add_executable(
  OpenApoc_ExtractFormTranslations ${EXTRACT_FORM_TRANSLATIONS_SOURCE_FILES}
                                   ${EXTRACT_FORM_TRANSLATIONS_HEADER_FILES})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

target_link_libraries(OpenApoc_ExtractFormTranslations OpenApoc_LibPugixml
                      ${Boost_LIBRARIES} fmt::fmt)
target_include_directories(OpenApoc_ExtractFormTranslations
                           PUBLIC ${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
