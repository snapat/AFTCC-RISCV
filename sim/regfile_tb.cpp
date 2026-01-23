#include <iostream>
#include <verilated.h>
#include "Vregfile.h"

// Helper to toggle clock
void tick(Vregfile* top) {
    top->clock = 0; top->eval();
    top->clock = 1; top->eval(); // Write happens on Rising Edge
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vregfile* rf = new Vregfile;

    std::cout << "[TEST] Starting Register File Verification...\n";

    // ==========================================
    // TEST 1: THE x0 IMMUTABILITY CHECK (CRITICAL)
    // ==========================================
    // Scenario: Try to force write 0xFFFFFFFF into Register x0.
    // If x0 changes, the CPU is broken.
    
    rf->registerWriteEnable = 1;
    rf->writeAddress = 0; // x0
    rf->writeData = 0xFFFFFFFF;
    
    tick(rf); // Attempt write

    // Check Port 0
    rf->readAddress0 = 0;
    rf->eval();

    if (rf->readData0 == 0) {
        std::cout << "[PASS] Register x0 Immutability: Write ignored, read returns 0.\n";
    } else {
        std::cout << "[FAIL] FATAL ERROR: x0 was overwritten! Got: " << std::hex << rf->readData0 << "\n";
        return 1;
    }

    // ==========================================
    // TEST 2: BASIC READ/WRITE (x1)
    // ==========================================
    // Write 0xDEADBEEF to x1
    
    rf->registerWriteEnable = 1;
    rf->writeAddress = 1;
    rf->writeData = 0xDEADBEEF;
    
    tick(rf); // Write happens

    // Read back on Port 0
    rf->readAddress0 = 1;
    rf->eval();

    if (rf->readData0 == 0xDEADBEEF) {
        std::cout << "[PASS] Basic Read/Write: x1 updated correctly.\n";
    } else {
        std::cout << "[FAIL] x1 Write Failed. Got: " << std::hex << rf->readData0 << "\n";
        return 1;
    }

    // ==========================================
    // TEST 3: DUAL PORT READ
    // ==========================================
    // Setup: x1 = DEADBEEF (from prev test)
    // Action: Write 0xCAFEBABE to x2
    // Check: Read x1 on Port 0, Read x2 on Port 1 simultaneously
    
    rf->writeAddress = 2;
    rf->writeData = 0xCAFEBABE;
    tick(rf);

    rf->readAddress0 = 1; // Expect DEADBEEF
    rf->readAddress1 = 2; // Expect CAFEBABE
    rf->eval();

    if ((rf->readData0 == 0xDEADBEEF) && (rf->readData1 == 0xCAFEBABE)) {
        std::cout << "[PASS] Dual Port Read: Simultaneously read x1 and x2.\n";
    } else {
        std::cout << "[FAIL] Dual Port Read Failed.\n";
        return 1;
    }

    // ==========================================
    // TEST 4: WRITE ENABLE PROTECTION
    // ==========================================
    // Action: Try to write 0xBADF00D to x2 with Enable = 0.
    // Expect: x2 remains 0xCAFEBABE.
    
    rf->registerWriteEnable = 0; // DISABLE WRITES
    rf->writeAddress = 2;
    rf->writeData = 0xBADF00D;
    
    tick(rf);

    rf->readAddress1 = 2;
    rf->eval();

    if (rf->readData1 == 0xCAFEBABE) {
        std::cout << "[PASS] Write Enable Protection: Data preserved when Enable=0.\n";
    } else {
        std::cout << "[FAIL] Protection Failed! x2 overwritten when disabled.\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Register File Verified.\n";

    delete rf;
    return 0;
}