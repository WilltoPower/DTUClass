cmake_minimum_required(VERSION 3.13.0)
project(dtumodule)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libdturuleskit)
aux_source_directory(. DIR_SRCS)

add_library(dtumodule STATIC ${DIR_SRCS})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug) 
else()
    SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release) 
endif()
