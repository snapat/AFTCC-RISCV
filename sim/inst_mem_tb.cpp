#include <iostream>
#include <fstream>
#include <verilated.h>
#include <sys/stat.h> // For creating directories
#include "Vinst_mem.h"

// --- HELPER: CREATE DUMMY FIRMWARE ---
// We create a file with known data so we can verify the ROM loaded it.
void create_dummy_firmware() {
    // 1. Create directory "firmware" (Linux/Mac command)
    system("mkdir -p firmware");

    // 2. Write hex file
    std::ofstream outfile("firmware/firmware.hex");
    
    // Write Index 0: 0xDEADBEEF (Instruction 1)
    outfile << "DEADBEEF\n";
    // Write Index 1: 0xCAFEBABE (Instruction 2)
    outfile << "CAFEBABE\n";
    // Write Index 2: 0x12345678 (Constant Data)
    outfile << "12345678\n";
    
    outfile.close();
    std::cout << "[SETUP] Created dummy firmware/firmware.hex\n";
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    // CRITICAL: Create the file BEFORE starting the module
    create_dummy_firmware();

    Vinst_mem* rom = new Vinst_mem;

    std::cout << "[TEST] Starting Instruction Memory Verification...\n";

    // ==========================================
    // TEST 1: INSTRUCTION FETCH (PORT A)
    // ==========================================
    // We expect Address 0 to hold DEADBEEF (from our dummy file)
    
    rom->romAxiReadAddress = 0x00000000;
    rom->eval(); // Combinational Read (No Clock needed for ROM usually)

    if (rom->romAxiReadData == 0xDEADBEEF) {
        std::cout << "[PASS] Port A (Instruction Fetch): Loaded 0xDEADBEEF correctly.\n";
    } else {
        std::cout << "[FAIL] Port A Failed. Expected DEADBEEF, Got: " << std::hex << rom->romAxiReadData << "\n";
        return 1;
    }

    // ==========================================
    // TEST 2: WORD ALIGNMENT
    // ==========================================
    // Address 0x04 should point to Index 1 (CAFEBABE)
    
    rom->romAxiReadAddress = 0x00000004;
    rom->eval();

    if (rom->romAxiReadData == 0xCAFEBABE) {
        std::cout << "[PASS] Address Alignment: Address 0x4 maps to Index 1.\n";
    } else {
        std::cout << "[FAIL] Address Alignment Failed. Expected CAFEBABE, Got: " << std::hex << rom->romAxiReadData << "\n";
        return 1;
    }

    // ==========================================
    // TEST 3: DUAL PORT ACCESS (SIMULTANEOUS READ)
    // ==========================================
    // Port A reads Index 0 (DEADBEEF)
    // Port B reads Index 2 (12345678)
    
    rom->romAxiReadAddress = 0x00000000;
    rom->busReadAddress    = 0x00000008; // Address 8 -> Index 2
    rom->eval();

    bool portA_ok = (rom->romAxiReadData == 0xDEADBEEF);
    bool portB_ok = (rom->busReadData    == 0x12345678);

    if (portA_ok && portB_ok) {
        std::cout << "[PASS] Dual Port Read: CPU and Bus read different addresses simultaneously.\n";
    } else {
        std::cout << "[FAIL] Dual Port Read Failed.\n";
        std::cout << "  Port A (Exp DEADBEEF): " << std::hex << rom->romAxiReadData << "\n";
        std::cout << "  Port B (Exp 12345678): " << std::hex << rom->busReadData << "\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Instruction Memory Verified.\n";

    delete rom;
    return 0;
}