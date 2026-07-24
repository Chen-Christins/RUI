cmake_minimum_required(VERSION 3.22)
project(RUI VERSION 0.1.0 LANGUAGES CXX)

# 引入 cmake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(common)
include(dependencies)
include(utils)

include_directories(
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/tools"
)

# 收集源文件
set(SOURCES
    # config 模块
    src/config/config.cc
    # core 模块
    src/core/composable.cc
    src/core/nav_controller.cc
    src/core/platform_adapter.cc
    # ui 模块
    src/ui/box.cc
    src/ui/button.cc
    src/ui/color.cc
    src/ui/dimension.cc
    src/ui/image.cc
    src/ui/lazy_list.cc
    src/ui/modifier.cc
    src/ui/scaffold.cc
    src/ui/spacer.cc
    src/ui/text_field.cc
    src/ui/text.cc
    # utils 模块
    src/utils/logger.cc
    src/utils/util.cc
    # devtools 模块
    tools/devtools/widget_inspector.cc
    tools/devtools/state_registry.cc
    tools/devtools/devtools_server.cc
)
add_library(rui STATIC ${SOURCES})
force_redefine_file_macro_for_sources(rui)

target_link_libraries(rui PUBLIC
    Qt6::Widgets
    Qt6::WebSockets
    Qt6::Network
    Qt6::Svg
)
target_link_libraries(rui PRIVATE spdlog::spdlog)

set(LIBS
    rui
    spdlog::spdlog
)

# 测试可执行文件
chen_add_executable(test
    "tests/test.cc"
    ""
    "${LIBS}"
)
# 设置输出目录
chen_set_target_output_dir(test
    "${CMAKE_SOURCE_DIR}/bin"
)

# 目标包含路径
if(WIN32 AND QT_WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET test POST_BUILD
        COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}" --compiler-runtime --no-translations "$<TARGET_FILE:test>"
    )
endif()
