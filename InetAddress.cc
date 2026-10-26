#include "InetAddress.h"

#include <string.h>

// 用端口和 IP 字符串构造地址，并填充 sockaddr_in
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

// 返回点分十进制的 IP 字符串
std::string InetAddress::toIp() const
{
    // addr_
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

// 返回 "ip:port" 形式的字符串
std::string InetAddress::toIpPort() const
{
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

// 返回主机字节序的端口号
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}
