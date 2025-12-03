//
// harness/main.cpp - Timing Harness for LRU Hash Table Analysis
//

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <map>

#include "Operation.h"
#include "RunResults.h"
#include "../HashTableDictionary.hpp"  // Adjust path as needed

// ============================================================================
// Helper: Map N to table size M (from Section 4.4)
// ============================================================================
// This mapping should come from the provided implementation's main.cpp
// You'll need to extract this from the HashTableDictionary project
std::map<int, int> N_to_M_mapping = {
    {1024,    1279},
    {2048,    2551},
    {4096,    5101},
    {8192,    10273},
    {16384,   20479},
    {32768,   40849},
    {65536,   81931},
    {131072,  163861},
    {262144,  327739},
    {524288,  655243},
    {1048576, 1310809}
};

int get_table_size_for_N(int N) {
    auto it = N_to_M_mapping.find(N);
    if (it != N_to_M_mapping.end()) {
        return it->second;
    }
    std::cerr << "ERROR: No table size mapping found for N=" << N << "\n";
    std::exit(1);
}

// ============================================================================
// Timing function - runs hash table operations and measures time
// ============================================================================
template<typename HashTable>
RunResult run_trace_ops(HashTable &table,
                        RunResult &runResult,
                        const std::vector<Operation> &ops) {

    // Count operations for sanity check
    for (const auto &op: ops) {
        if (op.isInsert()) {
            ++runResult.inserts;
        } else if (op.isErase()) {
            ++runResult.erases;
        }
    }

    std::cout << "  Operations breakdown: " << runResult.inserts
              << " inserts, " << runResult.erases << " erases\n";

    // ========================================================================
    // One untimed warm-up run
    // ========================================================================
    table.clear();
    std::cout << "  Starting warm-up run for N = " << runResult.run_meta_data.N << std::endl;

    for (const auto &op: ops) {
        if (op.isInsert()) {
            table.insert(op.key);
        } else if (op.isErase()) {
            table.remove(op.key);
        }
    }

    // ========================================================================
    // Seven timed runs - report median
    // ========================================================================
    using clock = std::chrono::steady_clock;
    const int numTrials = 7;
    std::vector<std::int64_t> trials_ns;
    trials_ns.reserve(numTrials);

    for (int i = 0; i < numTrials; ++i) {
        table.clear();
        std::cout << "  Timed run " << (i + 1) << "/" << numTrials
                  << " for N = " << runResult.run_meta_data.N << std::endl;

        auto t0 = clock::now();

        for (const auto &op: ops) {
            if (op.isInsert()) {
                table.insert(op.key);
            } else if (op.isErase()) {
                table.remove(op.key);
            }
        }

        auto t1 = clock::now();
        trials_ns.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    // Find median
    const size_t mid = trials_ns.size() / 2;
    std::nth_element(trials_ns.begin(), trials_ns.begin() + mid, trials_ns.end());
    runResult.elapsed_ns = trials_ns[mid];

    // ========================================================================
    // Capture hash table statistics
    // ========================================================================
    // After the final timed run, the table is still populated
    // Get its statistics via csvStats()
    runResult.hash_table_stats_csv = table.csvStats();

    std::cout << "  Median elapsed time: " << runResult.elapsed_ms() << " ms\n";

    return runResult;
}

// ============================================================================
// Load trace file - adapted for LRU format (I key, E key)
// ============================================================================
bool load_trace_strict_header(const std::string &path,
                              RunMetaData &runMeta,
                              std::vector<Operation> &out_operations) {
    out_operations.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "ERROR: Cannot open trace file: " << path << "\n";
        return false;
    }

    // ========================================================================
    // Read FIRST line as header: <profile> <N> <seed>
    // Example: lru_profile 1024 23
    // ========================================================================
    std::string header;
    if (!std::getline(in, header)) {
        std::cerr << "ERROR: Empty trace file\n";
        return false;
    }

    const auto first = header.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || header[first] == '#') {
        std::cerr << "ERROR: Invalid header line\n";
        return false;
    }

    std::istringstream hdr(header);
    std::string profile;
    int N = 0;
    int seed = 0;

    if (!(hdr >> profile >> N >> seed)) {
        std::cerr << "ERROR: Cannot parse header: " << header << "\n";
        return false;
    }

    runMeta.profile = profile;
    runMeta.N = N;
    runMeta.seed = seed;

    std::cout << "Loaded trace: " << profile << " N=" << N << " seed=" << seed << "\n";

    // ========================================================================
    // Read operation lines: I key  or  E key
    // ========================================================================
    std::string line;
    int line_num = 1;  // header is line 1

    while (std::getline(in, line)) {
        ++line_num;

        // Skip blank lines and comments
        const auto opCodeIdx = line.find_first_not_of(" \t\r\n");
        if (opCodeIdx == std::string::npos || line[opCodeIdx] == '#') {
            continue;
        }

        std::istringstream iss(line.substr(opCodeIdx));
        std::string opcode_str;
        if (!(iss >> opcode_str)) {
            continue;  // empty line
        }

        // Parse opcodes
        // IMPORTANT: Keys consist of TWO words separated by space
        // Format: I word1 word2  or  E word1 word2
        if (opcode_str == "I") {
            std::string w1, w2;
            if (!(iss >> w1 >> w2)) {
                std::cerr << "ERROR: Line " << line_num << ": Insert missing key (needs two words)\n";
                return false;
            }
            // Combine into single key with space: "word1 word2"
            std::string key = w1 + " " + w2;
            out_operations.emplace_back(OpCode::Insert, key);

        } else if (opcode_str == "E") {
            std::string w1, w2;
            if (!(iss >> w1 >> w2)) {
                std::cerr << "ERROR: Line " << line_num << ": Erase missing key (needs two words)\n";
                return false;
            }
            // Combine into single key with space: "word1 word2"
            std::string key = w1 + " " + w2;
            out_operations.emplace_back(OpCode::Erase, key);

        } else {
            std::cerr << "ERROR: Line " << line_num << ": Unknown opcode '"
                      << opcode_str << "'\n";
            return false;
        }
    }

    std::cout << "  Loaded " << out_operations.size() << " operations\n";
    return true;
}

// ============================================================================
// Find trace files matching the profile prefix
// ============================================================================
void find_trace_files_or_die(const std::string &dir,
                             const std::string &profile_prefix,
                             std::vector<std::string> &out_files) {
    namespace fs = std::filesystem;
    out_files.clear();

    std::error_code ec;
    fs::path p(dir);

    if (!fs::exists(p, ec)) {
        std::cerr << "Error: directory '" << dir << "' does not exist";
        if (ec) std::cerr << " (" << ec.message() << ")";
        std::cerr << "\n";
        std::exit(1);
    }
    if (!fs::is_directory(p, ec)) {
        std::cerr << "Error: path '" << dir << "' is not a directory";
        if (ec) std::cerr << " (" << ec.message() << ")";
        std::cerr << "\n";
        std::exit(1);
    }

    fs::directory_iterator it(p, ec);
    if (ec) {
        std::cerr << "Error: cannot iterate directory '" << dir << "': "
                << ec.message() << "\n";
        std::exit(1);
    }

    const std::string suffix = ".trace";
    for (const auto &entry: it) {
        if (!entry.is_regular_file(ec)) {
            if (ec) {
                std::cerr << "Error: is_regular_file failed for '"
                        << entry.path().string() << "': " << ec.message() << "\n";
                std::exit(1);
            }
            continue;
        }

        const std::string name = entry.path().filename().string();
        const bool has_prefix = (name.rfind(profile_prefix, 0) == 0);
        const bool has_suffix = name.size() >= suffix.size() &&
                                name.compare(name.size() - suffix.size(),
                                             suffix.size(), suffix) == 0;

        if (has_prefix && has_suffix) {
            out_files.push_back(entry.path().string());
        }
    }

    std::sort(out_files.begin(), out_files.end());
}

// ============================================================================
// Main
// ============================================================================
int main() {
    const auto profileName = std::string("lru_profile");
    const auto traceDir = std::string("../traceFiles");  // Adjust path as needed

    std::vector<std::string> traceFiles;
    find_trace_files_or_die(traceDir, profileName, traceFiles);

    if (traceFiles.empty()) {
        std::cerr << "ERROR: No trace files found in " << traceDir << "\n";
        return 1;
    }

    std::cout << "Found " << traceFiles.size() << " trace files\n";

    std::vector<RunResult> runResults;

    // ========================================================================
    // Process each trace file
    // ========================================================================
    for (const auto& traceFile : traceFiles) {
        const auto pos = traceFile.find_last_of("/\\");
        auto traceFileBaseName = (pos == std::string::npos) ? traceFile : traceFile.substr(pos + 1);

        std::cout << "\n========================================\n";
        std::cout << "Processing: " << traceFileBaseName << "\n";
        std::cout << "========================================\n";

        // Load trace
        std::vector<Operation> operations;
        RunMetaData run_meta_data;
        if (!load_trace_strict_header(traceFile, run_meta_data, operations)) {
            std::cerr << "ERROR: Failed to load trace: " << traceFile << "\n";
            continue;
        }

        // Get table size for this N
        int table_size = get_table_size_for_N(run_meta_data.N);
        std::cout << "  Table size M for N=" << run_meta_data.N << ": " << table_size << "\n";

        // ====================================================================
        // Run 1: Single probing with compaction
        // ====================================================================
        std::cout << "\n--- Single Probing (compaction ON) ---\n";
        {
            RunResult result_single(run_meta_data);
            result_single.impl = "hash_map_single";
            result_single.trace_path = traceFileBaseName;

            HashTableDictionary table_single(
                table_size,
                HashTableDictionary::SINGLE,  // Single probing
                true,                          // Compaction ON
                0.95                           // Default compaction trigger
            );

            run_trace_ops(table_single, result_single, operations);
            runResults.push_back(result_single);
        }

        // ====================================================================
        // Run 2: Double probing with compaction
        // ====================================================================
        std::cout << "\n--- Double Probing (compaction ON) ---\n";
        {
            RunResult result_double(run_meta_data);
            result_double.impl = "hash_map_double";
            result_double.trace_path = traceFileBaseName;

            HashTableDictionary table_double(
                table_size,
                HashTableDictionary::DOUBLE,  // Double probing
                true,                          // Compaction ON
                0.95                           // Default compaction trigger
            );

            run_trace_ops(table_double, result_double, operations);
            runResults.push_back(result_double);
        }
    }

    // ========================================================================
    // Output results
    // ========================================================================
    if (runResults.empty()) {
        std::cerr << "ERROR: No results to report\n";
        return 1;
    }

    std::cout << "\n========================================\n";
    std::cout << "Writing results\n";
    std::cout << "========================================\n";

    // Print to stdout
    std::cout << "\n" << RunResult::csv_header() << std::endl;
    for (const auto& run : runResults) {
        std::cout << run.to_csv_row() << std::endl;
    }

    // Write to CSV file
    namespace fs = std::filesystem;
    const fs::path csvDir = "../csvs";
    const fs::path csvPath = csvDir / "lru_profile.csv";

    // Create directory if needed
    if (!fs::exists(csvDir)) {
        fs::create_directories(csvDir);
    }

    const bool needHeader = !fs::exists(csvPath) || fs::file_size(csvPath) == 0;

    std::ofstream csv(csvPath, std::ios::app);
    if (!csv) {
        std::cerr << "ERROR: cannot open " << csvPath << " for writing\n";
        return 1;
    }

    if (needHeader) {
        csv << RunResult::csv_header() << '\n';
    }
    for (const auto& run : runResults) {
        csv << run.to_csv_row() << '\n';
    }
    csv.flush();

    std::cout << "\nResults written to: " << csvPath << "\n";

    return 0;
}