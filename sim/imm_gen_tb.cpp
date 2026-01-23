#include <iostream>
#include <verilated.h>
#include "Vimm_gen.h"

// --- OPCODE DEFINITIONS ---
#define OP_I_TYPE  0x13 // 0010011 (ADDI)
#define OP_S_TYPE  0x23 // 0100011 (SW)
#define OP_B_TYPE  0x63 // 1100011 (BEQ)
#define OP_U_TYPE  0x37 // 0110111 (LUI)
#define OP_J_TYPE  0x6F // 1101111 (JAL)

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vimm_gen* dut = new Vimm_gen;

    std::cout << "[TEST] Starting Immediate Generator Verification...\n";

    // ==========================================
    // TEST 1: I-TYPE (Negative Number Check)
    // ==========================================
    // Instruction: ADDI x0, x0, -1
    // Imm (-1) = 0xFFF (12 bits)
    // Encoding: [Imm 11:0] [rs1] [funct3] [rd] [opcode]
    //           111111111111 00000 000 00000 0010011
    // Hex: 0xFFF00013
    
    dut->instruction = 0xFFF00013;
    dut->eval();

    // Expect 32-bit sign extended -1 (0xFFFFFFFF)
    if (dut->immediateValue == 0xFFFFFFFF) {
        std::cout << "[PASS] I-Type (Negative): Sign Extension correct (-1).\n";
    } else {
        std::cout << "[FAIL] I-Type Failed. Expected -1, Got: " << (int)dut->immediateValue << "\n";
        return 1;
    }

    // ==========================================
    // TEST 2: S-TYPE (Split Field Check)
    // ==========================================
    // Instruction: SW x0, -1(x0)
    // Imm (-1) = 0xFFF
    // Split: imm[11:5] goes to inst[31:25], imm[4:0] goes to inst[11:7]
    //        1111111          11111
    // Encoding: 1111111 00000 00000 010 11111 0100011
    // Hex: 0xFE002F23
    
    dut->instruction = 0xFE002FA3;
    dut->eval();

    if (dut->immediateValue == 0xFFFFFFFF) {
        std::cout << "[PASS] S-Type (Store): Split Immediate Reassembly correct (-1).\n";
    } else {
        std::cout << "[FAIL] S-Type Failed. Expected -1, Got: " << (int)dut->immediateValue << "\n";
        return 1;
    }

    // ==========================================
    // TEST 3: B-TYPE (The "Scrambler" Check)
    // ==========================================
    // Instruction: BEQ x0, x0, -4 (Jump back 4 bytes)
    // Imm (-4) = ...11111100
    // RISC-V drops bit 0. Encoded bits: ...1111110
    // Bit 12 (Sign) -> Inst[31]
    // Bit 11        -> Inst[7]
    // Bits 10:5     -> Inst[30:25]
    // Bits 4:1      -> Inst[11:8]
    //
    // Let's use a known compiler output: 0xFE000EE3 (beq x0, x0, -4)
    
    dut->instruction = 0xFE000EE3;
    dut->eval();

    // Expected: -4 (0xFFFFFFFC)
    if (dut->immediateValue == 0xFFFFFFFC) {
        std::cout << "[PASS] B-Type (Branch): Scrambler Logic correct (-4).\n";
    } else {
        std::cout << "[FAIL] B-Type Failed. Expected -4 (0xFFFFFFFC), Got: " << std::hex << dut->immediateValue << "\n";
        return 1;
    }

    // ==========================================
    // TEST 4: U-TYPE (Upper Immediate)
    // ==========================================
    // Instruction: LUI x1, 0x12345
    // Encoding: 0x12345 + rd(1) + op(37) -> 0x123450B7
    
    dut->instruction = 0x123450B7;
    dut->eval();

    // Expected: 0x12345000 (Lower 12 bits zeroed)
    if (dut->immediateValue == 0x12345000) {
        std::cout << "[PASS] U-Type (LUI): Shift Logic correct.\n";
    } else {
        std::cout << "[FAIL] U-Type Failed. Expected 0x12345000, Got: " << std::hex << dut->immediateValue << "\n";
        return 1;
    }

    // ==========================================
    // TEST 5: J-TYPE (Function Call Check)
    // ==========================================
    // Instruction: JAL x1, -4
    // Known Hex: 0xFFDFF0EF
    
    dut->instruction = 0xFFDFF0EF;
    dut->eval();

    // Expected: -4 (0xFFFFFFFC)
    if (dut->immediateValue == 0xFFFFFFFC) {
        std::cout << "[PASS] J-Type (Jump): Scrambler Logic correct (-4).\n";
    } else {
        std::cout << "[FAIL] J-Type Failed. Expected -4, Got: " << std::hex << dut->immediateValue << "\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] Immediate Generator Verified.\n";

    delete dut;
    return 0;
}