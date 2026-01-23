#include <iostream>
#include <verilated.h>
#include "Vpc_reg.h"

// Helper to toggle clock
void tick(Vpc_reg* top) {
    top->clock = 0; top->eval();
    top->clock = 1; top->eval(); // Rising Edge -> Update happens here
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vpc_reg* pc = new Vpc_reg;

    std::cout << "[TEST] Starting Program Counter Verification...\n";

    // ==========================================
    // TEST 1: RESET BEHAVIOR
    // ==========================================
    // Hold Reset Low (Active)
    pc->resetActiveLow = 0;
    pc->nextProgramCounter = 0xDEADBEEF; // Junk data
    pc->enable = 1;
    
    // Clock it
    pc->clock = 0; pc->eval();
    pc->clock = 1; pc->eval();

    // Check: PC should be 0, ignoring the input
    if (pc->programCounter == 0) {
        std::cout << "[PASS] Reset Logic: PC cleared to 0.\n";
    } else {
        std::cout << "[FAIL] Reset Logic Failed. PC is: " << std::hex << pc->programCounter << "\n";
        return 1;
    }

    // Release Reset
    pc->resetActiveLow = 1;

    // ==========================================
    // TEST 2: NORMAL INCREMENT
    // ==========================================
    // Scenario: Normal instruction fetch (PC -> PC+4)
    pc->enable = 1;
    pc->nextProgramCounter = 0x00000004;
    
    tick(pc);

    if (pc->programCounter == 0x00000004) {
        std::cout << "[PASS] Normal Update: PC advanced to 0x4.\n";
    } else {
        std::cout << "[FAIL] Normal Update Failed. Expected 0x4, Got: " << pc->programCounter << "\n";
        return 1;
    }

    // ==========================================
    // TEST 3: STALL LOGIC (CRITICAL)
    // ==========================================
    // Scenario: A Hazard is detected! We set enable=0.
    // The NextPC input changes to 0x8, but the PC MUST stay at 0x4.
    
    pc->enable = 0; // FREEZE!
    pc->nextProgramCounter = 0x00000008;
    
    tick(pc);

    if (pc->programCounter == 0x00000004) {
        std::cout << "[PASS] Stall Logic: PC successfully frozen at 0x4 despite input change.\n";
    } else {
        std::cout << "[FAIL] Stall Logic Failed! PC updated when disabled. Got: " << pc->programCounter << "\n";
        return 1;
    }

    // ==========================================
    // TEST 4: RESUME
    // ==========================================
    // Scenario: Hazard cleared. Enable=1. PC should finally catch up.
    
    pc->enable = 1; // Unfreeze
    tick(pc);

    if (pc->programCounter == 0x00000008) {
        std::cout << "[PASS] Resume: PC updated to 0x8 after stall.\n";
    } else {
        std::cout << "[FAIL] Resume Failed.\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Program Counter Verified.\n";

    delete pc;
    return 0;
}