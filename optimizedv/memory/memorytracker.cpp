#include "memorytracker.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#ifdef __linux__
#include <execinfo.h>
#endif

// Initialize static members
std::unordered_map<void*, AllocationInfo> MemoryTracker::allocations;
std::mutex MemoryTracker::mutex;
std::atomic<uint64_t> MemoryTracker::totalAllocated(0);
std::atomic<uint64_t> MemoryTracker::totalFreed(0);
std::atomic<uint64_t> MemoryTracker::peakUsage(0);
std::atomic<uint64_t> MemoryTracker::currentUsage(0);
std::atomic<uint64_t> MemoryTracker::allocationCount(0);
std::atomic<uint64_t> MemoryTracker::fragmentation(0);
bool MemoryTracker::enabled = false;

std::string MemoryTracker::getStackTrace(int maxDepth) {
    std::stringstream ss;
    
#ifdef __linux__
    void* array[maxDepth];
    char** strings;
    int size;
    
    size = backtrace(array, maxDepth);
    strings = backtrace_symbols(array, size);
    
    if (strings != nullptr) {
        for (int i = 1; i < size; i++) { // Skip first (this function)
            ss << strings[i] << "\n";
        }
        free(strings);
    }
#else
    ss << "Stack trace not available on this platform\n";
#endif
    
    return ss.str();
}

void MemoryTracker::enable() {
    enabled = true;
}

void MemoryTracker::disable() {
    enabled = false;
}

void MemoryTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    allocations.clear();
    totalAllocated = 0;
    totalFreed = 0;
    peakUsage = 0;
    currentUsage = 0;
    allocationCount = 0;
    fragmentation = 0;
}

void* MemoryTracker::trackAlloc(size_t size, const char* file, int line) {
    if (!enabled) {
        return malloc(size);
    }
    
    void* ptr = malloc(size);
    if (!ptr) {
        return nullptr;
    }
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    AllocationInfo info;
    info.ptr = ptr;
    info.size = size;
    info.file = file;
    info.line = line;
    info.timestamp = timestamp;
    info.stackTrace = getStackTrace();
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        allocations[ptr] = info;
    }
    
    totalAllocated += size;
    currentUsage += size;
    allocationCount++;
    
    if (currentUsage.load() > peakUsage.load()) {
        peakUsage.store(currentUsage.load());
    }
    
    return ptr;
}

void MemoryTracker::trackFree(void* ptr) {
    if (!ptr || !enabled) {
        free(ptr);
        return;
    }
    
    size_t size = 0;
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = allocations.find(ptr);
        if (it != allocations.end()) {
            size = it->second.size;
            allocations.erase(it);
        }
    }
    
    totalFreed += size;
    currentUsage -= size;
    free(ptr);
}

double MemoryTracker::getFragmentationPercentage() {
    if (peakUsage == 0) return 0.0;
    return (static_cast<double>(fragmentation) / peakUsage) * 100.0;
}

void MemoryTracker::generateReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "=== Memory Usage Report ===\n";
    file << "Total Allocated: " << getTotalAllocated() << " bytes\n";
    file << "Total Freed: " << getTotalFreed() << " bytes\n";
    file << "Current Usage: " << getCurrentUsage() << " bytes\n";
    file << "Peak Usage: " << getPeakUsage() << " bytes\n";
    file << "Allocation Count: " << getAllocationCount() << "\n";
    file << "Fragmentation: " << getFragmentationPercentage() << "%\n";
    file << "Active Allocations: " << allocations.size() << "\n\n";
    
    if (!allocations.empty()) {
        file << "=== Active Allocations ===\n";
        for (const auto& [ptr, info] : allocations) {
            file << "Ptr: " << ptr << " | Size: " << info.size 
                 << " bytes | File: " << info.file << ":" << info.line << "\n";
        }
    }
    
    file.close();
}

void MemoryTracker::generateLeakReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "=== Memory Leak Report ===\n";
    file << "Total Leaks: " << allocations.size() << "\n";
    file << "Total Leaked Memory: " << currentUsage << " bytes\n\n";
    
    for (const auto& [ptr, info] : allocations) {
        file << "Leak at " << info.file << ":" << info.line << "\n";
        file << "Size: " << info.size << " bytes\n";
        file << "Address: " << ptr << "\n";
        file << "Stack Trace:\n" << info.stackTrace << "\n";
        file << "-------------------\n";
    }
    
    file.close();
}

void MemoryTracker::logAllocation(void* ptr, size_t size, const char* file, int line) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << "[ALLOC] " << ptr << " | " << size << " bytes | " 
              << file << ":" << line << "\n";
}

void MemoryTracker::logDeallocation(void* ptr) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << "[FREE] " << ptr << "\n";
}

void* MemoryTracker::malloc_hook(size_t size, const char* file, int line) {
    return trackAlloc(size, file, line);
}

void MemoryTracker::free_hook(void* ptr) {
    trackFree(ptr);
}

void MemoryTracker::analyzeFragmentation() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (allocations.empty()) {
        fragmentation = 0;
        return;
    }
    
    // Sort allocations by address
    std::vector<AllocationInfo> sorted;
    sorted.reserve(allocations.size());
    
    for (const auto& [ptr, info] : allocations) {
        sorted.push_back(info);
    }
    
    std::sort(sorted.begin(), sorted.end(), 
              [](const AllocationInfo& a, const AllocationInfo& b) {
                  return reinterpret_cast<uintptr_t>(a.ptr) < reinterpret_cast<uintptr_t>(b.ptr);
              });
    
    // Calculate fragmentation (gaps between allocations)
    uint64_t totalGap = 0;
    for (size_t i = 1; i < sorted.size(); i++) {
        uintptr_t prevEnd = reinterpret_cast<uintptr_t>(sorted[i-1].ptr) + sorted[i-1].size;
        uintptr_t currStart = reinterpret_cast<uintptr_t>(sorted[i].ptr);
        if (currStart > prevEnd) {
            totalGap += (currStart - prevEnd);
        }
    }
    
    fragmentation = totalGap;
}

// Operator new overloads
void* operator new(size_t size, const char* file, int line) {
    return MemoryTracker::trackAlloc(size, file, line);
}

void* operator new[](size_t size, const char* file, int line) {
    return MemoryTracker::trackAlloc(size, file, line);
}

void operator delete(void* ptr, const char* file, int line) noexcept {
    MemoryTracker::trackFree(ptr);
}

void operator delete[](void* ptr, const char* file, int line) noexcept {
    MemoryTracker::trackFree(ptr);
}

// Regular delete operators
void operator delete(void* ptr) noexcept {
    MemoryTracker::trackFree(ptr);
}

void operator delete[](void* ptr) noexcept {
    MemoryTracker::trackFree(ptr);
}