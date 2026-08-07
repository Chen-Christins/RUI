cmake_minimum_required(VERSION 3.22)
project(RUIPackageConsumer LANGUAGES CXX)

find_package(RUI CONFIG REQUIRED)

add_executable(rui_package_consumer main.cc)
target_link_libraries(rui_package_consumer PRIVATE RUI::rui)
