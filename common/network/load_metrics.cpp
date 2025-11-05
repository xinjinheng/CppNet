// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "load_metrics.h"
#include <algorithm>

namespace cppnet {

// Initialize weights (sum to 1.0)
const double LoadMetrics::_weight_cpu_load = 0.15;
const double LoadMetrics::_weight_thread_utilization = 0.1;
const double LoadMetrics::_weight_context_switch_rate = 0.05;
const double LoadMetrics::_weight_io_wait_time = 0.1;
const double LoadMetrics::_weight_packet_rate = 0.1;
const double LoadMetrics::_weight_bandwidth_usage = 0.05;
const double LoadMetrics::_weight_connection_count = 0.15;
const double LoadMetrics::_weight_memory_pool_usage = 0.1;
const double LoadMetrics::_weight_cache_hit_rate = 0.05;
const double LoadMetrics::_weight_task_queue_length = 0.05;
const double LoadMetrics::_weight_response_time = 0.05;
const double LoadMetrics::_weight_error_rate = 0.05;

LoadMetrics::LoadMetrics() {
}

void LoadMetrics::UpdateCPULoad(double load) {
    _cpu_load = std::min(1.0, std::max(0.0, load));
}

void LoadMetrics::UpdateIOWaitTime(uint64_t time) {
    _io_wait_time = time;
}

void LoadMetrics::UpdateConnectionCount(uint32_t count) {
    _connection_count = count;
}

void LoadMetrics::UpdateMemoryPoolUsage(double usage) {
    _memory_pool_usage = std::min(1.0, std::max(0.0, usage));
}

void LoadMetrics::UpdateTaskQueueLength(uint32_t length) {
    _task_queue_length = length;
}

void LoadMetrics::UpdatePacketRate(uint32_t rate) {
    _packet_rate = rate;
}

void LoadMetrics::UpdateBandwidthUsage(double usage) {
    _bandwidth_usage = std::min(1.0, std::max(0.0, usage));
}

void LoadMetrics::UpdateContextSwitchRate(uint32_t rate) {
    _context_switch_rate = rate;
}

void LoadMetrics::UpdateCacheHitRate(double rate) {
    _cache_hit_rate = std::min(1.0, std::max(0.0, rate));
}

void LoadMetrics::UpdateErrorRate(double rate) {
    _error_rate = std::min(1.0, std::max(0.0, rate));
}

void LoadMetrics::UpdateThreadUtilization(double utilization) {
    _thread_utilization = std::min(1.0, std::max(0.0, utilization));
}

void LoadMetrics::UpdateResponseTime(uint64_t time) {
    _response_time = time;
}

double LoadMetrics::CalculateLoadScore() {
    // Normalize all metrics to 0-1 range
    double cpu_load = _cpu_load;
    double thread_utilization = _thread_utilization;
    double context_switch_rate = std::min(1.0, static_cast<double>(_context_switch_rate) / 100000.0); // Normalize to 100k switches/sec
    double io_wait_time = std::min(1.0, static_cast<double>(_io_wait_time) / 1000000.0); // Normalize to 1 second
    double packet_rate = std::min(1.0, static_cast<double>(_packet_rate) / 1000000.0); // Normalize to 1M packets/sec
    double bandwidth_usage = _bandwidth_usage;
    double connection_count = std::min(1.0, static_cast<double>(_connection_count) / 10000.0); // Normalize to 10k connections
    double memory_pool_usage = _memory_pool_usage;
    double cache_hit_rate = 1.0 - _cache_hit_rate; // Higher cache miss rate increases load
    double task_queue_length = std::min(1.0, static_cast<double>(_task_queue_length) / 1000.0); // Normalize to 1k tasks
    double response_time = std::min(1.0, static_cast<double>(_response_time) / 1000000.0); // Normalize to 1 second
    double error_rate = _error_rate;
    
    // Calculate weighted sum
    double score = 0.0;
    score += _weight_cpu_load * cpu_load;
    score += _weight_thread_utilization * thread_utilization;
    score += _weight_context_switch_rate * context_switch_rate;
    score += _weight_io_wait_time * io_wait_time;
    score += _weight_packet_rate * packet_rate;
    score += _weight_bandwidth_usage * bandwidth_usage;
    score += _weight_connection_count * connection_count;
    score += _weight_memory_pool_usage * memory_pool_usage;
    score += _weight_cache_hit_rate * cache_hit_rate;
    score += _weight_task_queue_length * task_queue_length;
    score += _weight_response_time * response_time;
    score += _weight_error_rate * error_rate;
    
    // Ensure score is between 0 and 1
    return std::min(1.0, std::max(0.0, score));
}

} // namespace cppnet