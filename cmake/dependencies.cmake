# dependencies.cmake - 第三方库和依赖项配置

include_directories(.)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets)
