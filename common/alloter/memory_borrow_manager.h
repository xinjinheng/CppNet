// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef COMMON_ALLOTER_MEMORY_BORROW_MANAGER
#define COMMON_ALLOTER_MEMORY_BORROW_MANAGER

#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace cppnet {

class BlockMemoryPool;

class MemoryBorrowManager {
public:
    MemoryBorrowManager() = default;
    ~MemoryBorrowManager() = default;
    
    // Singleton instance
    static MemoryBorrowManager& Instance();
    
    // Register a memory pool with an ID
    void RegisterPool(uint32_t pool_id, std::shared_ptr<BlockMemoryPool> pool);
    
    // Unregister a memory pool
    void UnregisterPool(uint32_t pool_id);
    
    // Borrow memory blocks from another pool
    void* BorrowMemory(uint32_t requester_pool_id, uint32_t size, uint32_t num = 1);
    
    // Return borrowed memory blocks
    void ReturnMemory(uint32_t original_pool_id, void* mem, uint32_t size);
    
    // Get the total number of registered pools
    uint32_t GetPoolCount() const;
    
private:
    mutable std::mutex _mutex;
    std::unordered_map<uint32_t, std::shared_ptr<BlockMemoryPool>> _pool_map;
};

} // namespace cppnet

#endif