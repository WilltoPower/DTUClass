cmake_minimum_required(VERSION 3.13.0)
project(dtuconfigure)

include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)
include_directories(${CMAKE_SOURCE_DIR}/libdtustorage)
include_directories(${CMAKE_SOURCE_DIR}/libdtubase)
include_directories(${CMAKE_SOURCE_DIR}/libdtucomm)

aux_source_directory(. SRC)

add_library(dtuconfigure STATIC ${SRC})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug) 
else()
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release) 
endif()