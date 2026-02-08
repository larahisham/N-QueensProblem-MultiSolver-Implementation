#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <unordered_set>

// Memory management includes
#include "src/memory/memorytracker.h"
#include "src/memory/memorypool.h"
#include "src/memory/arenaallocator.h"

using namespace std;
using namespace chrono;

// Global memory pools for CSP
namespace {
    MemoryPool cspPool(sizeof(vector<int>), 1000);
    ArenaAllocator cspArena(8192); // 8KB for CSP
}

struct CSPState {
    int n;
    vector<int, MemoryPoolAllocator<int>> assignment;
    vector<unordered_set<int>> domains;
    
    CSPState(int size) : n(size), assignment(cspPool), domains(size) {
        assignment.resize(size, -1);
    }
};

bool is_safe(const vector<int>& assignment, int row, int col) {
    for (int r = 0; r < row; ++r) {
        int c = assignment[r];
        if (c == col || abs(c - col) == abs(r - row))
            return false;
    } 
    return true;
}

// Level 2 forward check with arena allocator
bool forward_check(CSPState& state, int row, int col) {
    // Use arena allocator for temporary storage
    vector<int, ArenaAllocatorWrapper<int>> queue(cspArena);
    
    for (int r1 = row + 1; r1 < state.n; ++r1) {
        auto& domain1 = state.domains[r1];
        vector<int> to_remove;
        
        for (int c1 : domain1) {
            if (c1 == col || abs(c1 - col) == abs(r1 - row)) {
                to_remove.push_back(c1);
            }
        }
        
        for (int c1 : to_remove) {
            domain1.erase(c1);
        }
        
        if (domain1.empty())
            return false;
        if (!to_remove.empty())
            queue.push_back(r1);
    }
    
    while (!queue.empty()) {
        int r1 = queue.back();
        queue.pop_back();
        
        for (int r2 = r1 + 1; r2 < state.n; ++r2) {
            auto& domain2 = state.domains[r2];
            vector<int> to_remove;
            
            for (int c2 : domain2) {
                for (int c1 : state.domains[r1]) {
                    if (c2 == c1 || abs(c2 - c1) == abs(r2 - r1)) {
                        to_remove.push_back(c2);
                        break;
                    }
                }
            }
            
            for (int c2 : to_remove) {
                domain2.erase(c2);
            }
            
            if (domain2.empty())
                return false;
            if (!to_remove.empty())
                queue.push_back(r2);
        }
    }
    return true;
}

// LCV: least constraining value with arena allocator
vector<int, ArenaAllocatorWrapper<int>> sorted_lcv(const CSPState& state, int row) {
    vector<pair<int, int>, ArenaAllocatorWrapper<pair<int, int>>> col_constraints(cspArena);
    
    for (int col : state.domains[row]) {
        int count = 0;
        for (int r = row + 1; r < state.n; ++r) {
            for (int c : state.domains[r]) {
                if (c == col || abs(c - col) == abs(r - row))
                    ++count;
            }
        }
        col_constraints.push_back({ count, col });
    }
    
    sort(col_constraints.begin(), col_constraints.end());
    vector<int, ArenaAllocatorWrapper<int>> sorted_cols(cspArena);
    for (auto& pair : col_constraints)
        sorted_cols.push_back(pair.second);
    return sorted_cols;
}

int select_variable(const CSPState& state) {
    int min_domain_size = state.n + 1;
    int best_row = -1;
    int max_constraints = -1;
    
    for (int row = 0; row < state.n; ++row) {
        if (state.assignment[row] != -1)
            continue;
            
        int domain_size = state.domains[row].size();
        if (domain_size < min_domain_size) {
            min_domain_size = domain_size;
            best_row = row;
        }
        else if (domain_size == min_domain_size) {
            int constraints = 0;
            for (int other = 0; other < state.n; ++other) {
                if (other != row && state.assignment[other] == -1) {
                    constraints += state.domains[other].size();
                }
            }
            if (constraints > max_constraints) {
                max_constraints = constraints;
                best_row = row;
            }
        }
    } 
    return best_row;
}

bool solve(CSPState& state) {
    bool done = true;
    for (int i = 0; i < state.n; ++i)
        if (state.assignment[i] == -1)
            done = false;
    if (done) return true;
    
    int row = select_variable(state);
    vector<int, ArenaAllocatorWrapper<int>> values = sorted_lcv(state, row);
    
    for (int col : values) {
        CSPState new_state = state;
        new_state.assignment[row] = col;
        
        if (!forward_check(new_state, row, col))
            continue;
        
        if (solve(new_state)) {
            state = new_state;
            return true;
        }
    } 
    return false;
}

double dfs_csp(int n) {
    #ifdef TRACK_MEMORY
    MemoryTracker::reset();
    MemoryTracker::enable();
    #endif
    
    CSPState state(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            state.domains[i].insert(j);
        }
    }
    
    auto start = high_resolution_clock::now();
    bool solved = solve(state);
    auto end = high_resolution_clock::now();
    duration<double> elapsed = end - start;
    
    #ifdef TRACK_MEMORY
    string filename = "csp_memory_N" + to_string(n) + ".txt";
    MemoryTracker::generateReport(filename);
    MemoryTracker::analyzeFragmentation();
    #endif
    
    cspArena.reset();
    
    if (!solved) {
        cerr << "CSP solver failed for N = " << n << endl;
    }
    
    return elapsed.count();
}

int main() {
    vector <int> TstValues = { 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
    
    #ifdef TRACK_MEMORY
    cout << "Memory tracking ENABLED for CSP\n";
    #endif
    
    ofstream csv("nqueens_csp_results.csv");
    csv << "N,Time(seconds)\n";
    cout << "DFS - CSP searching...\n";
    
    for (int n : TstValues) {
        cout << "Running for N = " << n << "...\n";
        double time_taken = dfs_csp(n);
        cout << "Time taken: " << time_taken << " seconds\n";
        csv << n << "," << time_taken << "\n";
    }
    
    csv.close();
    
    #ifdef TRACK_MEMORY
    MemoryTracker::generateLeakReport("csp_final_leaks.txt");
    #endif
    
    cout << "Results saved to nqueens_csp_results.csv\n";
    return 0;
}