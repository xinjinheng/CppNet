// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef COMMON_NETWORK_CONNECTION_MIGRATOR
#define COMMON_NETWORK_CONNECTION_MIGRATOR

#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace cppnet {

class RWSocket;
class Dispatcher;

class ConnectionMigrator {
public:
    ConnectionMigrator();
    ~ConnectionMigrator() = default;
    
    // Migrate a connection from source to target dispatcher
    bool MigrateConnection(std::shared_ptr<RWSocket> sock, 
                          std::shared_ptr<Dispatcher> source_dispatcher, 
                          std::shared_ptr<Dispatcher> target_dispatcher);
    
    // Check if migration is in progress for a socket
    bool IsMigrationInProgress(uint64_t sock_fd) const;
    
private:
    // Migration states
    enum class MigrationState {
        kIdle,
        kPreparing,
        kMigrating,
        kCompleted,
        kFailed
    };
    
    // Migration context
    struct MigrationContext {
        std::shared_ptr<RWSocket> socket;
        std::shared_ptr<Dispatcher> source_dispatcher;
        std::shared_ptr<Dispatcher> target_dispatcher;
        MigrationState state;
        std::mutex mutex;
        std::condition_variable cv;
        
        MigrationContext() : state(MigrationState::kIdle) {}
    };
    
    // Internal migration steps
    bool PrepareMigration(std::shared_ptr<MigrationContext> ctx);
    bool MigrateSocket(std::shared_ptr<MigrationContext> ctx);
    bool MigrateBuffers(std::shared_ptr<MigrationContext> ctx);
    bool MigrateEvents(std::shared_ptr<MigrationContext> ctx);
    bool CompleteMigration(std::shared_ptr<MigrationContext> ctx);
    
    // Migration context map
    std::unordered_map<uint64_t, std::shared_ptr<MigrationContext>> _migration_map;
    mutable std::mutex _migration_map_mutex;
    
    // Atomic flag for migration in progress
    std::atomic<bool> _migration_in_progress{false};
};

} // namespace cppnet

#endif