// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef CPPNET_LOAD_MONITOR
#define CPPNET_LOAD_MONITOR

#include <chrono>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace cppnet {

class Dispatcher;

struct LoadInfo {
    std::atomic<uint32_t> connection_count;
    std::atomic<uint32_t> cpu_usage;
    std::atomic<uint32_t> queue_length;
    std::atomic<uint64_t> total_bytes;
    std::chrono::steady_clock::time_point last_update_time;

    LoadInfo() : connection_count(0), cpu_usage(0), queue_length(0), total_bytes(0) {
        last_update_time = std::chrono::steady_clock::now();
    }
};

class LoadMonitor {
public:
    LoadMonitor();
    ~LoadMonitor();

    void AddDispatcher(std::shared_ptr<Dispatcher> dispatcher);
    void RemoveDispatcher(std::shared_ptr<Dispatcher> dispatcher);

    void UpdateLoadInfo(std::shared_ptr<Dispatcher> dispatcher, 
                      uint32_t connection_count, 
                      uint32_t cpu_usage, 
                      uint32_t queue_length, 
                      uint64_t total_bytes);

    // 只更新连接数
    void UpdateDispatcherLoad(std::shared_ptr<Dispatcher> dispatcher, uint32_t connection_count);

    std::shared_ptr<Dispatcher> GetLeastLoadedDispatcher();
    std::shared_ptr<Dispatcher> GetMostLoadedDispatcher();
    bool NeedLoadBalance();

    // 设置负载均衡阈值
    void SetCpuThreshold(uint32_t threshold) { _cpu_threshold = threshold; }
    void SetQueueThreshold(uint32_t threshold) { _queue_threshold = threshold; }
    void SetConnectionThreshold(uint32_t threshold) { _connection_threshold = threshold; }

private:
    struct DispatcherLoad {
        std::shared_ptr<Dispatcher> dispatcher;
        LoadInfo load_info;
    };

    std::vector<DispatcherLoad> _dispatcher_loads;
    std::mutex _mutex;
    uint32_t _cpu_threshold;
    uint32_t _queue_threshold;
    uint32_t _connection_threshold;
    std::atomic<bool> _running;
};

} // namespace cppnet

#endif