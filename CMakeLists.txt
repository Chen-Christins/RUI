cmake_minimum_required(VERSION 3.22)
project(SGUI VERSION 0.1.0 LANGUAGES CXX)

# 引入 cmake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(common)
include(dependencies)
include(utils)

# 收集源文件
chen_set_sources(src SRCS)
chen_set_sources(tests TEST_SRCS)

# 测试可执行文件
chen_add_executable(sgui_test "${SRCS};${TEST_SRCS}" "" "Qt6::Widgets;Qt6::WebSockets;Qt6::Network")

# 设置输出目录
chen_set_target_output_dir(sgui_test "${CMAKE_BINARY_DIR}/bin")

# 目标包含路径
target_include_directories(sgui_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/devtools
)