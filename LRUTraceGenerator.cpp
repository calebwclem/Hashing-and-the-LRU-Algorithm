/**
 * LRU Trace Generator
 *
 * Generates LRU access pattern traces for hash table experiments.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <sys/stat.h>

// ============================================================================
// Configuration
// ============================================================================
constexpr int DEFAULT_SEED = 23;
const std::string DEFAULT_CORPUS = "20980712_uniq_words.txt";
const std::string OUTPUT_DIR = "traceFiles";

// N values to generate traces for (2^10 through 2^20)
const std::vector<std::size_t> N_VALUES = {
    1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576
};

// ============================================================================
// Corpus Loading
// ============================================================================

bool loadCorpusWords(const std::string& corpusPath,
                     std::size_t count,
                     std::vector<std::string>& words) {
    words.clear();
    words.reserve(count);

    std::ifstream in(corpusPath);
    if (!in.is_open()) {
        std::cerr << "Cannot open corpus file: " << corpusPath << std::endl;
        return false;
    }

    std::string line;
    while (words.size() < count && std::getline(in, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        // Each line is a complete key (two words)
        words.push_back(line);
    }

    if (words.size() < count) {
        std::cerr << "Warning: Corpus has only " << words.size()
                  << " entries, needed " << count << std::endl;
        return false;
    }

    return true;
}

// ============================================================================
// Access Stream Builder
// ============================================================================

std::vector<std::string> buildAccessStream(const std::vector<std::string>& corpus,
                                           std::size_t N,
                                           unsigned int seed) {
    std::vector<std::string> stream;
    stream.reserve(12 * N);

    // Pool 1: first N words, each appearing once
    for (std::size_t i = 0; i < N; ++i) {
        stream.push_back(corpus[i]);
    }

    // Pool 2: next N words (indices N to 2N-1), each appearing 5 times
    for (std::size_t i = N; i < 2 * N; ++i) {
        for (int j = 0; j < 5; ++j) {
            stream.push_back(corpus[i]);
        }
    }

    // Pool 3: next 2N words (indices 2N to 4N-1), each appearing 3 times
    for (std::size_t i = 2 * N; i < 4 * N; ++i) {
        for (int j = 0; j < 3; ++j) {
            stream.push_back(corpus[i]);
        }
    }

    // Shuffle deterministically
    std::mt19937 rng(seed);
    std::shuffle(stream.begin(), stream.end(), rng);

    return stream;
}

// ============================================================================
// LRU Trace Generator
// ============================================================================

void generateLRUTrace(const std::vector<std::string>& accessStream,
                      std::size_t capacity,
                      std::ostream& out,
                      const std::string& profile,
                      int seed) {

    // Write header
    out << profile << " " << capacity << " " << seed << "\n";

    // LRU data structures
    std::list<std::string> lruList;  // front = MRU, back = LRU
    std::unordered_map<std::string, std::list<std::string>::iterator> residentMap;

    for (const auto& key : accessStream) {
        auto it = residentMap.find(key);

        if (it != residentMap.end()) {
            // HIT: key is already resident
            // Move to MRU position (front of list)
            lruList.erase(it->second);
            lruList.push_front(key);
            it->second = lruList.begin();

            // Emit insert (access) operation
            out << "I " << key << "\n";

        } else if (residentMap.size() < capacity) {
            // MISS, but space available
            // Insert at MRU position
            lruList.push_front(key);
            residentMap[key] = lruList.begin();

            // Emit insert operation
            out << "I " << key << "\n";

        } else {
            // MISS, table is full - need to evict LRU
            // Get the LRU key (back of list)
            const std::string& victim = lruList.back();

            // Emit evict operation
            out << "E " << victim << "\n";

            // Remove victim from map and list
            residentMap.erase(victim);
            lruList.pop_back();

            // Insert new key at MRU position
            lruList.push_front(key);
            residentMap[key] = lruList.begin();

            // Emit insert operation
            out << "I " << key << "\n";
        }
    }
}

// ============================================================================
// Main Program
// ============================================================================

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <corpus_file> [seed] [output_dir]" << std::endl;
    std::cerr << "       " << progName << " <corpus_file> <N> [seed] [output_file]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  Generate all traces (N=1024 to N=1048576):" << std::endl;
    std::cerr << "    " << progName << " 20980712_uniq_words.txt" << std::endl;
    std::cerr << "    " << progName << " 20980712_uniq_words.txt 23 traceFiles" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  Generate single trace:" << std::endl;
    std::cerr << "    " << progName << " 20980712_uniq_words.txt 1024" << std::endl;
    std::cerr << "    " << progName << " 20980712_uniq_words.txt 1024 23 my_trace.trace" << std::endl;
}

bool isValidN(std::size_t N) {
    for (auto validN : N_VALUES) {
        if (N == validN) return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 5) {
        printUsage(argv[0]);
        return 1;
    }

    std::string corpusPath = argv[1];

    // Check if second argument looks like an N value
    bool singleMode = false;
    std::size_t singleN = 0;

    if (argc >= 3) {
        try {
            singleN = std::stoull(argv[2]);
            if (isValidN(singleN)) {
                singleMode = true;
            }
        } catch (...) {
            // Not a valid N, treat as seed
        }
    }

    if (singleMode) {
        // Single trace generation mode
        int seed = (argc >= 4) ? std::stoi(argv[3]) : DEFAULT_SEED;
        std::string outputFile = (argc >= 5) ? argv[4] :
            "lru_profile_N_" + std::to_string(singleN) + "_S_" + std::to_string(seed) + ".trace";

        // Load corpus (need 4N words)
        std::vector<std::string> corpus;
        if (!loadCorpusWords(corpusPath, 4 * singleN, corpus)) {
            return 1;
        }

        // Build access stream
        std::cout << "Building access stream for N = " << singleN << "..." << std::endl;
        auto accessStream = buildAccessStream(corpus, singleN, seed);
        std::cout << "  Total accesses: " << accessStream.size() << std::endl;

        // Generate trace
        std::ofstream outFile(outputFile);
        if (!outFile.is_open()) {
            std::cerr << "Cannot create output file: " << outputFile << std::endl;
            return 1;
        }

        std::cout << "Generating trace..." << std::endl;
        generateLRUTrace(accessStream, singleN, outFile, "lru_profile", seed);
        outFile.close();

        std::cout << "Trace written to: " << outputFile << std::endl;

    } else {
        // Batch trace generation mode
        int seed = (argc >= 3) ? std::stoi(argv[2]) : DEFAULT_SEED;
        std::string outputDir = (argc >= 4) ? argv[3] : OUTPUT_DIR;

        // Create output directory if needed
        struct stat st;
        if (stat(outputDir.c_str(), &st) != 0) {
            mkdir(outputDir.c_str(), 0755);
        }

        // Find largest N to determine corpus size needed
        std::size_t maxN = *std::max_element(N_VALUES.begin(), N_VALUES.end());

        // Load corpus (need 4 * maxN words)
        std::cout << "Loading corpus..." << std::endl;
        std::vector<std::string> corpus;
        if (!loadCorpusWords(corpusPath, 4 * maxN, corpus)) {
            return 1;
        }
        std::cout << "Loaded " << corpus.size() << " words." << std::endl;

        // Generate traces for each N
        for (std::size_t N : N_VALUES) {
            std::string filename = "lru_profile_N_" + std::to_string(N) +
                                   "_S_" + std::to_string(seed) + ".trace";
            std::string filepath = outputDir + "/" + filename;

            std::cout << "\nGenerating trace for N = " << N << "..." << std::endl;

            // Build access stream
            auto accessStream = buildAccessStream(corpus, N, seed);
            std::cout << "  Access stream size: " << accessStream.size() << std::endl;

            // Generate trace
            std::ofstream outFile(filepath);
            if (!outFile.is_open()) {
                std::cerr << "Cannot create: " << filepath << std::endl;
                continue;
            }

            generateLRUTrace(accessStream, N, outFile, "lru_profile", seed);
            outFile.close();

            std::cout << "  Written: " << filename << std::endl;
        }

        std::cout << "\nAll traces generated in: " << outputDir << std::endl;
    }

    return 0;
}
