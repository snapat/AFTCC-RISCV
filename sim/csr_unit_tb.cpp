#include <iostream>
#include <verilated.h>
#include "Vcsr_unit.h"

// Helper to step the clock
void tick(Vcsr_unit* top) {
    top->clock = 0; top->eval();
    top->clock = 1; top->eval(); // Latch on Rising Edge
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vcsr_unit* csr = new Vcsr_unit;

    std::cout << "[TEST] Starting CSR Unit (MEPC) Verification...\n";

    // ==========================================
    // TEST 1: RESET BEHAVIOR
    // ==========================================
    csr->resetActiveLow = 0; // Assert Reset
    csr->clock = 0;
    csr->eval();
    
    // Check asynchronous reset or synchronous reset behavior
    // (Your code uses posedge clock OR negedge reset, so it should be async-ish)
    if (csr->mepcValue == 0) {
        std::cout << "[PASS] Reset Logic: MEPC cleared to 0.\n";
    } else {
        std::cout << "[FAIL] Reset Logic: MEPC not 0.\n"; return 1;
    }

    // Release Reset
    csr->resetActiveLow = 1;
    tick(csr);

    // ==========================================
    // TEST 2: HARDWARE TRAP SAVE (Simulate Timer Interrupt)
    // ==========================================
    // Scenario: The CPU is at PC 0x1000 when an interrupt fires.
    // The hardware must save 0x1000 to MEPC automatically.
    
    csr->pcFromCore = 0x00001000;
    csr->csrWriteEnable = 1; // Hardware Signal
    csr->busWriteEnable = 0; // Software Idle
    csr->busWriteData = 0x00000000;
    
    tick(csr); // Clock edge happens

    if (csr->mepcValue == 0x00001000) {
        std::cout << "[PASS] Hardware Trap: PC saved to MEPC correctly.\n";
    } else {
        std::cout << "[FAIL] Hardware Trap Failed. Expected 0x1000, Got 0x" << std::hex << csr->mepcValue << "\n"; return 1;
    }

    // ==========================================
    // TEST 3: SOFTWARE CONTEXT SWITCH (Simulate Scheduler)
    // ==========================================
    // Scenario: The ISR (Interrupt Handler) decides to switch tasks.
    // It writes the address of Task B (0x2000) into MEPC via the bus.

    csr->csrWriteEnable = 0; // Hardware done
    csr->busWriteEnable = 1; // Software Request (Store to CSR)
    csr->busWriteData   = 0x00002000; // Address of Task B
    
    tick(csr);

    if (csr->mepcValue == 0x00002000) {
        std::cout << "[PASS] Context Switch: Software overwrote MEPC successfully.\n";
    } else {
        std::cout << "[FAIL] Context Switch Failed. Software write ignored.\n"; return 1;
    }

    // ==========================================
    // TEST 4: PRIORITY CONFLICT (The "Nanas" Test)
    // ==========================================
    // Scenario: A Hardware Trap happens EXACTLY when the Software tries to write.
    // Your Logic: "else if (busWriteEnable)" is checked BEFORE "else if (csrWriteEnable)".
    // Therefore, Software should WIN. This is critical for OS stability.

    csr->csrWriteEnable = 1; // HW tries to write 0xDEADBEEF
    csr->pcFromCore     = 0xDEADBEEF;

    csr->busWriteEnable = 1; // SW tries to write 0xCAFEBABE
    csr->busWriteData   = 0xCAFEBABE;

    tick(csr);

    if (csr->mepcValue == 0xCAFEBABE) {
        std::cout << "[PASS] Priority Check: Software Override successful (OS controls the flow).\n";
    } else {
        std::cout << "[FAIL] Priority Check Failed! Hardware overwrote Software. Scheduler is unstable.\n"; 
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] CSR Unit Verified.\n";

    delete csr;
    return 0;
}