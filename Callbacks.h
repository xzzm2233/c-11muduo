#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

// 定义各类回调函数类型
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;             // 连接对象的智能指针
using TimerCallback = std::function<void()>;                         // 定时任务回调
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>; // 连接建立/关闭回调
using CloseCallback = std::function<void(const TcpConnectionPtr &)>; // 关闭回调
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>; // 写完成回调

using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                           Buffer *,
                                           Timestamp)>; // 消息到达回调
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>; // 高水位回调