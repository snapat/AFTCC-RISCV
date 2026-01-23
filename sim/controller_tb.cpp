#include <iostream>
#include <verilated.h>
#include "Vcontroller.h"

// --- OPCODE DEFINITIONS ---
#define OP_R_TYPE  0x33 // 0110011
#define OP_I_TYPE  0x13 // 0010011
#define OP_LOAD    0x03 // 0000011
#define OP_STORE   0x23 // 0100011
#define OP_BRANCH  0x63 // 1100011
#define OP_LUI     0x37 // 0110111
#define OP_JAL     0x6F // 1101111
#define OP_SYSTEM  0x73 // 1110011

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vcontroller* dut = new Vcontroller;

    std::cout << "[TEST] Starting Controller Verification...\n";

    // ==========================================
    // TEST 1: R-TYPE (ADD)
    // ==========================================
    dut->timerInterrupt = 0;
    dut->opcode = OP_R_TYPE;
    dut->funct3 = 0; // ADD
    dut->funct7 = 0;
    dut->eval();

    if (dut->registerWriteEnable == 1 && dut->aluInputSource == 0 && dut->memoryWriteEnable == 0) {
        std::cout << "[PASS] R-Type (ADD) Decode Correct.\n";
    } else {
        std::cout << "[FAIL] R-Type (ADD) Decode Failed.\n"; return 1;
    }

    // ==========================================
    // TEST 2: LOAD (LW)
    // ==========================================
    dut->opcode = OP_LOAD;
    dut->funct3 = 2; // LW
    dut->eval();

    // Check: Should write to Reg (1), Read from Mem (resultSource=1), Calc Addr (AluSrc=1)
    if (dut->registerWriteEnable == 1 && dut->resultSource == 1 && dut->aluInputSource == 1) {
        std::cout << "[PASS] Load (LW) Decode Correct.\n";
    } else {
        std::cout << "[FAIL] Load (LW) Decode Failed.\n"; return 1;
    }

    // ==========================================
    // TEST 3: STORE (SW)
    // ==========================================
    dut->opcode = OP_STORE;
    dut->funct3 = 2; // SW
    dut->eval();

    // Check: MemWrite=1, RegWrite=0
    if (dut->memoryWriteEnable == 1 && dut->registerWriteEnable == 0) {
        std::cout << "[PASS] Store (SW) Decode Correct.\n";
    } else {
        std::cout << "[FAIL] Store (SW) Decode Failed.\n"; return 1;
    }

    // ==========================================
    // TEST 4: BRANCH (BEQ)
    // ==========================================
    dut->opcode = OP_BRANCH;
    dut->funct3 = 0; // BEQ
    dut->eval();

    // Check: isBranch=1, ALU needs to SUB to compare (aluOpCategory=01 -> aluControl=001)
    if (dut->isBranch == 1 && dut->aluControlSignal == 1) { // 3'b001 is SUB
        std::cout << "[PASS] Branch (BEQ) Decode Correct.\n";
    } else {
        std::cout << "[FAIL] Branch (BEQ) Decode Failed. ALU Control: " << (int)dut->aluControlSignal << "\n"; return 1;
    }

    // ==========================================
    // TEST 5: INTERRUPT PRIORITY (CRITICAL SAFETY TEST)
    // ==========================================
    // Scenario: The CPU is trying to do a STORE (Dangerous!)
    // But a Timer Interrupt happens at the exact same time.
    // The Controller MUST disable the Store and force a Trap.
    
    dut->opcode = OP_STORE; // Trying to write to RAM
    dut->timerInterrupt = 1; // INTERRUPT FIRES!
    dut->eval();

    if (dut->isTrap == 1 && dut->csrWriteEnable == 1) {
        // SUCCESS: Trap Logic Active
        if (dut->memoryWriteEnable == 0 && dut->registerWriteEnable == 0) {
             std::cout << "[PASS] Interrupt Priority Verified: STORE instruction aborted safely.\n";
        } else {
             std::cout << "[FAIL] DANGER! Interrupt Fired but MemWrite/RegWrite still active!\n"; return 1;
        }
    } else {
        std::cout << "[FAIL] Interrupt Priority Failed. isTrap not asserted.\n"; return 1;
    }

    // Reset Interrupt for next tests
    dut->timerInterrupt = 0;

    // ==========================================
    // TEST 6: SYSTEM RETURN (MRET)
    // ==========================================
    dut->opcode = OP_SYSTEM;
    dut->funct3 = 0; 
    dut->eval();

    if (dut->isReturn == 1) {
        std::cout << "[PASS] System (MRET) Decode Correct.\n";
    } else {
        std::cout << "[FAIL] System (MRET) Decode Failed.\n"; return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Controller Logic Verified.\n";

    delete dut;
    return 0;
}