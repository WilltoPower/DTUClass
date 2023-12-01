cmake_minimum_required(VERSION 3.13.0)
project(dturules)

include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)
include_directories(${CMAKE_SOURCE_DIR}/libdtucomm)
include_directories(${CMAKE_SOURCE_DIR}/libdtudsp)
include_directories(${CMAKE_SOURCE_DIR}/libdtustorage)
include_directories(${CMAKE_SOURCE_DIR}/libdtutask)
include_directories(${CMAKE_SOURCE_DIR}/libmodule)
aux_source_directory(. DIR_SRCS)

add_library(dturules STATIC ${DIR_SRCS})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug) 
else()
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release) 
endif()