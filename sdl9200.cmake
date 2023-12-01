#告知当前使用的是交叉编译方式，必须配置
SET(CMAKE_SYSTEM_NAME Linux)
# 设置编译标志
SET(CMAKE_C_FLAGS "-std=gnu9x")
SET(CMAKE_CXX_FLAGS "-Wno-psabi")

if(NOT PLATFORM)
    set(PLATFORM "ck860")
    message("-- 未知编译环境,使用默认CK860进行编译")
endif()

# 分平台进行编译
if(${PLATFORM} STREQUAL "ck860")
# 编译伏羲平台程序
MESSAGE("-- build ck860")
# 添加伏羲特有的宏定义
ADD_DEFINITIONS(-D_FUXI_H2)
SET(CMAKE_SYSTEM_PROCESSOR arm)
# 设置交叉编译工具链
SET(ENV{TOOLCHAIN} /toolchains/csky-linux-gnuabiv2/bin/csky-abiv2-linux-)
SET(CMAKE_C_COMPILER "/toolchains/csky-linux-gnuabiv2/bin/csky-abiv2-linux-gcc")
SET(CMAKE_CXX_COMPILER "/toolchains/csky-linux-gnuabiv2/bin/csky-abiv2-linux-g++")
# 浮点数编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfdivdu -mhard-float -mdouble-float -march=ck860v -mcpu=ck860fv")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfdivdu -mhard-float -mdouble-float -march=ck860v -mcpu=ck860fv")
elseif(${PLATFORM} STREQUAL "am5716")
# 编译am5716平台程序
MESSAGE("-- build am5716")
SET(CMAKE_SYSTEM_PROCESSOR arm)
# 设置交叉编译工具链
SET(ENV{TOOLCHAIN} /opt/sdl6008/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-)
SET(CMAKE_C_COMPILER "/opt/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
SET(CMAKE_CXX_COMPILER "/opt/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
elseif(${PLATFORM} STREQUAL "x86")
# 编译x86_x64平台程序
MESSAGE("-- build x86")
else()
# 未知平台报错
MESSAGE(FATAL_ERROR "未知的平台环境")
endif()

# 设置调试编译选项
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "Set debug compilation options")
    add_definitions("-g")
endif()



#设置查找工具程序路径
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

#设置只在指定目录下查找库文件
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

#设置只在指定目录下查找头文件
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
