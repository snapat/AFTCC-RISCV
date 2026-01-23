#include <iostream>
#include <verilated.h>
#include "Vuart_tx.h"

// PARAMETER FROM VERILOG
// We must match this exactly or the test will fail timing checks
const int CLKS_PER_BIT = 108;

// Helper to step the clock
void tick(Vuart_tx* top) {
    top->i_Clk = 0; top->eval();
    top->i_Clk = 1; top->eval();
}

// --- HELPER: SAMPLER FUNCTION ---
// This function advances time by 'n' clocks and returns the majority logic level.
// It effectively simulates a receiver sampling the line.
int sample_line(Vuart_tx* top, int clocks_to_wait) {
    int sum = 0;
    for(int i = 0; i < clocks_to_wait; i++) {
        tick(top);
        sum += top->o_Tx_Serial;
    }
    // If line was High more than half the time, return 1
    return (sum > (clocks_to_wait / 2)) ? 1 : 0;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vuart_tx* uart = new Vuart_tx;

    std::cout << "[TEST] Starting UART Transmitter Verification...\n";
    std::cout << "[INFO] Baud Rate Setting: " << CLKS_PER_BIT << " clocks per bit.\n";

    // ==========================================
    // TEST 1: IDLE STATE VERIFICATION
    // ==========================================
    // UART line must stay HIGH (1) when idle.
    
    uart->i_Tx_DV = 0; // No Data Valid yet
    uart->i_Tx_Byte = 0x00;
    
    // Run for 100 cycles
    int idle_errors = 0;
    for(int i=0; i<100; i++) {
        tick(uart);
        if (uart->o_Tx_Serial == 0) idle_errors++; // Error if Low
        if (uart->i_Tx_Active == 1) idle_errors++; // Error if Active
    }

    if (idle_errors == 0) {
        std::cout << "[PASS] Idle State: Line held High, Active signal Low.\n";
    } else {
        std::cout << "[FAIL] Idle State Violation! Line dropped Low or Active went High.\n";
        return 1;
    }

    // ==========================================
    // TEST 2: TRANSMISSION TIMING & DATA INTEGRITY
    // ==========================================
    // Send character 'A' (0x41) -> Binary 0100 0001
    // UART sends LSB First: Start(0) -> 1 -> 0 -> 0 -> 0 -> 0 -> 0 -> 1 -> 0 -> Stop(1)
    
    uint8_t test_char = 0x41; // 'A'
    std::cout << "[TEST] Sending Character: 'A' (0x41)...\n";

    uart->i_Tx_DV = 1;      // Trigger Start
    uart->i_Tx_Byte = test_char;
    tick(uart);             // Clock it in
    uart->i_Tx_DV = 0;      // Clear Trigger (it's a pulse)

    // --- CHECK 1: START BIT (Must be 0) ---
    // We sample the middle of the bit period to be safe
    int start_bit = sample_line(uart, CLKS_PER_BIT);
    
    if (start_bit == 0) {
        std::cout << "[PASS] Start Bit Detected (Logic 0).\n";
    } else {
        std::cout << "[FAIL] Start Bit Missing! Line stayed High.\n"; return 1;
    }

    // --- CHECK 2: DATA BITS (LSB FIRST) ---
    // Expected sequence for 0x41 (LSB -> MSB): 1, 0, 0, 0, 0, 0, 1, 0
    int expected_bits[8] = {1, 0, 0, 0, 0, 0, 1, 0};
    
    for (int i = 0; i < 8; i++) {
        int received_bit = sample_line(uart, CLKS_PER_BIT);
        
        if (received_bit == expected_bits[i]) {
            // Good
        } else {
            std::cout << "[FAIL] Bit " << i << " Mismatch!\n";
            std::cout << "  Expected: " << expected_bits[i] << " Got: " << received_bit << "\n";
            return 1;
        }
    }
    std::cout << "[PASS] Data Payload (0x41) Verified correctly.\n";

    // --- CHECK 3: STOP BIT (Must be 1) ---
    int stop_bit = sample_line(uart, CLKS_PER_BIT);
    
    if (stop_bit == 1) {
        std::cout << "[PASS] Stop Bit Detected (Logic 1).\n";
    } else {
        std::cout << "[FAIL] Stop Bit Missing! Frame Error.\n"; return 1;
    }

    // --- CHECK 4: COMPLETION SIGNALS ---
    // At this point, o_Tx_Done should pulse High and i_Tx_Active should go Low
    tick(uart); // One extra tick for Cleanup State

    if (uart->o_Tx_Done == 1 && uart->i_Tx_Active == 0) {
        std::cout << "[PASS] Handshake Signals: Done asserted, Active cleared.\n";
    } else {
        std::cout << "[FAIL] Handshake Signals Failed.\n";
        std::cout << "  Done: " << (int)uart->o_Tx_Done << " (Expected 1)\n";
        std::cout << "  Active: " << (int)uart->i_Tx_Active << " (Expected 0)\n";
        return 1;
    }

    // ==========================================
    // TEST 3: BACK-TO-BACK TRANSMISSION
    // ==========================================
    // Verify we can send another byte immediately without getting stuck
    
    uart->i_Tx_DV = 1;
    uart->i_Tx_Byte = 0x55; // 01010101
    tick(uart);
    uart->i_Tx_DV = 0;

    // Fast-forward 10 clocks and check if Active goes High again
    for(int i=0; i<10; i++) tick(uart);

    if (uart->i_Tx_Active == 1 && uart->o_Tx_Serial == 0) {
        std::cout << "[PASS] Stress Test: Back-to-Back transmission accepted (Start Bit valid).\n";
    } else {
        std::cout << "[FAIL] Stress Test Failed. UART did not restart.\n";
        return 1;
    }

    std::cout << "------------------------------------------\n";
    std::cout << "[SUCCESS] UART Transmitter Verified.\n";

    delete uart;
    return 0;
}