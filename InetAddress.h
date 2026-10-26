#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr) {}

    // 返回 IP 字符串
    std::string toIp() const;
    // 返回 "ip:port" 字符串
    std::string toIpPort() const;
    // 返回端口号
    uint16_t toPort() const;

    // 返回 sockaddr_in 指针
    const sockaddr_in *getSockAddr() const { return &addr_; }
    // 设置 sockaddr_in
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};