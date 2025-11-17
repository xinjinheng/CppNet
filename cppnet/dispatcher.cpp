// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include <map>

#include "dispatcher.h"
#include "cppnet_base.h"
#include "socket/rw_socket.h"
#include "event/timer_event.h"
#include "socket/connect_socket.h"
#include "event/action_interface.h"

#include "common/util/time.h"
#include "common/timer/timer.h"
#include "common/alloter/pool_alloter.h"

namespace cppnet {

thread_local std::unordered_map<uint64_t, std::shared_ptr<TimerEvent>> Dispatcher::__all_timer_event_map;

Dispatcher::Dispatcher(std::shared_ptr<CppNetBase> base, uint32_t thread_num, uint32_t base_id):
    _cur_utc_time(0),
    _timer_id_creater(0),
    _connection_count(0),
    _cppnet_base(base) {

    _timer = MakeTimer1Min();

    _event_actions = MakeEventActions();
    _event_actions->Init();

    // start thread
    Start();
}

Dispatcher::Dispatcher(std::shared_ptr<CppNetBase> base, uint32_t base_id):
    _cur_utc_time(0),
    _timer_id_creater(0),
    _connection_count(0),
    _cppnet_base(base) {

    _timer = MakeTimer1Min();

    _event_actions = MakeEventActions();
    _event_actions->Init();

    // start thread
    Start();
}

Dispatcher::~Dispatcher() {
    if (std::this_thread::get_id() != _local_thread_id) {
        Stop();
        Join();
    }
}

void Dispatcher::Run() {
    _local_thread_id = std::this_thread::get_id();
    _cur_utc_time = UTCTimeMsec();
    int32_t wait_time = 0;
    uint64_t cur_time = 0;

    while (!_stop) {
        cur_time = UTCTimeMsec();
        _timer->TimerRun(uint32_t(cur_time - _cur_utc_time));
        _cur_utc_time = cur_time;

        if (_stop) {
            break;
        }

        wait_time = _timer->MinTime();

        _event_actions->ProcessEvent(wait_time);

        DoTask();
    }
}

void Dispatcher::Stop() {
    _stop = true;
    _event_actions->Wakeup();
}

void Dispatcher::Listen(uint64_t sock, const std::string& ip, uint16_t port) {
    auto task = [sock, ip, port, this]() {
        auto connect_sock = MakeConnectSocket();
        connect_sock->SetEventActions(_event_actions);
        connect_sock->SetCppNetBase(_cppnet_base.lock());
        connect_sock->SetSocket(sock);
        connect_sock->SetDispatcher(shared_from_this());

        connect_sock->Bind(ip, port);
        connect_sock->Listen();
    };

    if (std::this_thread::get_id() == _local_thread_id) {
        task();

    } else {
        PostTask(task);
    }
}

void Dispatcher::Connect(const std::string& ip, uint16_t port) {
    auto task = [ip, port, this]() {
        auto sock = MakeRWSocket();
        sock->SetDispatcher(shared_from_this());
        sock->SetEventActions(_event_actions);
        sock->SetCppNetBase(_cppnet_base.lock());
        sock->Connect(ip, port);
    };

    if (std::this_thread::get_id() == _local_thread_id) {
        task();

    } else {
        PostTask(task);
    }
}

void Dispatcher::PostTask(const Task& task) {
    {
        std::unique_lock<std::mutex> lock(_task_list_mutex);
        _task_list.push_back(task);
    }
    _event_actions->Wakeup();
}

uint32_t Dispatcher::AddTimer(const user_timer_call_back& cb, void* param, uint32_t interval, bool always) {
    std::shared_ptr<TimerEvent> event = std::make_shared<TimerEvent>();
    event->AddType(ET_USER_TIMER);
    event->SetTimerCallBack(cb, param);

    uint32_t timer_id = MakeTimerID();

    if (std::this_thread::get_id() == _local_thread_id) {
        _timer->AddTimer(event, interval, always);
        __all_timer_event_map[timer_id] = event;
        _event_actions->Wakeup();
        
    } else {
        auto task = [event, timer_id, interval, always, this]() {
            _timer->AddTimer(event, interval, always);
            __all_timer_event_map[timer_id] = event;
        };
        PostTask(task);
    }
    return timer_id;
}

uint32_t Dispatcher::AddTimer(std::shared_ptr<RWSocket> sock, uint32_t interval, bool always) {
    std::shared_ptr<TimerEvent> event = std::make_shared<TimerEvent>();
    event->AddType(ET_TIMER);
    event->SetSocket(sock);

    uint32_t timer_id = MakeTimerID();

    if (std::this_thread::get_id() == _local_thread_id) {
        _timer->AddTimer(event, interval, always);
        __all_timer_event_map[timer_id] = event;
        _event_actions->Wakeup();
        
    } else {
        auto task = [event, timer_id, interval, always, this]() {
            _timer->AddTimer(event, interval, always);
            __all_timer_event_map[timer_id] = event;
        };
        PostTask(task);
    }
    return timer_id;
}

void Dispatcher::StopTimer(uint32_t timer_id) {
    if (std::this_thread::get_id() == _local_thread_id) {
        auto iter = __all_timer_event_map.find(timer_id);
        if (iter == __all_timer_event_map.end()) {
            return;
        }
        
        _timer->RmTimer(iter->second);
        __all_timer_event_map.erase(iter);

    } else {
        auto task = [timer_id, this]() {
            auto iter = __all_timer_event_map.find(timer_id);
            if (iter == __all_timer_event_map.end()) {
                return;
            }

            _timer->RmTimer(iter->second);
            __all_timer_event_map.erase(iter);
        };
        PostTask(task);
    }
}

void Dispatcher::DoTask() {
    std::vector<Task> func_vec;
    {
        std::unique_lock<std::mutex> lock(_task_list_mutex);
        func_vec.swap(_task_list);
    }

    for (std::size_t i = 0; i < func_vec.size(); ++i) {
        func_vec[i]();
    }
}

uint32_t Dispatcher::MakeTimerID() {
    std::unique_lock<std::mutex> lock(_timer_id_mutex);
    return ++_timer_id_creater;
}

void Dispatcher::AddConnection(uint64_t sockfd, std::shared_ptr<RWSocket> sock) {
    std::unique_lock<std::mutex> lock(_connection_mutex);
    _connection_map[sockfd] = sock;
    _connection_count++;

    // 更新负载监控
    auto base = _cppnet_base.lock();
    if (base) {
        auto load_monitor = base->GetLoadMonitor();
        if (load_monitor) {
            load_monitor->UpdateDispatcherLoad(shared_from_this(), _connection_count);
        }
    }
}

void Dispatcher::RemoveConnection(uint64_t sockfd) {
    std::unique_lock<std::mutex> lock(_connection_mutex);
    auto iter = _connection_map.find(sockfd);
    if (iter != _connection_map.end()) {
        _connection_map.erase(iter);
        _connection_count--;

        // 更新负载监控
        auto base = _cppnet_base.lock();
        if (base) {
            auto load_monitor = base->GetLoadMonitor();
            if (load_monitor) {
                load_monitor->UpdateDispatcherLoad(shared_from_this(), _connection_count);
            }
        }
    }
}

uint32_t Dispatcher::GetConnectionCount() {
    std::unique_lock<std::mutex> lock(_connection_mutex);
    return _connection_count;
}

std::unordered_map<uint64_t, std::shared_ptr<RWSocket>> Dispatcher::GetAllConnections() {
    std::unique_lock<std::mutex> lock(_connection_mutex);
    return _connection_map;
}

}
