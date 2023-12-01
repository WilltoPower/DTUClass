cmake_minimum_required(VERSION 3.13.0)

INCLUDE(${CMAKE_SOURCE_DIR}/sdl9200.cmake)

project(SDL9200)
# 输出详细编译命令
set(CMAKEV_VERBOSE_MAKE_FILE ON)

set(CMAKE_CXX_STANDARD 14)
###
message("-- FLAGS:" ${CMAKE_C_FLAGS})

#设置查找工具程序路径
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

#设置只在指定目录下查找库文件
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

#设置只在指定目录下查找头文件
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
####

#if(CMAKE_COMPILER_IS_GNUCXX)
#    add_compile_options(-mcpu=ck860)
#    message(STATUS "optional:-mcpu=ck860")
#endif(CMAKE_COMPILER_IS_GNUCXX)

################################################################################
# 程序版本
# V01 主版本	功能模块变动较大时修改,例如模块功能增加,修改架构等
# R01 子版本	某个模块的功能发生较大变化时修改
# C01 内部修订版本	可作为修订版本, 修复一个严重BUG可以进行修改
# B00 阶段版本	如下：
# 00: Alpha版, 此阶段的软件主要以实现功能为主,BUG较多,需进一步修复
# 01: Beta版, 相对Alpha有改进,消除一些严重BUG, 仍存在缺陷,需要进一步测试
# 02: RC版, 基本成熟, 基本不存在导致错误的BUG;
# 03: Release, 正式发布版

set(MAIN_VERSION 20)
set(SUB_VERSION 23)
set(INNER_VERSION 02)
set(STAGE_VERSION 14)

STRING(TIMESTAMP COMPILETIME "%Y/%m/%d %H:%M:%S")

SET(COMPILE_TIME ${COMPILETIME})

# 添加config.h
configure_file (
  "${CMAKE_SOURCE_DIR}/include/version.h.in"
  "${CMAKE_SOURCE_DIR}/include/dtuversion.h"
)
# 这样可以找到config.h头文件
# include_directories("${CMAKE_CURRENT_BINARY_DIR}")
################################################################################
add_definitions(-DPUGIXML_HEADER_ONLY)
add_definitions(-DASIO_STANDALONE)

option (DEBUG_LOG "print debug log information" ON)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/include)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/include/msgpack)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/include/asio)
include_directories(${CMAKE_SOURCE_DIR}/libdtubase) 
include_directories(${CMAKE_SOURCE_DIR}/libdtuconfigure)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/include)
include_directories(${CMAKE_SOURCE_DIR}/libmodule)

add_subdirectory(thirdparty)
add_subdirectory(libdtuconfigure)
add_subdirectory(libdtubase)
add_subdirectory(libdturuleskit)
add_subdirectory(libdtustorage)
add_subdirectory(libdturulesfile)
add_subdirectory(libdtudsp)
add_subdirectory(libdtucomm)
add_subdirectory(libdtutask)
add_subdirectory(libdtugoose)
add_subdirectory(libmodule)
add_subdirectory(sdl9200)
add_subdirectory(serialretrans)