#include<iostream>
#include<vector>
#include<chrono>
#include<fstream>
#include<algorithm>
#include<unordered_set>

// Memory management includes
#include "src/memory/memorytracker.h"
#include "src/memory/memorypool.h"

using namespace std;
using namespace chrono;

// Global memory pool for hill climbing
namespace {
    MemoryPool hillClimbPool(sizeof(vector<int>), 1000);
}

template<typename Allocator>
int numOfConflicts(const vector<int, Allocator>& board, int row, int col) {
    int conflicts = 0;
    for (int i = 0; i < board.size(); ++i) {
        if (i == row) continue;
        if (board[i] == col || abs(board[i] - col) == abs(i - row))
            conflicts++;
    }
    return conflicts;
}

bool hillClimb(vector<int, MemoryPoolAllocator<int>>& board, int max_steps) {
    // Enable memory tracking for this run
    #ifdef TRACK_MEMORY
    MemoryTracker::reset();
    MemoryTracker::enable();
    #endif
    
    int n = board.size();
    srand(time(nullptr));
    for (int i = 0; i < n; ++i)
        board[i] = rand() % n;
        
    for (int step = 0; step < max_steps; ++step) {
        vector<int, MemoryPoolAllocator<int>> conflicted_rows(hillClimbPool);
        for (int row = 0; row < n; ++row) {
            if (numOfConflicts(board, row, board[row]) > 0)
                conflicted_rows.push_back(row);
        }
        if (conflicted_rows.empty()) {
            #ifdef TRACK_MEMORY
            MemoryTracker::generateReport("hillclimb_success_memory.txt");
            #endif
            return true;
        }
        int row = conflicted_rows[rand() % conflicted_rows.size()];
        int best_col = board[row];
        int min_conflict = numOfConflicts(board, row, best_col);
        for (int col = 0; col < n; ++col) {
            int conflicts = numOfConflicts(board, row, col);
            if (conflicts < min_conflict) {
                min_conflict = conflicts;
                best_col = col;
            }
        }
        if (best_col == board[row]) {
            #ifdef TRACK_MEMORY
            MemoryTracker::generateReport("hillclimb_failed_memory.txt");
            #endif
            return false;
        }
        board[row] = best_col;
        
        // Periodic memory reporting (every 1000 steps)
        #ifdef TRACK_MEMORY
        if (step % 1000 == 0) {
            cout << "Step " << step << ": Memory usage = " 
                 << MemoryTracker::getCurrentUsage() << " bytes\n";
        }
        #endif
    }
    
    #ifdef TRACK_MEMORY
    MemoryTracker::generateReport("hillclimb_failed_memory.txt");
    #endif
    return false;
}

double runHillClimbing(int n, int max_steps = 1000000) {
    vector<int, MemoryPoolAllocator<int>> board(hillClimbPool);
    board.resize(n);
    
    auto start = high_resolution_clock::now();
    bool success = hillClimb(board, max_steps);
    auto end = high_resolution_clock::now();
    
    if (success) {
        cout << "Hill climbing SUCCESS for N = " << n << endl;
    } else {
        cout << "Hill climbing FAILED for N = " << n << endl;
    }
    
    return duration<double>(end - start).count();
}

int main() {
    srand(time(nullptr));
    
    #ifdef TRACK_MEMORY
    cout << "Memory tracking ENABLED for hill climbing\n";
    MemoryTracker::enable();
    #endif
    
    ofstream file("nqueens_hillclimbing_results.csv");
    vector<int> TstValues = { 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
    file << "N,Time(seconds)\n";
    cout << "Pure Hill Climbing Results:\n";
    
    for (int n : TstValues) {
        cout << "Running for N = " << n << "...\n";
        double time_taken = runHillClimbing(n);
        file << n << "," << time_taken << "\n";
        cout << "Time = " << time_taken << " seconds\n";
        
        #ifdef TRACK_MEMORY
        // Generate memory report for each N
        string filename = "hillclimb_memory_N" + to_string(n) + ".txt";
        MemoryTracker::generateReport(filename);
        MemoryTracker::reset(); // Reset for next run
        #endif
    }
    
    file.close();
    
    #ifdef TRACK_MEMORY
    // Final leak check
    MemoryTracker::generateLeakReport("hillclimb_final_leaks.txt");
    #endif
    
    return 0;
}