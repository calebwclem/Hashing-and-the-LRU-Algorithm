cmake_minimum_required(VERSION 3.17)
project(HashTablesOpenAddressing)

set(CMAKE_CXX_STANDARD 17)

# ============================================================================
# Hash Table Library
# ============================================================================
add_library(hash_table_lib
        HashTableDictionary.cpp
        HashTableDictionary.hpp
)

# ============================================================================
# Standalone Hash Table Test (original main.cpp)
# ============================================================================
add_executable(HashTablesOpenAddressing
        main.cpp
        Operations.hpp
        InvertedListDictionary.hpp
        InvertedListDictionary.cpp
        SmallIntMixedOperations.cpp
        SmallIntMixedOperations.hpp
)

target_link_libraries(HashTablesOpenAddressing hash_table_lib)

# ============================================================================
# Harness for Timing Experiments
# ============================================================================
add_executable(harness
        harness/main.cpp
        harness/Operation.h
        harness/RunMetaData.h
        harness/RunResults.h
)

target_link_libraries(harness hash_table_lib)
target_include_directories(harness PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})