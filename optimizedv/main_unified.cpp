#include <iostream>
#include "src/memory/memorytracker.h"

// Simple test to verify memory tracking works
void testMemoryTracking() {
    std::cout << "=== Memory Tracking Test ===\n";
    
    MemoryTracker::enable();
    MemoryTracker::reset();
    
    // Test allocations
    int* array1 = new int[100];
    double* array2 = new double[50];
    char* buffer = new char[1024];
    
    std::cout << "Current memory usage: " << MemoryTracker::getCurrentUsage() << " bytes\n";
    std::cout << "Allocation count: " << MemoryTracker::getAllocationCount() << "\n";
    
    delete[] array1;
    delete[] array2;
    
    std::cout << "After deletions: " << MemoryTracker::getCurrentUsage() << " bytes\n";
    
    // Generate reports
    MemoryTracker::generateReport("test_memory_report.txt");
    
    // Intentionally leak buffer to test leak detection
    // delete[] buffer; // Commented out to create leak
    
    MemoryTracker::generateLeakReport("test_memory_leaks.txt");
    
    std::cout << "Test completed. Check test_memory_report.txt and test_memory_leaks.txt\n";
    
    MemoryTracker::disable();
}

void testArenaAllocator() {
    std::cout << "\n=== Arena Allocator Test ===\n";
    
    #include "src/memory/arenaallocator.h"
    ArenaAllocator arena(1024); // 1KB arena
    
    // Allocate some memory
    int* arr1 = static_cast<int*>(arena.allocate(10 * sizeof(int), alignof(int)));
    double* arr2 = static_cast<double*>(arena.allocate(5 * sizeof(double), alignof(double)));
    
    // Use the memory
    for (int i = 0; i < 10; i++) arr1[i] = i * 2;
    for (int i = 0; i < 5; i++) arr2[i] = i * 1.5;
    
    std::cout << "Arena total memory: " << arena.getTotalMemory() << " bytes\n";
    std::cout << "Arena used memory: " << arena.getUsedMemory() << " bytes\n";
    std::cout << "Arena wasted memory: " << arena.getWastedMemory() << " bytes\n";
    
    arena.reset();
    std::cout << "After reset - used memory: " << arena.getUsedMemory() << " bytes\n";
}

int main() {
    std::cout << "Memory Management System Test\n";
    std::cout << "=============================\n\n";
    
    testMemoryTracking();
    testArenaAllocator();
    
    std::cout << "\nAll tests completed!\n";
    return 0;
}