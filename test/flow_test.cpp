// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include <iostream>
#include <thread>
#include <chrono>
#include "include/cppnet.h"
#include "cppnet/flow/flow_monitor.h"

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
    
    // 模拟大流量发送
    std::string str = "test data flow control ";
    for (int i = 0; i < 100; i++) {
        sock->Write(str.c_str(), str.length());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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
        // std::cout << "write len: " << len << std::endl;
    });
    cppnet.SetConnectionCallback(OnConnect);
    cppnet.SetDisconnectionCallback(OnDisconnect);
    cppnet.SetAcceptCallback(OnAccept);
    
    // 这里应该添加流量监控和限流的初始化代码
    // cppnet.EnableFlowControl();
    // cppnet.SetBandwidthThreshold(1 * 1024 * 1024); // 1MB/s
    
    // 启动监听
    if (cppnet.ListenAndAccept("0.0.0.0", 8923)) {
        std::cout << "flow control test listen success on port 8923" << std::endl;
    } else {
        std::cout << "flow control test listen error" << std::endl;
        return -1;
    }
    
    // 等待连接
    cppnet.Join();
    
    return 0;
}