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
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Do not update fetched content during reconfigure" FORCE)
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
