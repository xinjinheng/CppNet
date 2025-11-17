// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include <iostream>
#include <thread>
#include <chrono>
#include "include/cppnet.h"
#include "cppnet/load_monitor.h"
#include "cppnet/connection_migrator.h"

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
    std::string str = "hello world";
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
    
    // 初始化网络库，使用4个线程
    cppnet.Init(4);
    
    // 设置回调函数
    cppnet.SetReadCallback(OnRead);
    cppnet.SetWriteCallback([](std::shared_ptr<CNSocket> sock, uint32_t len) {
        std::cout << "write len: " << len << std::endl;
    });
    cppnet.SetConnectionCallback(OnConnect);
    cppnet.SetDisconnectionCallback(OnDisconnect);
    cppnet.SetAcceptCallback(OnAccept);
    
    // 启动监听
    if (cppnet.ListenAndAccept("0.0.0.0", 8921)) {
        std::cout << "listen success on port 8921" << std::endl;
    } else {
        std::cout << "listen error" << std::endl;
        return -1;
    }
    
    // 模拟负载均衡
    std::thread balance_thread([&cppnet]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 这里可以添加负载均衡测试逻辑
        std::cout << "load balance test start" << std::endl;
        
        // 实际使用中，可以定期检查负载并进行迁移
        // cppnet.TriggerLoadBalance();
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    });
    
    balance_thread.detach();
    
    // 等待连接
    cppnet.Join();
    
    return 0;
}