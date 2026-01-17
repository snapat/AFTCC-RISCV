#include "Vsoc_top.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    // Instantiate the SoC
    Vsoc_top *dut = new Vsoc_top;

    // Set up Waveform Dumping
    VerilatedVcdC *m_trace = new VerilatedVcdC;
    dut->trace(m_trace, 5);
    m_trace->open("waveform.vcd");

    // Initialize Signals
    dut->clock = 0;
    dut->resetActiveLow = 0; // Hold Reset

    // Simulation Loop (Run for 2000 ticks)
    for (int i = 0; i < 2000; i++) {
        // Toggle Clock
        dut->clock ^= 1; 

        // Release Reset after 10 ticks
        if (i > 10) dut->resetActiveLow = 1;

        // Eval and Dump
        dut->eval();
        m_trace->dump(i);
    }

    // Cleanup
    m_trace->close();
    delete dut;
    exit(0);
}