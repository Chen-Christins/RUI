include_guard(GLOBAL)

option(RUI_AUTO_INSTALL_QT "Download Qt with aqtinstall when Qt6 is not found" OFF)

set(RUI_QT_VERSION "6.8.0" CACHE STRING "Qt version to install with aqtinstall")
set(RUI_QT_HOST "windows" CACHE STRING "aqt host argument")
set(RUI_QT_TARGET "desktop" CACHE STRING "aqt target argument")
set(RUI_QT_ARCH "win64_mingw" CACHE STRING "aqt architecture argument")
set(RUI_QT_MODULES "qtwebsockets" CACHE STRING "Extra Qt modules to install with aqtinstall")
set(RUI_QT_INSTALL_ROOT "${CMAKE_SOURCE_DIR}/.deps/qt" CACHE PATH "Project-local Qt install root")

if(NOT RUI_AUTO_INSTALL_QT)
    return()
endif()

find_package(Qt6 QUIET COMPONENTS Widgets WebSockets Network)
if(Qt6_FOUND)
    return()
endif()

set(_rui_qt_version_root "${RUI_QT_INSTALL_ROOT}/${RUI_QT_VERSION}")
set(_rui_qt_prefix "${_rui_qt_version_root}/${RUI_QT_ARCH}")
set(_rui_qt_config "${_rui_qt_prefix}/lib/cmake/Qt6/Qt6Config.cmake")

if(NOT EXISTS "${_rui_qt_config}")
    file(GLOB _rui_qt_configs
        LIST_DIRECTORIES FALSE
        "${_rui_qt_version_root}/*/lib/cmake/Qt6/Qt6Config.cmake"
    )
    if(_rui_qt_configs)
        list(GET _rui_qt_configs 0 _rui_qt_config)
        get_filename_component(_rui_qt_cmake_dir "${_rui_qt_config}" DIRECTORY)
        get_filename_component(_rui_qt_lib_cmake_dir "${_rui_qt_cmake_dir}" DIRECTORY)
        get_filename_component(_rui_qt_lib_dir "${_rui_qt_lib_cmake_dir}" DIRECTORY)
        get_filename_component(_rui_qt_prefix "${_rui_qt_lib_dir}" DIRECTORY)
    endif()
endif()

if(NOT EXISTS "${_rui_qt_config}")
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(NOT Python3_EXECUTABLE)
        message(FATAL_ERROR
            "RUI_AUTO_INSTALL_QT is ON, but Python3 was not found. "
            "Install Python or set Python3_EXECUTABLE."
        )
    endif()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m aqt --version
        RESULT_VARIABLE _rui_aqt_result
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT _rui_aqt_result EQUAL 0)
        message(STATUS "aqtinstall is not installed; installing it with pip.")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m pip install --user aqtinstall
            RESULT_VARIABLE _rui_pip_result
        )
        if(NOT _rui_pip_result EQUAL 0)
            message(FATAL_ERROR
                "Failed to install aqtinstall with pip. "
                "Install it manually with: ${Python3_EXECUTABLE} -m pip install --user aqtinstall"
            )
        endif()
    endif()

    set(_rui_aqt_command
        "${Python3_EXECUTABLE}" -m aqt install-qt
        -O "${RUI_QT_INSTALL_ROOT}"
        "${RUI_QT_HOST}"
        "${RUI_QT_TARGET}"
        "${RUI_QT_VERSION}"
        "${RUI_QT_ARCH}"
    )

    if(RUI_QT_MODULES)
        list(APPEND _rui_aqt_command -m ${RUI_QT_MODULES})
    endif()

    message(STATUS "Qt6 was not found; downloading Qt ${RUI_QT_VERSION} to ${RUI_QT_INSTALL_ROOT}.")
    execute_process(
        COMMAND ${_rui_aqt_command}
        RESULT_VARIABLE _rui_qt_install_result
    )

    if(NOT _rui_qt_install_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to install Qt with aqtinstall. "
            "Check RUI_QT_VERSION, RUI_QT_ARCH, and network access."
        )
    endif()
endif()

if(NOT EXISTS "${_rui_qt_config}")
    message(FATAL_ERROR "Qt install completed, but Qt6Config.cmake was not found at: ${_rui_qt_config}")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${_rui_qt_prefix}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "CMake package search prefixes" FORCE)
set(Qt6_DIR "${_rui_qt_prefix}/lib/cmake/Qt6" CACHE PATH "Qt6 CMake package directory" FORCE)
