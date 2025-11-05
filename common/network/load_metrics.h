// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef COMMON_NETWORK_LOAD_METRICS
#define COMMON_NETWORK_LOAD_METRICS

#include <cstdint>
#include <atomic>

namespace cppnet {

class LoadMetrics {
public:
    LoadMetrics();
    ~LoadMetrics() = default;
    
    // Update metrics
    void UpdateCPULoad(double load);
    void UpdateIOWaitTime(uint64_t time);
    void UpdateConnectionCount(uint32_t count);
    void UpdateMemoryPoolUsage(double usage);
    void UpdateTaskQueueLength(uint32_t length);
    void UpdatePacketRate(uint32_t rate);
    void UpdateBandwidthUsage(double usage);
    void UpdateContextSwitchRate(uint32_t rate);
    void UpdateCacheHitRate(double rate);
    void UpdateErrorRate(double rate);
    void UpdateThreadUtilization(double utilization);
    void UpdateResponseTime(uint64_t time);
    
    // Get metrics
    double GetCPULoad() const { return _cpu_load; }
    uint64_t GetIOWaitTime() const { return _io_wait_time; }
    uint32_t GetConnectionCount() const { return _connection_count; }
    double GetMemoryPoolUsage() const { return _memory_pool_usage; }
    uint32_t GetTaskQueueLength() const { return _task_queue_length; }
    uint32_t GetPacketRate() const { return _packet_rate; }
    double GetBandwidthUsage() const { return _bandwidth_usage; }
    uint32_t GetContextSwitchRate() const { return _context_switch_rate; }
    double GetCacheHitRate() const { return _cache_hit_rate; }
    double GetErrorRate() const { return _error_rate; }
    double GetThreadUtilization() const { return _thread_utilization; }
    uint64_t GetResponseTime() const { return _response_time; }
    
    // Calculate dynamic load score
    double CalculateLoadScore();
    
private:
    // CPU metrics
    std::atomic<double> _cpu_load{0.0};
    std::atomic<double> _thread_utilization{0.0};
    std::atomic<uint32_t> _context_switch_rate{0};
    
    // IO metrics
    std::atomic<uint64_t> _io_wait_time{0};
    std::atomic<uint32_t> _packet_rate{0};
    std::atomic<double> _bandwidth_usage{0.0};
    
    // Connection metrics
    std::atomic<uint32_t> _connection_count{0};
    
    // Memory metrics
    std::atomic<double> _memory_pool_usage{0.0};
    std::atomic<double> _cache_hit_rate{0.0};
    
    // Task metrics
    std::atomic<uint32_t> _task_queue_length{0};
    std::atomic<uint64_t> _response_time{0};
    
    // Error metrics
    std::atomic<double> _error_rate{0.0};
    
    // Weights for each metric (sum to 1.0)
    static const double _weight_cpu_load;
    static const double _weight_thread_utilization;
    static const double _weight_context_switch_rate;
    static const double _weight_io_wait_time;
    static const double _weight_packet_rate;
    static const double _weight_bandwidth_usage;
    static const double _weight_connection_count;
    static const double _weight_memory_pool_usage;
    static const double _weight_cache_hit_rate;
    static const double _weight_task_queue_length;
    static const double _weight_response_time;
    static const double _weight_error_rate;
};

} // namespace cppnet

#endif