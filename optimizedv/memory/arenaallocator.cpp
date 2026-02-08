#include "arenaallocator.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

ArenaAllocator::ArenaAllocator(size_t initialBlockSize) 
    : blockSize(initialBlockSize), currentBlock(0) {
    allocateNewBlock();
}

void ArenaAllocator::allocateNewBlock(size_t minSize) {
    size_t newSize = std::max(blockSize, minSize);
    blocks.emplace_back(newSize);
    currentBlock = blocks.size() - 1;
    blockSize = newSize * 2; // Double size for next allocation
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    if (size == 0) return nullptr;
    
    ArenaBlock& block = blocks[currentBlock];
    
    // Calculate aligned offset
    size_t offset = block.used;
    size_t padding = (alignment - (offset % alignment)) % alignment;
    
    // Check if we have enough space
    if (offset + padding + size > block.size) {
        allocateNewBlock(size + padding);
        return allocate(size, alignment); // Recursive call with new block
    }
    
    void* ptr = block.memory.get() + offset + padding;
    block.used = offset + padding + size;
    
    // Clear memory (optional)
    std::memset(ptr, 0, size);
    
    return ptr;
}

void ArenaAllocator::reset() {
    for (auto& block : blocks) {
        block.used = 0;
    }
    currentBlock = 0;
}

void ArenaAllocator::clear() {
    blocks.clear();
    currentBlock = 0;
    blockSize = 65536;
    allocateNewBlock();
}

size_t ArenaAllocator::getTotalMemory() const {
    size_t total = 0;
    for (const auto& block : blocks) {
        total += block.size;
    }
    return total;
}

size_t ArenaAllocator::getUsedMemory() const {
    size_t used = 0;
    for (const auto& block : blocks) {
        used += block.used;
    }
    return used;
}

size_t ArenaAllocator::getWastedMemory() const {
    return getTotalMemory() - getUsedMemory();
}