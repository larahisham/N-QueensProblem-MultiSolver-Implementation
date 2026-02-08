#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <fstream>

struct AllocationInfo {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    uint64_t timestamp;
    std::string stackTrace;
};

class MemoryTracker {
private:
    static std::unordered_map<void*, AllocationInfo> allocations;
    static std::mutex mutex;
    static std::atomic<uint64_t> totalAllocated;
    static std::atomic<uint64_t> totalFreed;
    static std::atomic<uint64_t> peakUsage;
    static std::atomic<uint64_t> currentUsage;
    static std::atomic<uint64_t> allocationCount;
    static std::atomic<uint64_t> fragmentation;
    static bool enabled;
    
    static std::string getStackTrace(int maxDepth = 10);
    
public:
    static void enable();
    static void disable();
    static void reset();
    
    static void* trackAlloc(size_t size, const char* file = __builtin_FILE(), int line = __builtin_LINE());
    static void trackFree(void* ptr);
    
    // Statistics
    static uint64_t getTotalAllocated() { return totalAllocated.load(); }
    static uint64_t getTotalFreed() { return totalFreed.load(); }
    static uint64_t getPeakUsage() { return peakUsage.load(); }
    static uint64_t getCurrentUsage() { return currentUsage.load(); }
    static uint64_t getAllocationCount() { return allocationCount.load(); }
    static double getFragmentationPercentage();
    
    // Reports
    static void generateReport(const std::string& filename = "memory_report.txt");
    static void generateLeakReport(const std::string& filename = "memory_leaks.txt");
    static void logAllocation(void* ptr, size_t size, const char* file, int line);
    static void logDeallocation(void* ptr);
    
    // Hook functions for malloc/free overrides
    static void* malloc_hook(size_t size, const char* file, int line);
    static void free_hook(void* ptr);
    
    // Memory fragmentation analysis
    static void analyzeFragmentation();
};

// Macro for easy tracking
#ifdef TRACK_MEMORY
    #define TRACK_NEW new(__FILE__, __LINE__)
    #define MEM_ALLOC(size) MemoryTracker::malloc_hook(size, __FILE__, __LINE__)
    #define MEM_FREE(ptr) MemoryTracker::free_hook(ptr)
#else
    #define TRACK_NEW new
    #define MEM_ALLOC(size) malloc(size)
    #define MEM_FREE(ptr) free(ptr)
#endif

// Overload operator new for tracking
void* operator new(size_t size, const char* file, int line);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* ptr, const char* file, int line) noexcept;
void operator delete[](void* ptr, const char* file, int line) noexcept;

// Ensure normal delete still works
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

#endif // MEMORY_TRACKER_H