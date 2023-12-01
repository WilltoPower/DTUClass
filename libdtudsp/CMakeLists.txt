cmake_minimum_required(VERSION 3.13.0)
project(dtudsp)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libdtubase)
include_directories(${CMAKE_SOURCE_DIR}/libdtucomm)
include_directories(${CMAKE_SOURCE_DIR}/libdtustorage)
include_directories(${CMAKE_SOURCE_DIR}/libdturulesfile)
include_directories(${CMAKE_SOURCE_DIR}/libdtugoose)
aux_source_directory(. SRC_DIR)

add_library(dtudsp STATIC ${SRC_DIR})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug) 
else()
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release) 
endif()