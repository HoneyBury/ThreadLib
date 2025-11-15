# 文件路徑: benchmarks/ThreadLib/CMakeLists.txt

cmake_minimum_required(VERSION 3.23)
project(CppThreadFlow LANGUAGES CXX)

# 自動查找所有源文件
file(GLOB_RECURSE SOURCES "./*.cpp")
file(GLOB_RECURSE HEADERS "./*.hpp")

# 將源碼編譯成一個靜態庫 (STATIC)
# 這樣它就能被鏈接到我們的 benchmark 程序中
add_library(CppThreadFlow STATIC ${SOURCES} ${HEADERS})

# 讓 CppThreadFlow::CppThreadFlow 這個目標可用
add_library(CppThreadFlow::CppThreadFlow ALIAS CppThreadFlow)

# 設置 C++17 標準
target_compile_features(CppThreadFlow PUBLIC cxx_std_17)

target_include_directories(CppThreadFlow
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)