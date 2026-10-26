cmake_minimum_required(VERSION 3.10)
project(mymuduo)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=gnu++17")

set(SRC_LIST
    Acceptor.cc
    Buffer.cc
    Channel.cc
    CurrentThread.cc
    DefaultPoller.cc
    EPollPoller.cc
    EventLoop.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
    InetAddress.cc
    Logger.cc
    Poller.cc
    PollPoller.cc
    Socket.cc
    TcpConnection.cc
    TcpServer.cc
    Thread.cc
    Timer.cc
    TimerQueue.cc
    Timestamp.cc
    AsyncLogging.cc
    LogFile.cc
    Connector.cc
    TcpClient.cc
)

add_library(mymuduo STATIC ${SRC_LIST})

target_include_directories(mymuduo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
