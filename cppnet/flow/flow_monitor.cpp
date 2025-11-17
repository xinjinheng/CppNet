// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "flow_monitor.h"
#include "socket/rw_socket.h"

namespace cppnet {

FlowMonitor::FlowMonitor() : 
    _bandwidth_threshold(10 * 1024 * 1024), // 10MB/s
    _packet_rate_threshold(1000), // 1000 packets/s
    _min_packet_size(40),
    _max_packet_size(1500) {
}

FlowMonitor::~FlowMonitor() {
}

void FlowMonitor::AddSocket(std::shared_ptr<RWSocket> socket) {
    if (!socket) {
        return;
    }

    std::lock_guard<std::mutex> lock(_stats_mutex);
    _flow_stats.emplace(socket, FlowStats());
}

void FlowMonitor::RemoveSocket(std::shared_ptr<RWSocket> socket) {
    if (!socket) {
        return;
    }

    std::lock_guard<std::mutex> lock(_stats_mutex);
    _flow_stats.erase(socket);
    
    std::lock_guard<std::mutex> block_lock(_block_mutex);
    _blocked_sockets.erase(socket);
}

void FlowMonitor::UpdateFlowStats(std::shared_ptr<RWSocket> socket, uint64_t bytes_in, uint64_t bytes_out) {
    if (!socket) {
        return;
    }

    std::lock_guard<std::mutex> lock(_stats_mutex);
    auto it = _flow_stats.find(socket);
    if (it == _flow_stats.end()) {
        return;
    }

    FlowStats& stats = it->second;
    stats.bytes_in += bytes_in;
    stats.bytes_out += bytes_out;
    stats.packets_in++;
    stats.packets_out++;
    
    auto now = std::chrono::steady_clock::now();
    stats.connection_time = std::chrono::duration_cast<std::chrono::seconds>(now - stats.last_packet_time).count();
    stats.last_packet_time = now;
    
    // 更新滑动窗口
    stats.recent_packets.emplace_back(bytes_in + bytes_out, now);
    
    // 保持窗口大小为最近100个数据包
    if (stats.recent_packets.size() > 100) {
        stats.recent_packets.erase(stats.recent_packets.begin());
    }
}

AnomalyInfo FlowMonitor::DetectAnomaly(std::shared_ptr<RWSocket> socket) {
    AnomalyInfo info;
    info.type = AnomalyType::NONE;
    info.score = 0.0;
    
    if (!socket) {
        return info;
    }

    std::lock_guard<std::mutex> lock(_stats_mutex);
    auto it = _flow_stats.find(socket);
    if (it == _flow_stats.end()) {
        return info;
    }

    const FlowStats& stats = it->second;
    auto now = std::chrono::steady_clock::now();
    
    // 计算过去1秒的流量
    uint64_t bytes_in_1s = 0;
    uint64_t bytes_out_1s = 0;
    uint32_t packets_1s = 0;
    
    for (const auto& packet : stats.recent_packets) {
        if (now - packet.second < std::chrono::seconds(1)) {
            bytes_in_1s += packet.first;
            packets_1s++;
        }
    }
    
    double bandwidth = bytes_in_1s / 1024.0 / 1024.0; // MB/s
    double packet_rate = packets_1s;
    
    // 检测高带宽
    if (bandwidth > _bandwidth_threshold / 1024 / 1024) {
        info.type = static_cast<AnomalyType>(static_cast<int>(info.type) | static_cast<int>(AnomalyType::HIGH_BANDWIDTH));
        info.score += 0.3;
    }
    
    // 检测高数据包速率
    if (packet_rate > _packet_rate_threshold) {
        info.type = static_cast<AnomalyType>(static_cast<int>(info.type) | static_cast<int>(AnomalyType::HIGH_PACKET_RATE));
        info.score += 0.3;
    }
    
    // 检测小数据包洪水
    if (packets_1s > 500 && bytes_in_1s < 100 * packets_1s) {
        info.type = static_cast<AnomalyType>(static_cast<int>(info.type) | static_cast<int>(AnomalyType::SMALL_PACKET_FLOOD));
        info.score += 0.4;
    }
    
    if (info.score > 0.5) {
        info.description = "Anomaly detected: ";
        if (info.type & AnomalyType::HIGH_BANDWIDTH) info.description += "high bandwidth ";
        if (info.type & AnomalyType::HIGH_PACKET_RATE) info.description += "high packet rate ";
        if (info.type & AnomalyType::SMALL_PACKET_FLOOD) info.description += "small packet flood ";
    }
    
    return info;
}

bool FlowMonitor::ThrottleSocket(std::shared_ptr<RWSocket> socket, uint32_t rate_limit) {
    // 简单实现：设置socket的发送缓冲区大小
    if (!socket) {
        return false;
    }
    
    // 这里需要操作系统支持动态调整发送缓冲区
    // 实际实现中应该使用令牌桶等算法
    return true;
}

bool FlowMonitor::BlockSocket(std::shared_ptr<RWSocket> socket, uint32_t duration) {
    if (!socket) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(_block_mutex);
    auto now = std::chrono::steady_clock::now();
    _blocked_sockets[socket] = now + std::chrono::seconds(duration);
    
    return true;
}

bool FlowMonitor::IsSocketBlocked(std::shared_ptr<RWSocket> socket) {
    if (!socket) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(_block_mutex);
    auto it = _blocked_sockets.find(socket);
    if (it == _blocked_sockets.end()) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (now < it->second) {
        return true;
    }
    
    _blocked_sockets.erase(it);
    return false;
}

AdaptiveLimiter::AdaptiveLimiter() : 
    _base_limit(1000),
    _max_limit(10000),
    _min_limit(100),
    _current_limit(1000),
    _total_bytes_in(0),
    _total_bytes_out(0),
    _connection_count(0) {
    _last_update = std::chrono::steady_clock::now();
}

AdaptiveLimiter::~AdaptiveLimiter() {
}

void AdaptiveLimiter::UpdateGlobalStats(uint64_t total_bytes_in, uint64_t total_bytes_out, uint32_t connection_count) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    _total_bytes_in = total_bytes_in;
    _total_bytes_out = total_bytes_out;
    _connection_count = connection_count;
    
    // 简单的自适应算法：根据连接数调整限流
    uint32_t new_limit = _base_limit * (1 + (_connection_count / 1000));
    new_limit = std::max(_min_limit, std::min(new_limit, _max_limit));
    _current_limit = new_limit;
}

uint32_t AdaptiveLimiter::GetCurrentLimit() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _current_limit;
}

void AdaptiveLimiter::ApplyLimit(std::shared_ptr<RWSocket> socket) {
    // 应用限流策略
    // 实际实现中应该在事件驱动层进行限流
}

} // namespace cppnet