// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef CPPNET_DISPATCHER
#define CPPNET_DISPATCHER

#include <mutex>
#include <memory>
#include <thread>
#include <functional>
#include <unordered_map>

#include "include/cppnet_type.h"
#include "common/thread/thread_with_queue.h"
#include "common/network/load_metrics.h"

namespace cppnet {

using Task = std::function<void()>;

class Timer;
class RWSocket;
class TimerEvent;
class CppNetBase;
class EventActions;

class Dispatcher: 
    public Thread, 
    public std::enable_shared_from_this<Dispatcher> {

public:
    Dispatcher(std::shared_ptr<CppNetBase> base, uint32_t thread_num, uint32_t base_id);
    explicit Dispatcher(std::shared_ptr<CppNetBase> base, uint32_t base_id = 0);
    ~Dispatcher() override;

    void Run() override;

    void Stop() override;

    void Listen(uint64_t sock, const std::string& ip, uint16_t port);

    void Connect(const std::string& ip, uint16_t port);

    void PostTask(const Task& task);

    uint32_t AddTimer(const user_timer_call_back& cb, void* param, uint32_t interval, bool always = false);
    uint32_t AddTimer(std::shared_ptr<RWSocket> sock, uint32_t interval, bool always = false);
    void StopTimer(uint32_t timer_id);

    std::thread::id GetThreadID() { return _local_thread_id; }
    std::shared_ptr<EventActions> GetEventActions() { return _event_actions; }

    // Load monitoring methods
    void AddConnection(std::shared_ptr<RWSocket> sock);
    void RemoveConnection(std::shared_ptr<RWSocket> sock);
    uint32_t GetConnectionCount() const { return _connection_count; }
    double GetLoadScore() const;
    
    // Load metrics
    void UpdateCPULoad(double load);
    void UpdateIOWaitTime(uint64_t time);
    void UpdateMemoryPoolUsage(double usage);
    void UpdateTaskQueueLength(uint32_t length);
    void UpdatePacketRate(uint32_t rate);
    void UpdateBandwidthUsage(double usage);
    void UpdateContextSwitchRate(uint32_t rate);
    void UpdateCacheHitRate(double rate);
    void UpdateErrorRate(double rate);
    void UpdateThreadUtilization(double utilization);
    void UpdateResponseTime(uint64_t time);

private:
    void DoTask();
    uint32_t MakeTimerID();
    void UpdateLoadScore();

    uint64_t _cur_utc_time;

    std::mutex _timer_id_mutex;
    uint32_t _timer_id_creater;

    std::mutex _task_list_mutex;
    std::vector<Task> _task_list;

    std::thread::id _local_thread_id;
    std::shared_ptr<Timer> _timer;
    std::shared_ptr<EventActions> _event_actions;

    std::weak_ptr<CppNetBase> _cppnet_base;

    // Load metrics
    LoadMetrics _load_metrics;
    std::atomic<double> _load_score{0.0};
    
    // Connection map
    std::unordered_map<uint64_t, std::shared_ptr<RWSocket>> _connection_map;
    std::mutex _connection_map_mutex;

    static thread_local std::unordered_map<uint64_t, std::shared_ptr<TimerEvent>> __all_timer_event_map;
};

}

#endif