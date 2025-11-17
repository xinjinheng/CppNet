// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "connection_migrator.h"
#include "cppnet_base.h"
#include "dispatcher.h"
#include "socket/rw_socket.h"
#include "event/event_interface.h"

namespace cppnet {

ConnectionMigrator::ConnectionMigrator(std::shared_ptr<CppNetBase> cppnet_base)
    : _cppnet_base(cppnet_base),
      _load_monitor(cppnet_base->GetLoadMonitor()),
      _running(false) {
}

ConnectionMigrator::~ConnectionMigrator() {
    Stop();
}

void ConnectionMigrator::Start() {
    _running = true;
}

void ConnectionMigrator::Stop() {
    _running = false;
}

void ConnectionMigrator::MigrateConnection(std::shared_ptr<RWSocket> socket, std::shared_ptr<Dispatcher> target_dispatcher) {
    std::lock_guard<std::mutex> lock(_migrate_mutex);
    MigrateSingleConnection(socket, target_dispatcher);
}

void ConnectionMigrator::MigrateConnectionsFromOverloadedDispatcher() {
    if (!_running) {
        return;
    }

    std::shared_ptr<Dispatcher> source_dispatcher = _load_monitor->GetMostLoadedDispatcher();
    if (!source_dispatcher) {
        return;
    }

    std::shared_ptr<Dispatcher> target_dispatcher = _load_monitor->GetLeastLoadedDispatcher();
    if (!target_dispatcher || source_dispatcher == target_dispatcher) {
        return;
    }

    // 选择需要迁移的连接
    std::vector<std::shared_ptr<RWSocket>> connections_to_migrate = SelectConnectionsToMigrate(source_dispatcher, 10);
    
    for (auto& socket : connections_to_migrate) {
        MigrateSingleConnection(socket, target_dispatcher);
    }
}

bool ConnectionMigrator::MigrateSingleConnection(std::shared_ptr<RWSocket> socket, std::shared_ptr<Dispatcher> target_dispatcher) {
    if (!socket || !target_dispatcher) {
        return false;
    }

    // 1. 暂停原线程的IO操作
    std::shared_ptr<Dispatcher> source_dispatcher = socket->GetDispatcher();
    if (!source_dispatcher || source_dispatcher == target_dispatcher) {
        return false;
    }

    // 2. 从原事件驱动中移除socket
    std::shared_ptr<EventActions> source_event_actions = socket->GetEventActions();
    if (!source_event_actions) {
        return false;
    }

    // 3. 更新socket的dispatcher
    UpdateSocketDispatcher(socket, target_dispatcher);

    // 4. 在新线程的事件驱动中重新注册socket
    std::shared_ptr<EventActions> target_event_actions = target_dispatcher->GetEventActions();
    if (!target_event_actions) {
        return false;
    }

    // 5. 恢复IO操作
    // 重新注册事件
    // 这里需要根据socket的状态重新添加相应的事件
    // 例如：如果socket有未完成的读取或写入操作，需要重新注册

    return true;
}

std::vector<std::shared_ptr<RWSocket>> ConnectionMigrator::SelectConnectionsToMigrate(std::shared_ptr<Dispatcher> source_dispatcher, uint32_t count) {
    // 这个函数需要根据实际情况实现
    // 这里简单返回一个空向量
    return std::vector<std::shared_ptr<RWSocket>>();
}

void ConnectionMigrator::UpdateSocketDispatcher(std::shared_ptr<RWSocket> socket, std::shared_ptr<Dispatcher> new_dispatcher) {
    if (socket) {
        socket->SetDispatcher(new_dispatcher);
    }
}

} // namespace cppnet