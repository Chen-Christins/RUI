# dependencies.cmake - 第三方库和依赖项配置

include_directories(.)

# 查找 Qt6
include(qt_auto_install)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebSockets Network)

if(WIN32)
    get_target_property(_qt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
    find_program(QT_WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}" NO_DEFAULT_PATH)
endif()

# Log4Qt - Log4J 风格的 Qt 日志框架
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED OFF CACHE BOOL "Allow FetchContent updates during configure" FORCE)
FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)

set(_rui_build_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(spdlog)
target_compile_definitions(spdlog PRIVATE
    "SPDLOG_SHORT_LEVEL_NAMES={\"TRACE\",\"DEBUG\",\"INFO\",\"WARN\",\"ERROR\",\"CRITICAL\",\"OFF\"}"
)
set(BUILD_SHARED_LIBS ${_rui_build_shared_libs})
unset(_rui_build_shared_libs)
