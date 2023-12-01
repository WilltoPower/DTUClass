cmake_minimum_required(VERSION 3.13.0)

project(SDL9200-D30)

set(PROGRAM_PERFFIX V01R01C03B01)
set(PROGRAM_NAME SDL9200)
set(PROGRAM_END .out)

set(EXECUTENAME ${PROGRAM_NAME}${PROGRAM_PERFFIX}${PROGRAM_END})

add_definitions(-DPUGIXML_HEADER_ONLY)

include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)
include_directories(${CMAKE_SOURCE_DIR}/libdtudsp)
include_directories(${CMAKE_SOURCE_DIR}/libdtucomm)
include_directories(${CMAKE_SOURCE_DIR}/libdtustorage)
include_directories(${CMAKE_SOURCE_DIR}/libdturulesfile)
include_directories(${CMAKE_SOURCE_DIR}/libdturuleskit)
include_directories(${CMAKE_SOURCE_DIR}/libdtutask)
include_directories(${CMAKE_SOURCE_DIR}/libdtugoose)
include_directories(${CMAKE_SOURCE_DIR}/libmodule)

aux_source_directory(. DIR_SRCS)
add_executable(${EXECUTENAME} ${DIR_SRCS})

add_dependencies(${EXECUTENAME} lib60870)
add_dependencies(${EXECUTENAME} libiec61850)

target_link_directories(${EXECUTENAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/lib
    )

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/debug)
else()
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/release)
endif()

target_link_libraries(${EXECUTENAME}
    "-Wl,--start-group"
    lib60870.a
    libiec61850.a
    hal
    dtubase
    dtugoose
    dtuconfigure
    dtustorage
    dturuleskit
    dtudsp
    dtucomm
    dtutask
    dturules
    dtumodule
    pthread
    dl
    "-Wl,--end-group" rt
)




