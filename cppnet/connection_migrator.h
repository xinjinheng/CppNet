// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef CPPNET_CONNECTION_MIGRATOR
#define CPPNET_CONNECTION_MIGRATOR

#include <memory>
#include <atomic>
#include <vector>
#include <mutex>

namespace cppnet {

class CppNetBase;
class RWSocket;
class Dispatcher;
class LoadMonitor;

class ConnectionMigrator {
public:
    explicit ConnectionMigrator(std::shared_ptr<CppNetBase> cppnet_base);
    ~ConnectionMigrator();

    void Start();
    void Stop();
    void MigrateConnection(std::shared_ptr<RWSocket> socket, std::shared_ptr<Dispatcher> target_dispatcher);
    void MigrateConnectionsFromOverloadedDispatcher();

private:
    bool MigrateSingleConnection(std::shared_ptr<RWSocket> socket, std::shared_ptr<Dispatcher> target_dispatcher);
    std::vector<std::shared_ptr<RWSocket>> SelectConnectionsToMigrate(std::shared_ptr<Dispatcher> source_dispatcher, uint32_t count);
    void UpdateSocketDispatcher(std::shared_ptr<RWSocket> socket, std::shared_ptr<Dispatcher> new_dispatcher);

    std::shared_ptr<CppNetBase> _cppnet_base;
    std::shared_ptr<LoadMonitor> _load_monitor;
    std::atomic<bool> _running;
    std::mutex _migrate_mutex;
};

} // namespace cppnet

#endif