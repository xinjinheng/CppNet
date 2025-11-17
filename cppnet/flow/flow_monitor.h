// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef CPPNET_FLOW_FLOW_MONITOR
#define CPPNET_FLOW_FLOW_MONITOR

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>

namespace cppnet {

class RWSocket;

struct FlowStats {
    std::atomic<uint64_t> bytes_in;
    std::atomic<uint64_t> bytes_out;
    std::atomic<uint32_t> packets_in;
    std::atomic<uint32_t> packets_out;
    std::atomic<uint32_t> connection_time;
    
    // 滑动窗口统计
    std::chrono::steady_clock::time_point last_packet_time;
    std::vector<std::pair<uint64_t, std::chrono::steady_clock::time_point>> recent_packets;
    
    FlowStats() : bytes_in(0), bytes_out(0), packets_in(0), packets_out(0), connection_time(0) {
        last_packet_time = std::chrono::steady_clock::now();
    }
};

enum class AnomalyType {
    NONE = 0,
    HIGH_BANDWIDTH = 1,
    HIGH_PACKET_RATE = 2,
    LARGE_PACKET_SIZE = 4,
    SMALL_PACKET_FLOOD = 8,
    CONNECTION_FLOOD = 16,
    SLOWLORIS = 32,
};

struct AnomalyInfo {
    AnomalyType type;
    double score;
    std::string description;
};

class FlowMonitor {
public:
    FlowMonitor();
    ~FlowMonitor();

    void AddSocket(std::shared_ptr<RWSocket> socket);
    void RemoveSocket(std::shared_ptr<RWSocket> socket);
    
    void UpdateFlowStats(std::shared_ptr<RWSocket> socket, uint64_t bytes_in, uint64_t bytes_out);
    AnomalyInfo DetectAnomaly(std::shared_ptr<RWSocket> socket);
    
    // 设置异常检测阈值
    void SetBandwidthThreshold(uint64_t threshold) { _bandwidth_threshold = threshold; }
    void SetPacketRateThreshold(uint32_t threshold) { _packet_rate_threshold = threshold; }
    void SetPacketSizeThreshold(uint32_t min, uint32_t max) {
        _min_packet_size = min;
        _max_packet_size = max;
    }
    
    // 限流操作
    bool ThrottleSocket(std::shared_ptr<RWSocket> socket, uint32_t rate_limit);
    bool BlockSocket(std::shared_ptr<RWSocket> socket, uint32_t duration);
    bool IsSocketBlocked(std::shared_ptr<RWSocket> socket);

private:
    std::unordered_map<std::shared_ptr<RWSocket>, FlowStats> _flow_stats;
    std::unordered_map<std::shared_ptr<RWSocket>, std::chrono::steady_clock::time_point> _blocked_sockets;
    
    uint64_t _bandwidth_threshold;
    uint32_t _packet_rate_threshold;
    uint32_t _min_packet_size;
    uint32_t _max_packet_size;
    
    std::mutex _stats_mutex;
    std::mutex _block_mutex;
};

class AdaptiveLimiter {
public:
    AdaptiveLimiter();
    ~AdaptiveLimiter();

    void UpdateGlobalStats(uint64_t total_bytes_in, uint64_t total_bytes_out, uint32_t connection_count);
    uint32_t GetCurrentLimit();
    void ApplyLimit(std::shared_ptr<RWSocket> socket);
    
    void SetBaseLimit(uint32_t limit) { _base_limit = limit; }
    void SetMaxLimit(uint32_t limit) { _max_limit = limit; }
    void SetMinLimit(uint32_t limit) { _min_limit = limit; }

private:
    uint32_t _base_limit;
    uint32_t _max_limit;
    uint32_t _min_limit;
    uint32_t _current_limit;
    
    uint64_t _total_bytes_in;
    uint64_t _total_bytes_out;
    uint32_t _connection_count;
    
    std::chrono::steady_clock::time_point _last_update;
    std::mutex _mutex;
};

} // namespace cppnet

#endif