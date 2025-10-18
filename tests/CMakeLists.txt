# 确保能找到 Conan 提供的 GTest 包
find_package(GTest REQUIRED)
# 引入现代化的 GTest/CTest 集成模块
include(GoogleTest)

# 1. 创建一个【单一的】测试可执行文件，包含所有测试源文件
#    假设你的 main_test.cpp 只是另一个包含 TEST(...) 的文件，
#    如果它包含了 main() 函数，则应该移除，因为 GTest::gtest_main 会提供 main()。
add_executable(run_tests
        test_concurrent_queue.cpp
        test_thread_pool.cpp
        # 如果还有其他测试文件，都在这里添加
        main_test.cpp
)

# 2. 为这个单一的测试目标链接你的库和 GTest
#    使用 GTest::gtest_main 会自动提供 main 函数并链接 GTest::gtest
target_link_libraries(run_tests
        PRIVATE
        ${PROJECT_NAME}
        GTest::gtest_main
)

# 3. 启用 CTest，并让它自动发现 "run_tests" 中的所有测试用例
#    这一步会自动创建一个名为 "run_tests" 的 CTest 测试，
#    它会智能地运行所有独立的 TEST(...) 和 TEST_F(...)。
gtest_discover_tests(run_tests DISCOVERY_MODE PRE_TEST )