cmake_minimum_required(VERSION 3.22)
project(RUI VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(PROJECT_IS_TOP_LEVEL)
    option(RUI_BUILD_EXAMPLES "Build the RUI example application" ON)
else()
    option(RUI_BUILD_EXAMPLES "Build the RUI example application" OFF)
endif()
option(RUI_BUILD_TESTS "Build RUI unit tests" ${PROJECT_IS_TOP_LEVEL})

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
    src/core/compose_widget.cc
    src/core/coroutine_scope.cc
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
    src/ui/animation.cc
    src/ui/snackbar.cc
    src/ui/switch.cc
    src/ui/checkbox.cc
    src/ui/radio_button.cc
    src/ui/card.cc
    src/ui/divider.cc
    src/ui/dialog.cc
    src/ui/animated_visibility.cc
    src/ui/slider.cc
    src/ui/progress_indicator.cc
    src/ui/chip.cc
    src/ui/badge.cc
    src/ui/dropdown_menu.cc
    src/ui/graphics_effect.cc
    src/ui/flow_layout.cc
    src/ui/surface.cc
    src/ui/tristate_checkbox.cc
    # utils 模块
    src/utils/logger.cc
    src/utils/util.cc
    # devtools 模块
    tools/devtools/widget_inspector.cc
    tools/devtools/state_registry.cc
    tools/devtools/devtools_server.cc
)

if(APPLE)
    enable_language(OBJCXX)
    list(APPEND SOURCES src/core/macos_window.mm)
endif()

add_library(rui STATIC ${SOURCES})
add_library(RUI::rui ALIAS rui)
force_redefine_file_macro_for_sources(rui)

target_compile_features(rui PUBLIC cxx_std_20)
target_include_directories(rui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tools>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(rui PUBLIC
    Qt6::Widgets
    Qt6::WebSockets
    Qt6::Network
    Qt6::Svg
)
target_link_libraries(rui PRIVATE $<BUILD_INTERFACE:spdlog::spdlog>)
target_link_libraries(rui INTERFACE $<INSTALL_INTERFACE:spdlog::spdlog>)

set(LIBS
    rui
    spdlog::spdlog
)

if(RUI_BUILD_EXAMPLES)
    # 测试可执行文件
    chen_add_executable(rui_demo
        "tests/test.cc"
        ""
        "${LIBS}"
    )
    set_target_properties(rui_demo PROPERTIES OUTPUT_NAME test)
    # 设置输出目录
    chen_set_target_output_dir(rui_demo
        "${CMAKE_SOURCE_DIR}/bin"
    )

    chen_add_executable(example
        "tests/example.cc"
        ""
        "${LIBS}"
    )
    chen_set_target_output_dir(example
            "${CMAKE_SOURCE_DIR}/bin"
    )

    if(WIN32 AND QT_WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET rui_demo POST_BUILD
            COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}" --compiler-runtime --no-translations "$<TARGET_FILE:rui_demo>"
        )
    endif()
endif()

if(RUI_BUILD_TESTS)
    enable_testing()
    add_executable(rui_unit_tests
        tests/unit/core_state_test.cc
    )
    chen_set_target_output_dir(rui_unit_tests
        "${CMAKE_SOURCE_DIR}/bin"
    )
    force_redefine_file_macro_for_sources(rui_unit_tests)
    target_compile_features(rui_unit_tests PRIVATE cxx_std_20)
    target_link_libraries(rui_unit_tests PRIVATE
        rui
    )
    if(WIN32 AND QT_WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET rui_unit_tests POST_BUILD
            COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}" --compiler-runtime --no-translations "$<TARGET_FILE:rui_unit_tests>"
        )
    endif()
    add_test(NAME rui_unit_tests COMMAND rui_unit_tests)
endif()

install(TARGETS rui
    EXPORT RUITargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY tools/devtools/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/devtools
    FILES_MATCHING PATTERN "*.hpp"
)
install(FILES tools/devtools/frontend.html
    DESTINATION ${CMAKE_INSTALL_DATADIR}/rui
)
install(FILES cmake/RUIConsumerPresets.json
    DESTINATION .
)

configure_package_config_file(
    cmake/RUIConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/RUIConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RUI"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RUIConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(EXPORT RUITargets
    FILE RUITargets.cmake
    NAMESPACE RUI::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RUI"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RUIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RUIConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/RUI"
)

set(CPACK_PACKAGE_NAME RUI)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "RUI-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
