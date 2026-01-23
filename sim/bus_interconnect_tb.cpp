#include <iostream>
#include <verilated.h>
#include "Vbus_interconnect.h"

// --- MEMORY MAP CONSTANTS ---
// Based on your RTL logic:
// Bit 30 = 1 -> IO  (0x40000000)
// Bit 29 = 1 -> RAM (0x20000000)
// Else       -> ROM (0x00000000)
const uint32_t ADDR_ROM = 0x00001000;
const uint32_t ADDR_RAM = 0x20000004;
const uint32_t ADDR_IO  = 0x40000008;

// Helper to toggle clock
void tick(Vbus_interconnect* top) {
    top->clock = 1; top->eval();
    top->clock = 0; top->eval();
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vbus_interconnect* bus = new Vbus_interconnect;

    std::cout << "[TEST] Starting Bus Interconnect Verification...\n";

    // --- TEST 1: RESET & DEFAULT STATE ---
    bus->resetActiveLow = 0;
    tick(bus);
    bus->resetActiveLow = 1;
    
    // Default: CPU should be master
    // Check if CPU write to RAM works
    bus->cpuAxiWriteAddress = ADDR_RAM;
    bus->cpuAxiWriteValid = 1;
    bus->cpuAxiWriteData = 0xDEADBEEF;
    bus->dmaAxiWriteValid = 0; // DMA Idle
    bus->eval();

    if (bus->ramAxiWriteValid == 1 && bus->ramAxiWriteData == 0xDEADBEEF) {
        std::cout << "[PASS] Test 1: CPU Default Master Access to RAM.\n";
    } else {
        std::cout << "[FAIL] Test 1: CPU failed to access RAM.\n";
        return 1;
    }

    // --- TEST 2: ADDRESS DECODING (ROUTING) ---
    // Try accessing IO (0x40...)
    bus->cpuAxiWriteAddress = ADDR_IO;
    bus->eval();

    if (bus->ioAxiWriteValid == 1 && bus->ramAxiWriteValid == 0) {
        std::cout << "[PASS] Test 2: Address Decoding (IO Selected, RAM Ignored).\n";
    } else {
        std::cout << "[FAIL] Test 2: Address Decoding Failed.\n";
        return 1;
    }

    // --- TEST 3: DMA ARBITRATION (THE TAKEOVER) ---
    // Assert DMA Request
    bus->dmaAxiWriteAddress = ADDR_IO;
    bus->dmaAxiWriteData    = 0xCAFEBABE;
    bus->dmaAxiWriteValid   = 1; // DMA requests bus
    
    // CPU tries to conflict
    bus->cpuAxiWriteAddress = ADDR_RAM;
    bus->cpuAxiWriteData    = 0x11111111;
    bus->cpuAxiWriteValid   = 1;

    // Pulse Clock (Arbitration Logic needs a posedge to switch ActiveMasterReg)
    tick(bus); 

    if (bus->ioAxiWriteData == 0xCAFEBABE && bus->ioAxiWriteValid == 1) {
        std::cout << "[PASS] Test 3: DMA Successfully Preempted CPU.\n";
    } else {
        std::cout << "[FAIL] Test 3: DMA Arbitration Failed. CPU still driving bus?\n";
        std::cout << "  Expected: CAFEBABE, Got: " << std::hex << bus->ioAxiWriteData << "\n";
        return 1;
    }

    // --- TEST 4: DMA RELEASE ---
    bus->dmaAxiWriteValid = 0; // DMA done
    tick(bus); // One clock to release lock

    // Bus should return to CPU
    if (bus->ramAxiWriteData == 0x11111111) { // CPU data from Test 3
        std::cout << "[PASS] Test 4: Bus Control Returned to CPU.\n";
    } else {
        std::cout << "[FAIL] Test 4: Bus stuck on DMA or invalid state.\n";
        return 1;
    }

    // --- TEST 5: READ DATA ROUTING ---
    // Simulate RAM returning data to CPU
    bus->cpuAxiReadValid = 1;
    bus->cpuAxiReadAddress = ADDR_RAM;
    bus->ramAxiReadData = 0x99887766; // Data coming FROM RAM
    bus->dmaAxiReadValid = 0; // Ensure CPU is master
    tick(bus); // Latch state if needed, mostly comb logic though

    if (bus->cpuAxiReadData == 0x99887766) {
        std::cout << "[PASS] Test 5: RAM Read Data Routed to CPU correctly.\n";
    } else {
        std::cout << "[FAIL] Test 5: Read Data Routing Failed.\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Bus Interconnect Verified.\n";
    
    delete bus;
    return 0;
}