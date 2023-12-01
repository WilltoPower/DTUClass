cmake_minimum_required(VERSION 3.13.0)

project(RETRANS)

set(PROGRAM_NAME retrans)

set(EXECUTENAME ${PROGRAM_NAME})

# include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)

add_executable(${EXECUTENAME} ./comConf.cpp
                              ./serial_transfer_impl.cpp)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/debug)
else()
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/release)
endif()

target_link_libraries(${EXECUTENAME}
    "-Wl,--start-group"
    pthread
    "-Wl,--end-group" rt
)
