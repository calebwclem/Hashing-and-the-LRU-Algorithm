cmake_minimum_required(VERSION 3.17)
project(HashTablesOpenAddressing)

set(CMAKE_CXX_STANDARD 17)

add_executable(HashTablesOpenAddressing main.cpp HashTableDictionary.cpp HashTableDictionary.hpp InvertedListDictionary.hpp InvertedListDictionary.cpp SmallIntMixedOperations.cpp SmallIntMixedOperations.hpp
        Operations.hpp)