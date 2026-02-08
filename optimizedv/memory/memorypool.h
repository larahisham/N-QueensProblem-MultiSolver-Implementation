#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <memory>
#include <cstddef>

class MemoryPool {
private:
    struct Block {
        Block* next;
    };
    
    size_t blockSize;
    size_t poolSize;
    std::vector<std::unique_ptr<char[]>> pools;
    Block* freeList;
    
    void allocatePool();
    
public:
    MemoryPool(size_t blockSize, size_t initialBlocks = 1024);
    ~MemoryPool() = default;
    
    void* allocate();
    void deallocate(void* ptr);
    
    // Disable copying
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    // Allow moving
    MemoryPool(MemoryPool&&) = default;
    MemoryPool& operator=(MemoryPool&&) = default;
};

template<typename T>
class MemoryPoolAllocator {
private:
    MemoryPool& pool;
    
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::false_type;
    
    MemoryPoolAllocator(MemoryPool& pool) noexcept : pool(pool) {}
    
    template<typename U>
    MemoryPoolAllocator(const MemoryPoolAllocator<U>& other) noexcept : pool(other.pool) {}
    
    T* allocate(size_t n) {
        if (n != 1) {
            // For arrays, fall back to new
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        return static_cast<T*>(pool.allocate());
    }
    
    void deallocate(T* ptr, size_t n) {
        if (n != 1) {
            ::operator delete(ptr);
        } else {
            pool.deallocate(ptr);
        }
    }
    
    template<typename U>
    bool operator==(const MemoryPoolAllocator<U>& other) const noexcept {
        return &pool == &other.pool;
    }
    
    template<typename U>
    bool operator!=(const MemoryPoolAllocator<U>& other) const noexcept {
        return !(*this == other);
    }
};

#endif // MEMORY_POOL_H