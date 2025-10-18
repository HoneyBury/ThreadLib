# CppThreadFlow

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![CMake](https://img.shields.io/badge/CMake-3.23%2B-green.svg)](https://cmake.org/)
[![Conan](https://img.shields.io/badge/Conan-2.0%2B-purple.svg)](https://conan.io/)
[![Language](https://img.shields.io/badge/C%2B%2B-17%2F20-blue.svg)](https://isocpp.org/)
[![CI/CD](https://github.com/HoneyBury/ThreadLib/actions/workflows/ci.yml/badge.svg)](https://github.com/HoneyBury/ThreadLib/actions)

一个现代、高效、功能丰富的C++线程库，旨在简化并发和异步编程。

`CppThreadFlow` 提供了一套高级的、易于使用的工具集，包括但不限于线程池、任务调度器、线程安全容器和同步原语，帮助开发者轻松构建健壮、高性能的多线程应用程序。

## ✨ 核心特性

* **现代C++设计**：基于 C++17 标准，充分利用 `std::future`, `std::function`, `std::thread` 等现代C++特性，提供类型安全、无锁编程友好的接口。
* **高性能线程池**：
    * 动态/静态线程数量调整。
    * 多种任务窃取（Work-Stealing）策略，实现负载均衡。
    * 支持任务优先级。
* **灵活的任务调度**：
    * 提交异步任务，通过 `future` 获取结果。
    * 支持延迟任务、周期性任务。
* **线程安全容器**：
    * 线程安全的队列（`ConcurrentQueue`）。
    * 线程安全的哈希表（`ConcurrentHashMap`）。
* **丰富的同步原语**：
    * 信号量（`Semaphore`）、屏障（`Barrier`）、锁存器（`Latch`）等。
* **Header-Only (可选)**: 核心功能可通过头文件方式引入，简化集成。
* **跨平台支持**: 在 Windows, Linux, 和 macOS 上经过测试。
* **完善的测试**: 使用 Google Test 保证代码质量和稳定性。

## 依赖

* C++ 编译器 (支持 C++17 及以上)
* CMake (>= 3.23)
* Conan (>= 2.0)

## 🚀 快速开始 (如何使用本库)

将 `CppThreadFlow` 集成到你的项目中非常简单。

**1. 添加依赖到 `conanfile.py`**

```python
# conanfile.py
from conan import ConanFile

class YourProject(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        # 请替换为实际的库名称和版本
        self.requires("cppthreadflow/0.1.0")
````

**2. 在 `CMakeLists.txt` 中链接库**

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.23)
project(YourAwesomeProject LANGUAGES CXX)

# ... (其他设置)

find_package(CppThreadFlow REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE CppThreadFlow::CppThreadFlow)
```

## 💡 使用示例

下面是一些基本用法示例，帮助你快速上手。

**示例1: 基本的线程池任务提交**

```cpp
#include <iostream>
#include <cppthreadflow/ThreadPool.h>

int main() {
    // 创建一个拥有4个线程的线程池
    cppthreadflow::ThreadPool pool(4);

    // 提交一个返回 int 的任务
    auto future = pool.submit([]() {
        std::cout << "Task is running in a worker thread." << std::endl;
        return 42;
    });

    // 等待任务完成并获取结果
    int result = future.get();
    std::cout << "Task finished with result: " << result << std::endl;

    return 0;
}
```

## 👨‍💻 开发者指南 (如何贡献代码)

如果你希望为本库贡献代码或进行二次开发，请遵循以下步骤。

**1. 克隆仓库**

```bash
git clone [https://github.com/HoneyBury/ThreadLib.git](https://github.com/HoneyBury/ThreadLib.git)
cd YOUR_REPO
```

**2. 安装 Conan 依赖**
此命令会下载依赖，并在 `build/` 目录下生成CMake所需的文件。

```bash
# Debug 模式
conan install . -s build_type=Debug --build=missing
# Release 模式
conan install . -s build_type=Release --build=missing
```

**3. 配置并构建项目**
我们推荐使用 CMake Presets 来简化配置和构建流程。

```bash
# 配置项目 (Debug)
cmake --preset conan-debug

# 构建项目 (Debug)
cmake --build --preset conan-debug
```

构建产物将位于 `build/` 目录下。

## 🧪 运行测试

我们使用 `ctest` 来运行测试用例，确保所有功能正常。

```bash
# 进入构建目录
cd build/debug

# 运行测试
ctest -C Debug --output-on-failure
```

## 📦 打包

本库主要通过 Conan 进行包管理和分发。

```bash
# 创建Conan包
conan create . --user=your_user --channel=stable

# 上传到私有仓库 (例如 Artifactory)
conan upload cppthreadflow/0.1.0 --user=your_user --channel=stable -r=your_remote
```

## 🗺️ 路线图 (Roadmap)

  * [ ] 实现协程支持 (C++20)。
  * [ ] 增加更多线程安全的容器。
  * [ ] 提供性能基准测试套件。
  * [ ] 完善 API 文档。

## 🤝 贡献

欢迎任何形式的贡献！无论是提交 Issue、发起 Pull Request 还是改进文档，我们都非常欢迎。

## 📜 许可证

本项目基于 [MIT License](https://www.google.com/search?q=LICENSE) 开源。

```
