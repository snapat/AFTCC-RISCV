

#include <iostream>
#include <cstdlib>     // For rand()
#include <verilated.h> // Core Verilator routine
#include "Valu.h"      // Generated header from your SystemVerilog

// --- THE GOLDEN MODEL ---
// This function mimics exactly what the hardware *should* do in C++.
// We use this to verify the hardware result.
uint32_t solve_golden(uint32_t a, uint32_t b, int op) {
    switch(op) {
        case 0: return a + b;       // 000: ADD
        case 1: return a - b;       // 001: SUB
        case 2: return a & b;       // 010: AND
        case 3: return a | b;       // 011: OR
        case 4: return a ^ b;       // 100: XOR
        case 5: return (a < b) ? 1 : 0; // 101: SLT
        default: return 0;
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    
    // Instantiate the ALU
    Valu* alu = new Valu;

    // Simulation Parameters
    const int NUM_TESTS = 100000; // Check 100k vectors
    int success_count = 0;

    std::cout << "[TEST] Starting ALU Verification (100,000 Vectors)...\n";

    for (int i = 0; i < NUM_TESTS; i++) {
        // 1. Generate Random Inputs
        // Use 32-bit random numbers (rand() is usually 15-bit, so we shift/mix)
        uint32_t a = (rand() << 16) | rand();
        uint32_t b = (rand() << 16) | rand();
        int op = rand() % 6; // Valid ops are 0-5

        // 2. Drive the Hardware Inputs
        alu->inputA = a;
        alu->inputB = b;
        alu->aluControl = op;
        
        // 3. Evaluate (Run one cycle of logic)
        alu->eval();

        // 4. Compute Expected Result (Golden Model)
        uint32_t expected_result = solve_golden(a, b, op);
        bool expected_zero = (expected_result == 0);

        // 5. Compare Hardware vs Software
        if ((alu->aluResult != expected_result) || (alu->zero != expected_zero)) {
            std::cout << "\n[FAIL] Mismatch Detected at Test #" << i << "\n";
            std::cout << "  OPCODE: " << op << "\n";
            std::cout << "  Input A: 0x" << std::hex << a << "\n";
            std::cout << "  Input B: 0x" << std::hex << b << "\n";
            std::cout << "  Expected: 0x" << expected_result << " (Zero: " << expected_zero << ")\n";
            std::cout << "  Actual:   0x" << alu->aluResult << " (Zero: " << (int)alu->zero << ")\n";
            
            delete alu;
            return 1; // Exit with error code
        }
    }

    // If we get here, we survived 100,000 tests
    std::cout << "[PASS] ALU Integrity Verified. 100,000 vectors matched.\n";
    
    delete alu;
    return 0;
}