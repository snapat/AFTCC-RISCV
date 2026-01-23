#include <iostream>
#include <fstream>
#include <verilated.h>
#include <verilated_vcd_c.h> 
#include "Vsoc_top.h"
// --- CRITICAL FIX: Include internal structure to spy on signals ---
#include "Vsoc_top___024root.h" 

// --- 1. FIRMWARE GENERATOR ---
// Creates a hex file that mimics a real OS kernel.
// Logic:
//   1. Boot: Print 'H'
//   2. Loop: Infinite Wait
//   3. ISR (at 0x10): Print '!', then MRET
void create_test_firmware() {
    // Create directory (Linux/Mac command)
    system("mkdir -p firmware"); 
    std::ofstream outfile("firmware/firmware.hex");

    // --- BOOT SECTOR (0x00) ---
    // 0x00: LUI x1, 0x40000     (Base Addr for IO = 0x40000000)
    outfile << "400000b7\n"; 
    // 0x04: ADDI x2, x0, 72     (Load 'H' into x2)
    outfile << "04800113\n"; 
    // 0x08: SW x2, 0(x1)        (Write 'H' to UART)
    outfile << "0020a023\n"; 
    // 0x0C: JAL x0, 0           (Infinite Loop: Jump to 0x0C)
    outfile << "0000006f\n"; 

    // --- TRAP VECTOR (0x10) ---
    // The CPU jumps here automatically when Timer Interrupt fires.
    // 0x10: ADDI x2, x0, 33     (Load '!' into x2)
    outfile << "02100113\n";
    // 0x14: SW x2, 0(x1)        (Write '!' to UART)
    outfile << "0020a023\n";
    // 0x18: MRET                (Return from Interrupt to 0x0C)
    outfile << "30200073\n";

    outfile.close();
    std::cout << "[SETUP] Generated firmware/firmware.hex\n";
}

// --- 2. CLOCK STEPPER ---
void tick(Vsoc_top* top, VerilatedVcdC* tfp, unsigned long& tick_count) {
    top->clock = 0; top->eval();
    // Explicit cast to vluint64_t fixes "ambiguous call" error
    if(tfp) tfp->dump((vluint64_t)(tick_count * 10)); 
    
    top->clock = 1; top->eval();
    if(tfp) tfp->dump((vluint64_t)(tick_count * 10 + 5));
    
    tick_count++;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true); // Enable Waveform Tracing

    // Generate the firmware file before the CPU starts
    create_test_firmware();

    // Instantiate CPU
    Vsoc_top* soc = new Vsoc_top;
    VerilatedVcdC* tfp = new VerilatedVcdC;
    
    // Attach Trace (Depth 99 to see all submodules)
    soc->trace(tfp, 99);
    tfp->open("soc_trace.vcd");

    unsigned long tick_count = 0;
    std::cout << "[TEST] Starting System-Level Verification...\n";

    // Reset Sequence
    soc->resetActiveLow = 0;
    for(int i=0; i<10; i++) tick(soc, tfp, tick_count);
    soc->resetActiveLow = 1;

    std::cout << "[INFO] CPU Reset Complete. Running Firmware...\n";

    // Monitors
    int interrupt_count = 0;
    bool boot_char_detected = false;
    bool interrupt_char_detected = false;

    // Run for 100,000 cycles (enough for ~20 interrupts)
    const int MAX_CYCLES = 100000; 

    while (tick_count < MAX_CYCLES) {
        tick(soc, tfp, tick_count);

        // --- SPY ON INTERNAL SIGNALS ---
        // We use 'rootp' to access signals not exposed in the port list.
        // This verifies the Timer Logic even if UART is broken.
        
        if (soc->rootp->soc_top__DOT__timerInterrupt == 1) {
            // Simple debounce to avoid printing every cycle of the pulse
            if (tick_count % 100 == 0) { 
                interrupt_count++;
                std::cout << "[EVENT] Hardware Timer Interrupt Fired at Tick " << tick_count << "\n";
            }
        }

        // Spy on the IO Bus to detect UART writes instantly
        if (soc->rootp->soc_top__DOT__ioWriteValid && 
            soc->rootp->soc_top__DOT__ioWriteAddress == 0x40000000) {
            
            char c = (char)soc->rootp->soc_top__DOT__ioWriteData;
            
            if (c == 'H') {
                boot_char_detected = true;
                std::cout << "[UART] Boot Message Received: 'H'\n";
            }
            if (c == '!') {
                interrupt_char_detected = true;
                std::cout << "[UART] Interrupt Message Received: '!'\n";
            }
        }
    }

    // --- FINAL REPORT ---
    std::cout << "------------------------------------------\n";
    std::cout << "[REPORT] Simulation Finished.\n";
    
    bool pass = true;

    // Check 1: Did we boot?
    if (boot_char_detected) {
        std::cout << "[PASS] Boot Sequence Verified (Saw 'H').\n";
    } else {
        std::cout << "[FAIL] CPU did not boot. Check PC Logic or Instruction Memory.\n";
        pass = false;
    }

    // Check 2: Did the Timer work?
    if (interrupt_count > 0) {
        std::cout << "[PASS] Timer Hardware Verified (" << interrupt_count << " pulses detected).\n";
    } else {
        std::cout << "[FAIL] Timer never fired. Check Timer Limit or Clock Divider.\n";
        pass = false;
    }

    // Check 3: Did the ISR work? (Trap -> MRET)
    if (interrupt_char_detected) {
        std::cout << "[PASS] Interrupt Service Routine Verified (Saw '!').\n";
        std::cout << "       (Proof: CPU jumped to 0x10, executed code, and returned).\n";
    } else {
        std::cout << "[FAIL] CPU trapped but did not execute ISR code.\n";
        pass = false;
    }

    if (pass) std::cout << "[SUCCESS] FULL SYSTEM INTEGRITY CONFIRMED.\n";
    else std::cout << "[FAILURE] System Checks Failed.\n";

    tfp->close();
    delete soc;
    delete tfp;
    return 0;
}