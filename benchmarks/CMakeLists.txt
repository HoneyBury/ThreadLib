# 文件路徑: benchmarks/CMakeLists.txt

cmake_minimum_required(VERSION 3.23)
project(CppThreadFlowBenchmarks LANGUAGES CXX)

# 1. 查找由 Conan 管理的依賴 (benchmark)
find_package(benchmark REQUIRED)

# 2. 將 ThreadLib 文件夾作為一個子項目添加
#    這將會執行 ThreadLib/CMakeLists.txt
#    並創建 CppThreadFlow::CppThreadFlow 目標
add_subdirectory(ThreadLib)

# 3. 定義我們的 benchmark 可執行文件
add_executable(run_benchmarks
        benchmark_concurrent_hash_map.cpp
        benchmark_thread_pool.cpp
)

# 4. 鏈接所有需要的庫
target_link_libraries(run_benchmarks PRIVATE
        # 鏈接我們剛從源碼編譯的庫
        CppThreadFlow::CppThreadFlow

        # 鏈接 Google Benchmark
        benchmark::benchmark
        benchmark::benchmark_main
)

# 5. 確保 benchmark 程序也使用 C++17
target_compile_features(run_benchmarks PRIVATE cxx_std_17)