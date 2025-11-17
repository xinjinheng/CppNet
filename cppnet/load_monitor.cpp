// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "load_monitor.h"
#include "dispatcher.h"
#include <algorithm>

namespace cppnet {

LoadMonitor::LoadMonitor() : 
    _cpu_threshold(80),
    _queue_threshold(1000),
    _connection_threshold(10000),
    _running(true) {
}

LoadMonitor::~LoadMonitor() {
    _running = false;
}

void LoadMonitor::AddDispatcher(std::shared_ptr<Dispatcher> dispatcher) {
    std::lock_guard<std::mutex> lock(_mutex);
    _dispatcher_loads.emplace_back(DispatcherLoad{dispatcher, LoadInfo()});
}

void LoadMonitor::RemoveDispatcher(std::shared_ptr<Dispatcher> dispatcher) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = std::find_if(_dispatcher_loads.begin(), _dispatcher_loads.end(), 
                          [&dispatcher](const DispatcherLoad& dl) {
                              return dl.dispatcher == dispatcher;
                          });
    if (it != _dispatcher_loads.end()) {
        _dispatcher_loads.erase(it);
    }
}

void LoadMonitor::UpdateLoadInfo(std::shared_ptr<Dispatcher> dispatcher, 
                               uint32_t connection_count, 
                               uint32_t cpu_usage, 
                               uint32_t queue_length, 
                               uint64_t total_bytes) {

    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& dl : _dispatcher_loads) {
        if (dl.dispatcher == dispatcher) {
            dl.load_info.connection_count = connection_count;
            dl.load_info.cpu_usage = cpu_usage;
            dl.load_info.queue_length = queue_length;
            dl.load_info.total_bytes = total_bytes;
            dl.load_info.last_update_time = std::chrono::steady_clock::now();
            return;
        }
    }
}

void LoadMonitor::UpdateDispatcherLoad(std::shared_ptr<Dispatcher> dispatcher, uint32_t connection_count) {

    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& dl : _dispatcher_loads) {
        if (dl.dispatcher == dispatcher) {
            dl.load_info.connection_count = connection_count;
            dl.load_info.last_update_time = std::chrono::steady_clock::now();
            return;
        }
    }
}

std::shared_ptr<Dispatcher> LoadMonitor::GetLeastLoadedDispatcher() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_dispatcher_loads.empty()) {
        return nullptr;
    }

    auto least_it = _dispatcher_loads.begin();
    for (auto it = _dispatcher_loads.begin() + 1; it != _dispatcher_loads.end(); ++it) {
        if (it->load_info.cpu_usage < least_it->load_info.cpu_usage ||
            (it->load_info.cpu_usage == least_it->load_info.cpu_usage &&
             it->load_info.connection_count < least_it->load_info.connection_count)) {
            least_it = it;
        }
    }
    return least_it->dispatcher;
}

std::shared_ptr<Dispatcher> LoadMonitor::GetMostLoadedDispatcher() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_dispatcher_loads.empty()) {
        return nullptr;
    }

    auto most_it = _dispatcher_loads.begin();
    for (auto it = _dispatcher_loads.begin() + 1; it != _dispatcher_loads.end(); ++it) {
        if (it->load_info.cpu_usage > most_it->load_info.cpu_usage ||
            (it->load_info.cpu_usage == most_it->load_info.cpu_usage &&
             it->load_info.connection_count > most_it->load_info.connection_count)) {
            most_it = it;
        }
    }
    return most_it->dispatcher;
}

bool LoadMonitor::NeedLoadBalance() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_dispatcher_loads.size() < 2) {
        return false;
    }

    uint32_t max_cpu = 0;
    uint32_t max_queue = 0;
    uint32_t max_conn = 0;

    for (auto& dl : _dispatcher_loads) {
        if (dl.load_info.cpu_usage > max_cpu) {
            max_cpu = dl.load_info.cpu_usage;
        }
        if (dl.load_info.queue_length > max_queue) {
            max_queue = dl.load_info.queue_length;
        }
        if (dl.load_info.connection_count > max_conn) {
            max_conn = dl.load_info.connection_count;
        }
    }

    return max_cpu > _cpu_threshold ||
           max_queue > _queue_threshold ||
           max_conn > _connection_threshold;
}

} // namespace cppnet