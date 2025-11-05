// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "connection_migrator.h"
#include "cppnet/socket/rw_socket.h"
#include "cppnet/dispatcher.h"
#include "cppnet/event/action_interface.h"

namespace cppnet {

ConnectionMigrator::ConnectionMigrator() {
}

bool ConnectionMigrator::MigrateConnection(std::shared_ptr<RWSocket> sock, 
                                           std::shared_ptr<Dispatcher> source_dispatcher, 
                                           std::shared_ptr<Dispatcher> target_dispatcher) {
    if (!sock || !source_dispatcher || !target_dispatcher) {
        return false;
    }
    
    uint64_t sock_fd = sock->GetSocket();
    
    // Check if migration is already in progress for this socket
    { 
        std::unique_lock<std::mutex> lock(_migration_map_mutex);
        auto iter = _migration_map.find(sock_fd);
        if (iter != _migration_map.end() && iter->second->state != MigrationState::kCompleted && iter->second->state != MigrationState::kFailed) {
            return false;
        }
    }
    
    // Create migration context
    auto ctx = std::make_shared<MigrationContext>();
    ctx->socket = sock;
    ctx->source_dispatcher = source_dispatcher;
    ctx->target_dispatcher = target_dispatcher;
    ctx->state = MigrationState::kPreparing;
    
    // Add to migration map
    { 
        std::unique_lock<std::mutex> lock(_migration_map_mutex);
        _migration_map[sock_fd] = ctx;
    }
    
    bool success = false;
    try {
        // Step 1: Prepare migration
        if (!PrepareMigration(ctx)) {
            throw std::runtime_error("Migration preparation failed");
        }
        
        // Step 2: Migrate socket
        if (!MigrateSocket(ctx)) {
            throw std::runtime_error("Socket migration failed");
        }
        
        // Step 3: Migrate buffers
        if (!MigrateBuffers(ctx)) {
            throw std::runtime_error("Buffer migration failed");
        }
        
        // Step 4: Migrate events
        if (!MigrateEvents(ctx)) {
            throw std::runtime_error("Event migration failed");
        }
        
        // Step 5: Complete migration
        if (!CompleteMigration(ctx)) {
            throw std::runtime_error("Migration completion failed");
        }
        
        success = true;
        ctx->state = MigrationState::kCompleted;
        
    } catch (const std::exception& e) {
        ctx->state = MigrationState::kFailed;
        // TODO: Log error
    }
    
    // Notify waiting threads
    { 
        std::unique_lock<std::mutex> lock(ctx->mutex);
        ctx->cv.notify_all();
    }
    
    return success;
}

bool ConnectionMigrator::IsMigrationInProgress(uint64_t sock_fd) const {
    std::unique_lock<std::mutex> lock(_migration_map_mutex);
    auto iter = _migration_map.find(sock_fd);
    if (iter == _migration_map.end()) {
        return false;
    }
    
    MigrationState state = iter->second->state;
    return state == MigrationState::kPreparing || state == MigrationState::kMigrating;
}

bool ConnectionMigrator::PrepareMigration(std::shared_ptr<MigrationContext> ctx) {
    if (!ctx || !ctx->socket) {
        return false;
    }
    
    // Mark socket as migrating
    ctx->socket->SetContext(reinterpret_cast<void*>(this));
    
    // Stop any pending operations on the socket
    ctx->source_dispatcher->PostTask([ctx]() {
        // TODO: Implement proper socket operation stopping
        // This might involve canceling pending I/O, stopping timers, etc.
    });
    
    ctx->state = MigrationState::kMigrating;
    return true;
}

bool ConnectionMigrator::MigrateSocket(std::shared_ptr<MigrationContext> ctx) {
    if (!ctx || !ctx->socket || !ctx->source_dispatcher || !ctx->target_dispatcher) {
        return false;
    }
    
    // Remove socket from source dispatcher
    ctx->source_dispatcher->PostTask([ctx]() {
        ctx->source_dispatcher->RemoveConnection(ctx->socket);
    });
    
    // Add socket to target dispatcher
    ctx->target_dispatcher->PostTask([ctx]() {
        ctx->target_dispatcher->AddConnection(ctx->socket);
    });
    
    // Update socket's dispatcher and event actions
    ctx->socket->SetDispatcher(ctx->target_dispatcher);
    ctx->socket->SetEventActions(ctx->target_dispatcher->GetEventActions());
    
    return true;
}

bool ConnectionMigrator::MigrateBuffers(std::shared_ptr<MigrationContext> ctx) {
    if (!ctx || !ctx->socket) {
        return false;
    }
    
    // Buffers are already owned by the socket, so no need to migrate them
    // They will be accessible from the new dispatcher thread
    return true;
}

bool ConnectionMigrator::MigrateEvents(std::shared_ptr<MigrationContext> ctx) {
    if (!ctx || !ctx->socket || !ctx->source_dispatcher || !ctx->target_dispatcher) {
        return false;
    }
    
    // Remove event from source dispatcher
    ctx->source_dispatcher->PostTask([ctx]() {
        auto event_actions = ctx->source_dispatcher->GetEventActions();
        if (event_actions) {
            // TODO: Remove the socket's events from the source dispatcher
            // This might involve canceling read/write events
        }
    });
    
    // Add event to target dispatcher
    ctx->target_dispatcher->PostTask([ctx]() {
        auto event_actions = ctx->target_dispatcher->GetEventActions();
        if (event_actions) {
            // TODO: Add the socket's events to the target dispatcher
            // This might involve registering read/write events
        }
    });
    
    return true;
}

bool ConnectionMigrator::CompleteMigration(std::shared_ptr<MigrationContext> ctx) {
    if (!ctx || !ctx->socket) {
        return false;
    }
    
    // Clear migration context from socket
    ctx->socket->SetContext(nullptr);
    
    // Resume socket operations
    ctx->target_dispatcher->PostTask([ctx]() {
        // TODO: Implement proper socket operation resuming
        // This might involve restarting timers, resuming I/O, etc.
        ctx->socket->Read(); // Example: resume reading
    });
    
    ctx->state = MigrationState::kCompleted;
    return true;
}

} // namespace cppnet