#include <iostream>
#include <verilated.h>
#include "Vdata_mem.h"

// Helper to step the clock
void tick(Vdata_mem* top) {
    top->clock = 0; top->eval();
    top->clock = 1; top->eval(); // Write happens on Rising Edge
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vdata_mem* ram = new Vdata_mem;

    std::cout << "[TEST] Starting Data RAM Verification...\n";

    // ==========================================
    // TEST 1: BASIC READ/WRITE
    // ==========================================
    // Write 0xDEADBEEF to Address 0x100
    ram->ramAxiWriteValid = 1;
    ram->ramAxiWriteAddress = 0x00000100;
    ram->ramAxiWriteData    = 0xDEADBEEF;
    
    tick(ram); // Trigger Write

    // Disable Write
    ram->ramAxiWriteValid = 0;
    
    // Read Address 0x100
    ram->ramAxiReadAddress = 0x00000100;
    ram->eval(); // Update combinational read logic

    if (ram->ramAxiReadData == 0xDEADBEEF) {
        std::cout << "[PASS] Basic Read/Write Verified.\n";
    } else {
        std::cout << "[FAIL] Write Failed. Expected DEADBEEF, Got: " << std::hex << ram->ramAxiReadData << "\n";
        return 1;
    }

    // ==========================================
    // TEST 2: WORD ALIGNMENT (CRITICAL)
    // ==========================================
    // The CPU uses Byte Addressing (0, 4, 8, C).
    // The RAM uses Word Indexing (0, 1, 2, 3).
    // Logic: Address 0x04 should map to Index 1.
    //        Address 0x05, 0x06, 0x07 should ALSO map to Index 1 (Byte offsets).
    
    ram->ramAxiWriteValid = 1;
    ram->ramAxiWriteAddress = 0x00000004; // Address 4
    ram->ramAxiWriteData    = 0xCAFEBABE;
    tick(ram);

    // Now Read back using a misaligned address (0x00000006)
    // If your logic [11:2] works, this should still read index 1.
    ram->ramAxiWriteValid = 0;
    ram->ramAxiReadAddress = 0x00000006; 
    ram->eval();

    if (ram->ramAxiReadData == 0xCAFEBABE) {
        std::cout << "[PASS] Word Alignment Verified (Address 0x4 == 0x6).\n";
    } else {
        std::cout << "[FAIL] Word Alignment Failed! 0x4 and 0x6 treated as different words.\n";
        return 1;
    }

    // ==========================================
    // TEST 3: WRITE PROTECTION
    // ==========================================
    // Assert Data and Address, but keep Valid = 0.
    // Memory should NOT change.
    
    // 1. Write Initial Value (0x11111111) at Address 0x200
    ram->ramAxiWriteValid = 1;
    ram->ramAxiWriteAddress = 0x00000200;
    ram->ramAxiWriteData    = 0x11111111;
    tick(ram);

    // 2. Try to Overwrite with 0xFFFFFFFF but Valid = 0
    ram->ramAxiWriteValid = 0; // DISABLE WRITE
    ram->ramAxiWriteData    = 0xFFFFFFFF;
    tick(ram);

    // 3. Read Back
    ram->ramAxiReadAddress = 0x00000200;
    ram->eval();

    if (ram->ramAxiReadData == 0x11111111) {
        std::cout << "[PASS] Write Protection Verified (Data preserved when Valid=0).\n";
    } else {
        std::cout << "[FAIL] Write Protection Failed! RAM was overwritten without Valid signal.\n";
        return 1;
    }

    // ==========================================
    // TEST 4: ADDRESS BOUNDARIES
    // ==========================================
    // Test Index 0 and Index 1 to ensure they don't overlap
    ram->ramAxiWriteValid = 1;
    
    // Write 0xAAAA at Address 0
    ram->ramAxiWriteAddress = 0x00000000;
    ram->ramAxiWriteData    = 0xAAAAAAAA;
    tick(ram);

    // Write 0xBBBB at Address 4
    ram->ramAxiWriteAddress = 0x00000004;
    ram->ramAxiWriteData    = 0xBBBBBBBB;
    tick(ram);

    // Read 0
    ram->ramAxiReadAddress = 0x00000000;
    ram->eval();
    bool val0_ok = (ram->ramAxiReadData == 0xAAAAAAAA);

    // Read 4
    ram->ramAxiReadAddress = 0x00000004;
    ram->eval();
    bool val4_ok = (ram->ramAxiReadData == 0xBBBBBBBB);

    if (val0_ok && val4_ok) {
        std::cout << "[PASS] Address Space Isolation Verified (Addr 0 and 4 are distinct).\n";
    } else {
        std::cout << "[FAIL] Address Overlap Detected!\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Data RAM Verified.\n";

    delete ram;
    return 0;
}