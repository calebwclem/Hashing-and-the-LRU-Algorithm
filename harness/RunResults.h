#pragma once
#include <string>
#include <cstdint>
#include <sstream>

#include "RunMetaData.h"

struct RunResult {
    // identifiers
    std::string impl;         // "hash_map_single" or "hash_map_double"
    std::string trace_path;   // e.g., "lru_profile_N_1024_S_23.trace"
    RunMetaData run_meta_data;

    RunResult(const RunMetaData& meta_data): run_meta_data(meta_data) {}

    // timing
    std::int64_t elapsed_ns = 0;   // total replay time (nanoseconds)

    // operation counts (for LRU hash table)
    long inserts = 0;  // 'I'
    long erases  = 0;  // 'E'

    // Hash table statistics (will be populated from HashTableDictionary::csvStats())
    std::string hash_table_stats_csv = "";

    // convenience
    long total_ops() const {
        return inserts + erases;
    }
    double elapsed_ms() const {
        return static_cast<double>(elapsed_ns) / 1e6;
    }
    double ops_per_sec() const {
        const double secs = static_cast<double>(elapsed_ns) / 1e9;
        return secs > 0.0 ? static_cast<double>(total_ops()) / secs : 0.0;
    }

    // CSV helpers
    static std::string csv_header() {
        // From Section 4.5: impl,profile,trace_path,N,seed,elapsed_ms,ops_total,inserts,erases,
        // followed by hash table's csvStatsHeader()
        // The hash table provides its own header, so we'll build our prefix
        return "impl,profile,trace_path,N,seed,elapsed_ms,ops_total,inserts,erases,"
               // Hash table adds: table_size,active,available,tombstones,total_probes,inserts,deletes,
               //                  lookups,full_scans,compactions,max_in_table,available_pct,
               //                  load_factor_pct,eff_load_factor_pct,tombstones_pct,average_probes,
               //                  probe_type,compaction_state
               "table_size,active,available,tombstones,total_probes,table_inserts,table_deletes,"
               "lookups,full_scans,compactions,max_in_table,available_pct,"
               "load_factor_pct,eff_load_factor_pct,tombstones_pct,average_probes,"
               "probe_type,compaction_state";
    }

    std::string to_csv_row() const {
        std::ostringstream os;
        // Our timing and operation data
        os << impl << ','
           << run_meta_data.profile << ','
           << trace_path << ','
           << run_meta_data.N << ','
           << run_meta_data.seed << ','
           << elapsed_ms() << ','
           << total_ops() << ','
           << inserts << ','
           << erases;

        // Append hash table statistics if available
        if (!hash_table_stats_csv.empty()) {
            os << ',' << hash_table_stats_csv;
        }

        return os.str();
    }
};