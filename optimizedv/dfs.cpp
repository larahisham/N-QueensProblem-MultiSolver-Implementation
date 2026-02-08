#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>

// Memory management includes
#include "src/memory/memorytracker.h"
#include "src/memory/memorypool.h"
#include "src/memory/arenaallocator.h"

using namespace std;
using namespace std::chrono;

// Global memory pools
namespace {
    MemoryPool dfsBoardPool(sizeof(vector<int>), 1000);
    ArenaAllocator dfsArena(4096);
}

template <typename Allocator>
bool is_safe(const vector<int, Allocator>& assignment, int row, int col) {
    for (int r = 0; r < row; ++r) {
        int c = assignment[r];
        if (c == col || abs(c - col) == abs(r - row))
            return false;
    } 
    return true;
}

// Backtracking function using memory pool
void solve_all(vector<int, MemoryPoolAllocator<int>>& board, int row, int n, int& count) {
    if (row == n) {
        count++;
        return;
    }
    
    for (int col = 0; col < n; ++col) {
        if (is_safe(board, row, col)) {
            board[row] = col;
            solve_all(board, row + 1, n, count);
        }
    }
}

double dfs_blind(int n, int& solution_count) {
    #ifdef TRACK_MEMORY
    MemoryTracker::reset();
    MemoryTracker::enable();
    #endif
    
    vector<int, MemoryPoolAllocator<int>> board(dfsBoardPool);
    board.resize(n, -1);
    solution_count = 0;
    
    auto start = high_resolution_clock::now();
    solve_all(board, 0, n, solution_count);
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    
    #ifdef TRACK_MEMORY
    string filename = "dfs_memory_N" + to_string(n) + ".txt";
    MemoryTracker::generateReport(filename);
    MemoryTracker::analyzeFragmentation();
    #endif
    
    dfsArena.reset(); // Clean up arena
    
    return duration.count();
}

int main() {
    vector<int> TstValues = { 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
    
    #ifdef TRACK_MEMORY
    cout << "Memory tracking ENABLED for DFS\n";
    #endif
    
    ofstream csv("nqueens_dfs_results.csv");
    csv << "N,Time(seconds),Solutions\n";
    cout << "DFS - blindly searching all solutions for N-Queens...\n";
    
    for (int n : TstValues) {
        cout << "Running for N = " << n << "...\n";
        int solution_count = 0;
        double time_taken = dfs_blind(n, solution_count);
        cout << "Time taken: " << time_taken << " seconds, Solutions: " << solution_count << "\n";
        csv << n << "," << time_taken << "," << solution_count << "\n";
    }
    
    csv.close();
    
    #ifdef TRACK_MEMORY
    MemoryTracker::generateLeakReport("dfs_final_leaks.txt");
    #endif
    
    cout << "Results saved to nqueens_dfs_results.csv\n";
    return 0;
}