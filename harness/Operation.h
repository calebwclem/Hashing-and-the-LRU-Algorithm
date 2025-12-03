//
// Operation.hpp - Adapted for LRU Hash Table Analysis
//

#pragma once
#include <cassert>
#include <string>
#include <iostream>

enum class OpCode {
    Insert,     // I key
    Erase       // E key
};

struct Operation {
    OpCode tag;
    std::string key;   // word/token from the trace

    // Constructor for Insert: I key
    Operation(OpCode op_code, std::string k)
        : tag(op_code), key(std::move(k)) {
        // Both Insert and Erase take a key
    }

    void print() const {
        switch (tag) {
            case OpCode::Insert:
                std::cout << "I " << key << std::endl;
                break;
            case OpCode::Erase:
                std::cout << "E " << key << std::endl;
                break;
            default:
                std::cout << "Unknown operation" << std::endl;
        }
    }

    // Identify the operation type
    bool isInsert() const { return tag == OpCode::Insert; }
    bool isErase()  const { return tag == OpCode::Erase; }
};
