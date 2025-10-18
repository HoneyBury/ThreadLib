//
// Created by Administrator on 2025/10/18.
//
#pragma once

#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/async.h" // 用于异步初始化

/**
 * Log.h
 * 全局异步日志系统接口
 */
namespace Log
{
// 声明一个全局共享的 logger 指针，外部文件可通过 extern 访问
extern std::shared_ptr<spdlog::logger> g_Logger;

/**
 * @brief 初始化日志系统：
 * 1. 创建异步线程池。
 * 2. 配置 Console (控制台) 和 File (文件) 两个 Sink (日志重定向)。
 * 3. 创建异步 Logger 并设置自定义格式。
 */
void Init();

/**
 * @brief 关闭日志系统：
 * 确保所有排队日志被写入，并释放资源。
 */
void Shutdown();
}

// -----------------------------------------------------------
// 日志宏定义
// -----------------------------------------------------------

// 核心宏：检查 logger 是否存在，如果存在则调用相应的日志函数
#define LOG_CHECK_AND_CALL(level, ...) \
if (Log::g_Logger) { \
Log::g_Logger->level(__VA_ARGS__); \
}

// 暴露给外部使用的日志宏
#define LOG_TRACE(...)    LOG_CHECK_AND_CALL(trace, __VA_ARGS__)
#define LOG_INFO(...)     LOG_CHECK_AND_CALL(info, __VA_ARGS__)
#define LOG_WARN(...)     LOG_CHECK_AND_CALL(warn, __VA_ARGS__)
#define LOG_ERROR(...)    LOG_CHECK_AND_CALL(error, __VA_ARGS__)
#define LOG_CRITICAL(...) LOG_CHECK_AND_CALL(critical, __VA_ARGS__)
