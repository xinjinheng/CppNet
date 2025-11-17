// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include <iostream>
#include <thread>
#include <chrono>
#include "include/cppnet.h"
#include "cppnet/ssl/ssl_context.h"

using namespace cppnet;

void OnRead(std::shared_ptr<CNSocket> sock, std::shared_ptr<CppNetBuffer> data, uint32_t len) {
    std::string str;
    data->Peek(str, len);
    std::cout << "read data: " << str << std::endl;
    sock->Write(str.c_str(), len);
}

void OnConnect(std::shared_ptr<CNSocket> sock, uint16_t err) {
    if (err != CEC_SUCCESS) {
        std::cout << "connect error: " << err << std::endl;
        return;
    }
    std::cout << "connect success" << std::endl;
    std::string str = "hello ssl world";
    sock->Write(str.c_str(), str.length());
}

void OnDisconnect(std::shared_ptr<CNSocket> sock, uint16_t err) {
    std::cout << "disconnect error: " << err << std::endl;
}

void OnAccept(std::shared_ptr<CNSocket> sock, uint16_t err) {
    if (err != CEC_SUCCESS) {
        std::cout << "accept error: " << err << std::endl;
        return;
    }
    std::cout << "accept success" << std::endl;
}

int main() {
    CppNet cppnet;
    
    // 初始化网络库
    cppnet.Init(1);
    
    // 设置回调函数
    cppnet.SetReadCallback(OnRead);
    cppnet.SetWriteCallback([](std::shared_ptr<CNSocket> sock, uint32_t len) {
        std::cout << "write len: " << len << std::endl;
    });
    cppnet.SetConnectionCallback(OnConnect);
    cppnet.SetDisconnectionCallback(OnDisconnect);
    cppnet.SetAcceptCallback(OnAccept);
    
    // 这里应该添加SSL初始化代码
    // 实际使用中需要证书文件
    // cppnet.EnableSSL("server.crt", "server.key", "ca.crt");
    
    // 启动监听
    if (cppnet.ListenAndAccept("0.0.0.0", 8922)) {
        std::cout << "ssl listen success on port 8922" << std::endl;
    } else {
        std::cout << "ssl listen error" << std::endl;
        return -1;
    }
    
    // 等待连接
    cppnet.Join();
    
    return 0;
}