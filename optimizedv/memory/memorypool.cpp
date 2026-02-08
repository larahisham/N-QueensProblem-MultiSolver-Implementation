#include "memorypool.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

MemoryPool::MemoryPool(size_t blockSize, size_t initialBlocks)
    : blockSize(std::max(blockSize, sizeof(Block))), poolSize(initialBlocks), freeList(nullptr) {
    allocatePool();
}

void MemoryPool::allocatePool() {
    // Allocate new pool
    auto pool = std::make_unique<char[]>(blockSize * poolSize);
    
    // Add all blocks to free list
    char* start = pool.get();
    for (size_t i = 0; i < poolSize; ++i) {
        Block* block = reinterpret_cast<Block*>(start + i * blockSize);
        block->next = freeList;
        freeList = block;
    }
    
    // Store pool ownership
    pools.push_back(std::move(pool));
}

void* MemoryPool::allocate() {
    if (!freeList) {
        allocatePool();
    }
    
    Block* block = freeList;
    freeList = freeList->next;
    
    // Clear memory (optional, but good practice)
    std::memset(block, 0, blockSize);
    
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    Block* block = static_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
}