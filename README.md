# RUI

RUI 是一个基于 C++20、CMake 和 Qt6 的跨平台响应式 UI 框架，灵感来自 Jetpack Compose / SwiftUI 等现代声明式 UI 范式。它面向桌面端应用开发，提供组合式 UI 组件、响应式状态管理、导航控制、平台适配能力和内置开发调试工具。

当前仓库包含框架核心代码、基础 UI 组件、Qt 平台适配层、DevTools，以及一个用于演示和验证框架能力的 `sgui_test` 可执行程序。

## 特性

- **组合式 UI** — 使用 `Column`、`Row`、`Text`、`Button` 等组件，通过 `Composable` 宏声明式构建界面
- **链式 Modifier** — 支持 `padding`、`size`、`foreground`、`background`、`fontSize`、`border`、`align` 等样式修饰
- **响应式状态** — `MutableState<T>` 提供线程安全的可观察状态，状态变更自动触发 UI 重组
- **导航控制** — 栈式页面导航（`push`/`pop`/`replace`）及独立窗口管理（`openWindow`）
- **内置 DevTools** — 基于 WebSocket 的 Web 调试面板，支持组件树查看、属性检查、状态监控、布局高亮、截图预览
- **Material Design 色板** — 内置 Blue、Red、Green、Orange 及 Grey 色系
- **日志系统** — 基于 spdlog，Debug/Release 差异化日志级别与刷盘策略
- **XML 配置** — 运行时配置文件驱动工作目录、日志、DevTools 等设置

## 目录结构

```text
.
|-- bin/                  # 构建产物目录（可执行文件、资源文件）
|   |-- sgui_test         #   可执行文件
|   `-- conf/
|       `-- config.xml    #   运行时配置文件（由开发者手动创建）
|-- cmake/                # CMake 公共脚本与依赖配置
|   |-- common.cmake      #   编译器标志、C++20 标准、警告
|   |-- dependencies.cmake #   Qt6 + spdlog 依赖解析
|   |-- qt_auto_install.cmake # Windows 下自动下载 Qt
|   |-- spdlog_config.h   #   自定义日志级别缩写
|   `-- utils.cmake       #   工具函数：源文件收集、目标创建
|-- src/                  # 框架核心代码
|   |-- config/           #   XML 配置解析
|   |-- core/             #   平台适配、状态管理、导航、组合式接口
|   |-- ui/               #   UI 组件（Column、Row、Text、Button、Modifier、Color）
|   `-- utils/            #   日志、工具基类
|-- tools/devtools/       # 开发调试工具与前端页面
|   |-- devtools_server.* #   HTTP + WebSocket 服务器（JSON-RPC 协议）
|   |-- widget_inspector.* #  QWidget 树 → JSON 序列化
|   |-- state_registry.*  #   全局状态注册表
|   `-- frontend.html     #   Web 调试面板前端（组件树、属性检查、状态监控、截图预览）
|-- tests/
|   `-- test.cc           # 示例 / 测试入口
|-- cmake/                # CMake 公共脚本与依赖配置
|-- CMakeLists.txt        # 主构建脚本
|-- CMakePresets.json     # 推荐的本地构建预设
`-- .clang-format         # 代码风格配置
```

## 开发环境要求

通用要求：

- CMake 3.22 或更高版本
- 支持 C++20 的编译器
- Git，用于 CMake `FetchContent` 拉取 spdlog
- Qt 6，组件包括 `Widgets`、`WebSockets`、`Network`
- 可用网络连接，用于首次拉取第三方依赖

平台建议：

- **Windows**：推荐 MinGW-w64，并使用仓库提供的 `mingw-debug` 预设。
- **macOS**：推荐 Apple Clang + Ninja，Qt 可通过 Qt Online Installer 或 Homebrew 安装。
- **Linux**：推荐 GCC 或 Clang + Ninja，Qt 可通过发行版包管理器、Qt Online Installer 或 `aqtinstall` 安装。

Windows 的 `mingw-debug` 预设默认开启 `RUI_AUTO_INSTALL_QT=ON`。如果本机没有可用的 Qt6，配置阶段会通过 Python 和 `aqtinstall` 把 Qt 6.8.0 下载到仓库内的 `.deps/qt` 目录。

macOS 和 Linux 当前没有内置 CMake preset，建议先安装 Qt6，再通过 `CMAKE_PREFIX_PATH` 指定 Qt 安装路径。

## 搭建开发环境

### Windows

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

运行示例程序：

```powershell
.\bin\sgui_test.exe
```

### macOS

安装基础工具：

```bash
xcode-select --install
brew install cmake ninja qt
```

配置并构建：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"

cmake --build build
```

运行示例程序：

```bash
./bin/sgui_test
```

### Linux

以 Ubuntu / Debian 为例，安装基础工具和 Qt6：

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-websockets-dev
```

配置并构建：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

运行示例程序：

```bash
./bin/sgui_test
```

构建产物会输出到：

```text
bin/
```

`frontend.html` 会在构建时自动复制到 `bin/`。`bin/conf/config.xml` 需要开发者手动创建（参见下文配置说明）。

## 使用已有 Qt

如果你已经安装了 Qt6，可以显式指定 Qt 路径。

Windows MinGW 示例：

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/mingw32-make.exe `
  -DRUI_AUTO_INSTALL_QT=OFF `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/mingw_64"

cmake --build build
```

请根据本机 Qt 安装位置调整 `CMAKE_PREFIX_PATH`。

macOS / Linux 示例：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="/path/to/qt"

cmake --build build
```

`CMAKE_PREFIX_PATH` 应指向 Qt 安装前缀，该目录下通常包含 `lib/cmake/Qt6/Qt6Config.cmake`。

## 第三方项目接入

### 使用发布的 SDK 包

推送 `v*` tag 后，GitHub Actions 会为 Windows MinGW x86_64、Linux x86_64 和 macOS arm64 构建 SDK，并自动附加到对应的 GitHub Release。SDK 包含 RUI 静态库、公共头文件、DevTools 前端和 CMake package 配置，不包含 Qt 运行库。

安装与当前平台、编译器匹配的 Qt6，解压 SDK 后即可通过 `find_package` 使用：

```cmake
cmake_minimum_required(VERSION 3.22)
project(my_app LANGUAGES CXX)

find_package(RUI CONFIG REQUIRED)

add_executable(my_app main.cc)
target_link_libraries(my_app PRIVATE RUI::rui)
```

一键配置和构建：

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/path/to/qt;/path/to/RUI-0.1.0-platform"
cmake --build build --parallel
```

Windows PowerShell 中使用分号分隔 Qt 和 RUI SDK 路径时，需要为整个 `CMAKE_PREFIX_PATH` 参数加引号。SDK 与应用必须使用兼容的操作系统、CPU 架构、编译器 ABI 和 Qt 主版本。

SDK 根目录同时提供 `RUIConsumerPresets.json`。第三方项目可以创建一个不提交到版本库的 `CMakeUserPresets.json`：

```json
{
  "version": 4,
  "include": [
    "/path/to/rui/RUIConsumerPresets.json"
  ]
}
```

Qt 已位于系统搜索路径时，可以直接一键构建：

```bash
cmake --preset rui-release
cmake --build --preset rui-release
```

如果 Qt 不在系统搜索路径，可在执行配置前设置 `CMAKE_PREFIX_PATH` 环境变量。Windows 的 RUI 包使用 MinGW ABI，因此还需确保 MinGW `g++` 和 Ninja 位于 `PATH` 中。

### 直接从源码构建

第三方也可以使用 CMake `FetchContent` 拉取指定版本，RUI 会自动下载 spdlog，且作为子项目时默认不构建仓库内示例：

```cmake
include(FetchContent)
FetchContent_Declare(
    RUI
    GIT_REPOSITORY https://github.com/<owner>/<repo>.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(RUI)

target_link_libraries(my_app PRIVATE RUI::rui)
```

将 `<owner>/<repo>` 替换为实际 GitHub 仓库地址。首次配置需要网络连接和已安装的 Qt6。

### 手动生成 SDK

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DRUI_BUILD_EXAMPLES=OFF
cmake --build build --parallel
cpack --config build/CPackConfig.cmake -G TGZ
```

Windows 可将最后一个命令的生成器改为 `ZIP`。也可以在 GitHub 仓库的 Actions 页面手动运行 `Package RUI SDK` workflow，手动运行产生的包会保存在 workflow artifacts 中，tag 构建还会创建 GitHub Release。

## 配置文件

运行前需要创建 `bin/conf/config.xml`，可参考源码根目录的 `config.xml` 模板：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<config>
    <!-- 工作目录，必须为绝对路径 -->
    <workpath>/path/to/your/bin</workpath>
    <!-- 日志配置 -->
    <logs enabled="true" level="trace" max_size="10MB" max_files="1">logs/app.log</logs>
    <!-- 开发者工具 -->
    <devtools>
        <frontend>frontend.html</frontend>
    </devtools>
</config>
```

| 字段 | 说明 |
|------|------|
| `workpath` | 工作目录，**必须为绝对路径**，所有相对路径以此为基础解析。例如设为 `/Users/xxx/SGUI/bin` 后，日志路径 `logs/app.log` 解析为 `/Users/xxx/SGUI/bin/logs/app.log` |
| `logs` | 文件日志开关（`enabled`）、日志级别（`level`）、轮转大小（`max_size`）、轮转文件数（`max_files`）及相对路径 |
| `frontend` | DevTools 前端 HTML 的相对路径 |

### Debug 与 Release 差异

| 行为 | Debug | Release |
|------|-------|---------|
| 控制台日志 | 输出到 stdout | 关闭 |
| 日志级别 | `trace`（全部输出） | `info` 及以上 |
| 刷盘策略 | 每条日志立即落盘 | ERROR 及以上才刷盘 |

## 参与开发

建议按以下流程提交改动：

1. 从最新主分支创建功能分支。
2. 修改代码前先确认相关模块边界：
   - `src/core`：平台适配、状态、导航、组合式接口
   - `src/ui`：具体 UI 组件和样式能力
   - `tools/devtools`：调试工具、布局检查、状态检查和 WebSocket 服务
   - `tests`：演示入口和验证代码
3. 保持改动聚焦，避免把格式化、重命名和功能修改混在一次提交里。
4. 新增源文件时放入对应目录即可，当前 CMake 会递归收集 `src`、`tools`、`tests` 下的 `.cc`、`.cpp`、`.c` 文件。
5. 提交前至少执行一次完整构建。Windows 可以使用 preset：

```powershell
cmake --build --preset mingw-debug
```

macOS / Linux 可以使用通用构建目录：

```bash
cmake --build build
```

如修改了 UI 行为，建议同时运行 `sgui_test` 做人工验证。

## 代码风格

仓库提供了 `.clang-format`（基于 Google 风格，120 列宽，4 空格缩进）。修改 C++ 代码后，建议对变更文件执行格式化：

```bash
clang-format -i src/path/to/file.cc src/path/to/file.hpp
```

## 常见问题

### 配置阶段下载 Qt 失败

确认 Python、pip 和网络可用，也可以先手动安装 `aqtinstall`：

```powershell
python -m pip install --user aqtinstall
```

然后重新执行：

```powershell
cmake --preset mingw-debug
```

### 找不到 Qt6

如果不使用自动安装，请确认 `CMAKE_PREFIX_PATH` 指向 Qt 安装前缀，例如：

```text
C:/Qt/6.8.0/mingw_64
```

该目录下通常应包含 `lib/cmake/Qt6/Qt6Config.cmake`。

### 运行时报缺少 Qt DLL

Windows 下构建脚本会在找到 `windeployqt` 时自动部署运行依赖。如果仍然缺少 DLL，请确认使用的 Qt 版本和 MinGW 工具链匹配，然后重新配置并构建。
