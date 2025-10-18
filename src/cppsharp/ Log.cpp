//
// Created by Administrator on 2025/10/18.
//
#include <codecvt>
#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h" // 控制台彩色输出
#include "spdlog/sinks/rotating_file_sink.h" // 轮转文件 Sink
#include <locale>
// 定义全局共享的 logger 实例
std::shared_ptr<spdlog::logger> Log::g_Logger;

void Log::Init()
{
    // --- 1. 配置异步线程池 ---
    size_t queue_size = 4096; // 队列大小
    size_t thread_count = 1;  // 工作线程数

    // 初始化线程池
    spdlog::init_thread_pool(queue_size, thread_count);

    // --- 2. 配置 Sinks (日志重定向) ---

    // Sink 1: 控制台 (带颜色)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // 定制格式：[时间.毫秒] [级别] [线程ID] [文件名:行号] 消息
    console_sink->set_pattern("[%T.%e] [%^%l%$] [Thread %t] %v");
    console_sink->set_level(spdlog::level::info); // 控制台只显示 INFO 及以上
    // Sink 2: 轮转文件 (更适合生产环境)
    // 文件名: logs/App.log, 最大 10MB, 最多保留 3 个旧文件
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/App.log", 1048576 * 10, 3
    );
    file_sink->set_level(spdlog::level::trace); // 文件记录所有级别的日志

    // 文件格式：[日期 时间.毫秒] [级别] 消息
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    // Sink 列表
    std::vector<spdlog::sink_ptr> sinks { console_sink, file_sink };

    // --- 3. 创建异步 Logger ---
    // 使用 spdlog::async_factory 来创建异步 Logger 实例
    g_Logger = std::make_shared<spdlog::async_logger>(
        "ASYNC_GLOBAL_LOGGER",       // Logger 名称
        sinks.begin(), sinks.end(),  // Sinks 列表
        spdlog::thread_pool(),       // 绑定到异步线程池
        spdlog::async_overflow_policy::block // 队列满时阻塞主线程，确保不丢失消息
    );

    // 设置 Logger 自身的日志级别为 Trace
    g_Logger->set_level(spdlog::level::trace);

    // 注册 Logger
    spdlog::register_logger(g_Logger);

    // 设置全局刷新策略：当记录 WARN 及以上级别的消息时，立即刷新
    g_Logger->flush_on(spdlog::level::warn);

    LOG_INFO("日志系统初始化完成。异步双目标重定向已配置。");
}

void Log::Shutdown()
{
    if (g_Logger)
    {
        LOG_INFO("正在关闭日志系统，确保日志数据被写入...");
        // 显式刷新并清除 logger，确保所有日志都写入
        g_Logger->flush();
        spdlog::drop("ASYNC_GLOBAL_LOGGER");
        g_Logger.reset();
    }

    // 关闭全局线程池 (停止后台线程)
    spdlog::shutdown();
}