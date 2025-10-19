//
// Created by HoneyBury on 25-6-22.
//
#include <iostream>
#include <thread>
#include <chrono>
#include "cppsharp/Log.h"
#include "ThreadLib/scheduler.hpp"
#include "ThreadLib/thread_pool.hpp"
int main()
{
 // 1. 初始化日志系统
 Log::Init();
 using namespace std::chrono_literals;

 cppthreadflow::ThreadPool pool(4);
 cppthreadflow::Scheduler scheduler(pool);

 std::cout << "调度任务开始于: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;

 // 1. 安排一个3秒后执行的单次任务
 scheduler.schedule_after(3s, [] {
     std::cout << "延迟3秒的任务执行了！" << std::endl;
 });

 // 2. 安排一个5秒后首次执行，然后每2秒执行一次的周期性任务
 auto first_time = cppthreadflow::Scheduler::Clock::now() + 5s;
 scheduler.schedule_periodic(first_time, 2s, [] {
     static std::atomic<int> count = 0;
     std::cout << "周期性任务执行第 " << ++count << " 次。" << std::endl;
 });

 // 3. 安排一个1秒后执行的单次任务 (这个会比上面那个3秒的先执行)
 scheduler.schedule_after(1s, []{
     std::cout << "延迟1秒的任务执行了！" << std::endl;
 });

 // 让主线程保持运行以观察调度结果
 std::this_thread::sleep_for(12s);

 std::cout << "主程序结束。" << std::endl;
 // 当 scheduler 和 pool 对象离开作用域时，它们的析构函数会被调用，
 // 会自动、安全地停止所有后台线程。
 return 0;
}