# dependencies.cmake - 第三方库和依赖项配置

include_directories(.)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets WebSockets Network)

# Log4Qt - Log4J 风格的 Qt 日志框架
include(FetchContent)
FetchContent_Declare(
    log4qt
    GIT_REPOSITORY https://github.com/MEONMedical/Log4Qt.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
set(BUILD_WITH_DB_LOGGING OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_TELNET_LOGGING OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_QML_LOGGING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(log4qt)
