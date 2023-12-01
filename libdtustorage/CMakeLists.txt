cmake_minimum_required(VERSION 3.13.0)
project(dtustorage)

include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)
include_directories(${CMAKE_SOURCE_DIR}/libdtucomm)
include_directories(${CMAKE_SOURCE_DIR}/libdtudsp)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libdtutask)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/include/sqlite3)

aux_source_directory(. DIR_SRCS)
aux_source_directory(${CMAKE_SOURCE_DIR}/thirdparty/include/sqlite3 DIR_SRCS)

add_library(dtustorage STATIC ${DIR_SRCS})


if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug) 
else()
 SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release) 
endif()