# CppNet 新功能说明

## 1. 动态负载均衡的连接迁移机制

### 功能描述
当前CppNet中每个socket绑定到固定线程处理，虽避免了线程竞争，但在极端场景下可能导致单线程过载。动态负载均衡机制支持在特定条件下将部分连接迁移到负载较低的线程。

### 核心组件
- **LoadMonitor**：监控每个Dispatcher的负载情况
- **ConnectionMigrator**：管理连接迁移过程

### 使用方法
```cpp
CppNet cppnet;
// 初始化网络库，使用4个线程
cppnet.Init(4);

// 网络库会自动监控负载并在需要时进行连接迁移
```

### 配置选项
- CPU使用率阈值：默认80%
- 连接队列长度阈值：默认1000
- 连接数阈值：默认10000

## 2. TLS/SSL 双向认证与动态证书管理

### 功能描述
扩展对TLS/SSL的支持，实现加密传输。支持单向认证和双向认证，并允许运行时动态更新证书链。

### 核心组件
- **SSLContext**：SSL上下文管理
- **SSLSession**：SSL会话处理
- **SSLCertificateManager**：证书管理

### 使用方法
```cpp
CppNet cppnet;
cppnet.Init(1);

// 启用SSL（需要证书文件）
cppnet.EnableSSL("server.crt", "server.key", "ca.crt");

// 启动监听安全端口
if (cppnet.ListenAndAccept("0.0.0.0", 443)) {
    // ...
}
```

### 配置选项
- 支持TLS 1.2/1.3
- 支持单向/双向认证
- 支持动态证书更新
- 支持证书链验证

## 3. 基于流量特征的异常检测与自适应限流算法

### 功能描述
新增异常流量检测模块，通过分析连接的流量特征识别异常连接，并基于检测结果动态调整限流策略。

### 核心组件
- **FlowMonitor**：流量监控与异常检测
- **AdaptiveLimiter**：自适应限流

### 使用方法
```cpp
CppNet cppnet;
cppnet.Init(1);

// 启用流量控制
cppnet.EnableFlowControl();

// 设置带宽阈值为1MB/s
cppnet.SetBandwidthThreshold(1 * 1024 * 1024);

// 启动监听
if (cppnet.ListenAndAccept("0.0.0.0", 8923)) {
    // ...
}
```

### 异常类型
- 高带宽（>10MB/s默认）
- 高数据包速率（>1000 packets/s默认）
- 小数据包洪水
- 连接洪水
- Slowloris攻击

## 编译说明

### 启用SSL支持
```bash
g++ -DUSE_OPENSSL -lssl -lcrypto your_file.cpp
```

### 依赖
- OpenSSL 1.1.1或更高版本

## 测试

### 负载均衡测试
```bash
./load_balance_test
```

### SSL测试
```bash
./ssl_test
```

### 流量控制测试
```bash
./flow_test
```

## 注意事项

1. 连接迁移过程中会暂停IO操作，可能会影响短暂的性能
2. SSL功能需要正确配置证书文件
3. 异常检测算法会带来一定的性能开销，建议根据实际情况调整阈值
4. 自适应限流算法会根据系统负载动态调整限流策略

## 性能优化

1. 负载监控使用滑动窗口统计，减少性能开销
2. SSL会话复用，提高性能
3. 异常检测算法采用轻量级特征提取
4. 连接迁移采用批量处理，减少上下文切换

## 未来改进

1. 支持更多的负载均衡算法
2. 支持TLS 1.3的0-RTT模式
3. 支持机器学习的异常检测
4. 支持更精细的流量控制策略