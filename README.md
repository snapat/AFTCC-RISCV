# 32-bit RISC-V Core (SystemVerilog)

A cycle-accurate implementation of a 32-bit RISC-V processor built from scratch in SystemVerilog. This project focuses on High-Performance Embedded Computing (HPEC) and Hardware-Software Integration (HSI), featuring a custom preemptive multitasking architecture verified using C++ testbenches via Verilator.

**Current Status:** Phase 2 Complete (SoC Integration & Hardware Interrupts Verified)

---

## 📸 The "Reflex" Verification (Phase 2)
The critical milestone for Phase 2 was verifying the **Hardware-Based Preemption**.

**The Proof:**
The waveform below demonstrates the "Handshake" between the Timer and the Controller.
* **Condition:** `timerInterrupt` goes HIGH.
* **Reaction:** `programCounter` immediately vectors to `0x00000020` (The Trap Handler) instead of the next instruction.
* **State:** The CPU effectively "hijacks" the fetch stage to enforce a context switch.

![Phase 2 Reflex](images/reflex_proof.png)
*(Note: Capture a zoomed-in screenshot of `clk`, `pc`, `timerInterrupt`, and `instr` and save it as `images/reflex_proof.png`)*

---

## 🏗 Architecture Modules

### 1. SoC Top (System-on-Chip)
The integration layer connecting the CPU "Organs" to the "Skeleton".
* **Components:** CPU Core, Instruction Memory (ROM), Data Memory (RAM), GPIO, and Timer.
* **Features:**
    * **Bus Interconnect:** Simple router for Memory and I/O traffic.
    * **Preemption Timer:** A hardware counter that triggers an interrupt every 50 cycles (simulation) to enable preemptive multitasking.
* **Memory Architecture:** Split Instruction (ROM) and Data (RAM) memory to avoid structural hazards (Harvard Architecture).

### 2. Main Controller (Decoder & Reflex)
The "Brain" of the CPU, updated to handle **Exceptional Control Flow**.
* **Inputs:** `opcode`, `funct3`, `funct7`, `timerInterrupt`.
* **Outputs:** Control signals for Datapath and CSR Write Enable.
* **Interrupt Logic:** Handles the hardware vectoring to the trap handler (`0x10`) when `timerInterrupt` is high.

### 3. Legacy Modules (Phase 1)
* **ALU:** Pure combinational logic handling ADD, SUB, AND, OR, XOR, SLT.
* **Register File:** Standard 32x32-bit Register File (x0 hardwired to 0).

*(Previous verification images archived in `old_sim/`)*

---

## 🚀 How to Run

This project uses **Verilator** for fast C++ based simulation and **GTKWave** for debugging.

### Prerequisites
* **MacOS:** `brew install verilator gtkwave`
* **Linux:** `sudo apt install verilator gtkwave`

### Simulation Script
A unified build script `run.sh` handles compilation and execution.

```bash
# 1. Make the script executable
chmod +x run.sh

# 2. Run the Full SoC Simulation (Phase 2)
./run.sh soc_top

# 3. View the Waveform
open waveform.vcd