# LRU Hash Table Analysis Project

# STUDENT INFORMATION
- NAME: Caleb Clements
- STUDENT ID: 008878539
- COURSE: CS315 - Data Structures
- ASSIGNMENT: Hash Tables and LRU Algorithm Analysis
- REPOSITORY LINK: https://github.com/calebwclem/Hashing-and-the-LRU-Algorithm

# COLLABORATION & SOURCES
I completed this assignment mostly individually. However, I discussed my ideas for analysis with Will as well as John. I used Claude AI (Anthropic) as a coding assistant for debugging, code structure suggestions, and understanding the assignment requirements.

Furthermore, I ran out of time spending so much time on my analysis and I used AI to build this README.

# PROJECT OVERVIEW
This project implements and analyzes open-addressed hash tables under a delete-heavy LRU (Least Recently Used) workload. The analysis compares single probing (linear) versus double hashing across multiple problem sizes (N = 1024 to 1,048,576) with compaction enabled.

## Components Implemented:
1. **Timing Harness** - Measures hash table performance on LRU traces
2. **LRU Trace Generator** - Creates deterministic LRU access patterns
3. **Analysis Report** - Comprehensive performance analysis with visualizations

# IMPLEMENTATION DETAILS

## 1. Timing Harness (`harness/`)
The harness processes LRU trace files and measures hash table performance:

### Key Features:
- Reads trace files with `I key` (insert) and `E key` (erase) operations
- Executes 1 warm-up run + 7 timed runs, reports median elapsed time
- Tests both single probing and double hashing on identical traces
- Generates CSV output with timing and structural metrics

### File Structure:
```
harness/
├── main.cpp           # Main timing harness
├── Operation.hpp      # Operation types (Insert, Erase)
├── RunResults.hpp     # Results data structure with CSV output
└── RunMetaData.hpp    # Trace metadata (profile, N, seed)
```

### Key Implementation Details:
- **N-to-M Mapping**: Uses provided prime table sizes for each capacity N
- **Two-word Keys**: Parses keys as two-word strings (e.g., "federal government")
- **Probe Types**: Tests both `HashTableDictionary::SINGLE` and `DOUBLE`
- **Compaction**: Always enabled with 0.95 effective load trigger
- **Output**: Combines harness timing with hash table's `csvStats()` output

## 2. LRU Trace Generator (`lru_generator.cpp`)
Generates deterministic LRU access patterns for hash table experiments.

### Algorithm:
1. **Corpus Selection** (4N unique words from corpus):
    - First N words: appear once each
    - Next N words: appear 5 times each
    - Next 2N words: appear 3 times each
    - Total: 12N operations per trace

2. **Deterministic Shuffle**: Uses `std::mt19937` with seed=23 for reproducibility

3. **LRU Simulation**:
    - Maintains doubly-linked list (MRU at front, LRU at back)
    - Uses `std::unordered_map` for O(1) lookups
    - On hit: moves key to MRU position, emits `I key`
    - On miss (space available): inserts at MRU, emits `I key`
    - On miss (full): evicts LRU, emits `E victim` then `I key`

### Data Structures:
```cpp
std::list<std::string> lruList;              // LRU ordering
std::unordered_map<std::string, 
    std::list<std::string>::iterator> residentMap;  // Fast lookup
```

### Output Format:
```
lru_profile 1024 23
I federal government
I supreme court
E federal government
I judicial review
...
```

## 3. Analysis Components

### Histogram Visualization:
- Used provided D3.js histogram app to analyze table structure
- Examined 0/1 binary maps showing ACTIVE+DELETED clusters
- Compared run-length distributions before/after compaction
- Generated for both single and double probing at multiple N values

### Metrics Analyzed:
- **Timing**: elapsed_ms, throughput (ops/ms), latency (ms/op)
- **Work**: average_probes, total_probes
- **Structure**: load_factor_pct, eff_load_factor_pct, tombstones_pct
- **Maintenance**: compactions count
- **Clustering**: run-length distributions from histograms

# How to compile and run my solution.

## Prerequisites
- C++17 or later compiler
- CMake 3.17 or later
- Provided hash table implementation (`HashTableDictionary.cpp/hpp`)
- Word corpus: `20980712_uniq_words.txt`

## Build Instructions

### Option 1: Using CMake (Recommended)
```bash
mkdir build
cd build
cmake ..
make
```

This creates three executables:
- `HashTablesOpenAddressing` - Standalone hash table tester
- `harness` - Timing harness for experiments
- `lru_generator` - Trace file generator (if added to CMakeLists.txt)

### Option 2: Manual Compilation
```bash
# Compile hash table library
g++ -std=c++17 -Wall -O3 -c HashTableDictionary.cpp

# Compile harness
g++ -std=c++17 -Wall -O3 harness/main.cpp HashTableDictionary.o -o harness

# Compile trace generator
g++ -std=c++17 -Wall -O3 lru_generator.cpp -o lru_generator
```

## Running the Programs

### 1. Generate Traces (All N Values)
```bash
./lru_generator 20980712_uniq_words.txt 23 traceFiles
```
This generates 11 trace files in `traceFiles/`:
- `lru_profile_N_1024_S_23.trace`
- `lru_profile_N_2048_S_23.trace`
- ... through N=1048576

### 2. Generate Single Trace
```bash
./lru_generator 20980712_uniq_words.txt 1024 23 my_trace.trace
```

### 3. Run Timing Harness
```bash
cd build
./harness
```
Output: `csvs/lru_profile.csv` with 22 rows (11 N values × 2 probe types)

### 4. Test Standalone Hash Table
```bash
./HashTablesOpenAddressing ../traceFiles/lru_profile_N_1024_S_23.trace
```
Outputs detailed statistics and structure maps to console.

## Visualization

### Timing Plots:
1. Open `hash_table_lru_d3_plotting_app.html` in browser
2. Load `csvs/lru_profile.csv`
3. Select metrics: elapsed_ms, average_probes, throughput, etc.

### Structure Histograms:
1. Run standalone to generate 0/1 maps (redirect output to file)
2. Extract binary map section
3. Open `hash_table_d3_histogram_app.html`
4. Load map file to visualize run-length distributions

# TESTING & STATUS

## Implementation Status
✅ Timing harness - fully functional
✅ LRU trace generator - fully functional
✅ Analysis report - complete with visualizations
✅ All 11 N values tested (1024 to 1,048,576)

## Key Findings
- **Single probing** faster than double probing across all N values
- Cache locality dominates probe count advantage
- Double probing: 2.5× fewer probes but 10-15% slower
- Compaction successfully breaks up clusters (3.3-3.4× more runs)
- Consistent +6 compaction offset (double triggers 6 more compactions)

## Verification
- Traces match provided format exactly
- CSV output includes all required columns
- Histogram visualizations show expected before/after patterns
- Timing results consistent across multiple runs

## Known Issues/Observations
- N=32768 shows anomalous 7% effective load gap (likely late compaction timing)
- Double probing consistently triggers exactly 6 more compactions (deterministic pattern)
- Single probing maintains advantage despite worse clustering (cache effects dominate)

# FILE STRUCTURE
```
HashMapAnalysis_StudentFiles/
├── CMakeLists.txt                          # Build configuration
├── README.md                               # This file
├── ANALYSIS_REPORT.md                      # Detailed analysis
├── HashTableDictionary.cpp/.hpp            # Provided implementation
├── Operations.hpp                          # Provided operation types
├── main.cpp                                # Standalone tester
├── lru_generator.cpp                       # LRU trace generator
├── harness/                                # Timing harness
│   ├── main.cpp
│   ├── Operation.hpp
│   ├── RunResults.hpp
│   └── RunMetaData.hpp
├── traceFiles/                             # Generated traces
│   └── lru_profile_N_*_S_23.trace
├── csvs/                                   # Timing results
│   └── lru_profile.csv
├── 20980712_uniq_words.txt                 # Word corpus
├── hash_table_lru_d3_plotting_app.html     # Visualization tools
└── hash_table_d3_histogram_app.html
```

# ANALYSIS SUMMARY

## Section 5.1: Timing Analysis
- Single probing maintains 10-15% performance advantage across all N
- Cache locality benefits outweigh probe count disadvantage
- Per-operation hashing overhead (2 hashes vs 1) costs ~3-4ms at N=1024
- No crossover point observed despite theoretical expectations

## Section 5.2: Structure Analysis
- Compaction increases runs 3.3-3.4× (172→561 single, 242→825 double)
- Mean run length drops 74-75% after compaction
- Double probing: 29% shorter runs before compaction (20.02 vs 28.17)
- Tail behavior: single's p95=122 vs double's p95=66 before compaction

## Key Insights
1. Implementation details matter more than asymptotic analysis
2. Cache effects dominate at practical table sizes (~40MB ≈ L3 cache)
3. String hashing cost is significant constant factor
4. Compaction successfully prevents performance degradation

# REFERENCES
- Assignment specification (Sections 1-7)
- Provided HashTableDictionary implementation
- D3.js visualization applications
- CS315 course materials