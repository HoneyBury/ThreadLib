//
// Created by HoneyBury on 25-6-22.
//
#include <iostream>
#include <thread>
#include <chrono>
#include "cppsharp/Log.h"
void worker_function(int id)
{
 // 在另一个线程中安全地使用宏
 LOG_INFO("工作线程 {} 启动...", id);
 std::this_thread::sleep_for(std::chrono::milliseconds(50));
 LOG_ERROR("工作线程 {} 发生了一个模拟错误。", id);
}

int main()
{
 // 1. 初始化系统
 Log::Init();

 // 2. 在主线程中记录日志
 LOG_CRITICAL("=== 应用程序启动 ===");
 LOG_INFO("主线程正在执行任务...");

 // 3. 启动多个工作线程演示多线程安全和异步
 std::thread t1(worker_function, 1);
 std::thread t2(worker_function, 2);

 // 4. 执行大量日志操作，测试异步性能
 for (int i = 0; i < 1000; ++i)
 {
  LOG_TRACE("循环日志测试: {}", i);
 }

 // 5. 等待线程完成
 t1.join();
 t2.join();

 // 6. 记录结束信息
 LOG_WARN("所有工作已完成。");

 // 7. 关闭系统
 Log::Shutdown();

 std::cout << "程序执行完毕，请检查 logs/App.log 文件。" << std::endl;

 return 0;
}