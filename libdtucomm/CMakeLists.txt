cmake_minimum_required(VERSION 3.13.0)
project(dtucomm)

include_directories(${CMAKE_SOURCE_DIR}/libdtudsp)
include_directories(${CMAKE_SOURCE_DIR}/libdtucomm)
include_directories(${CMAKE_SOURCE_DIR}/libdtutask)
include_directories(${CMAKE_SOURCE_DIR}/libdtustorage)
include_directories(${CMAKE_SOURCE_DIR}/libdturuleskit)
include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)

aux_source_directory(. SRC)

add_library(dtucomm STATIC ${SRC})

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug) 
else()
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release) 
endif()