# 32-bit RISC-V Preemptive Multitasking Core

A cycle-accurate implementation of a 32-bit RISC-V processor built from scratch in SystemVerilog. This project demonstrates **Hardware-Enforced Preemptive Multitasking** on a bare-metal Single-Cycle architecture.

**Current Status:** Phase 3 Complete (Hardware Interrupts & Context Switching Verified)

---

##  Architectural Philosophy & Trade-offs
This core was designed with specific constraints to prioritize architectural clarity and atomic instruction execution.

### 1. The "Preemptive-Lite" Scheduler
* **Mechanism:** Uses a custom hardware timer that triggers a trap (`timerInterrupt`) every 50 clock cycles.
* **Policy:** Implements a **Round-Robin** scheduler in software.
* **Why Round-Robin?** While simple, it provides deterministic task switching and ensures fairness (anti-starvation), which is critical for verifying the hardware interrupt logic without the complexity of priority queues.

### 2. The "Zero-Wait" Combinational Bus
* **Design:** A custom memory interface modeled on the **AXI-Lite Topology** (separate Read/Write Address & Data channels).
* **The Trade-off:** Standard AXI-Lite uses a `VALID`/`READY` handshake which introduces wait states.
* **The Solution:** To maintain a strict **CPI = 1 (Single-Cycle)** performance without complex stall logic in the controller, the `READY` signal is effectively tied HIGH. This creates a high-performance combinational path where memory transactions are atomic and instantaneous.

### 3. Atomic Interrupt Handling
* **Why Single-Cycle?** In pipelined architectures, interrupts require flushing instructions in Fetch/Decode stages.
* **This Core:** Because every instruction completes in exactly one cycle, interrupts are **Atomic**. The processor effectively "finishes" the current instruction and traps to the OS vector (`0x10`) on the exact same clock edge, ensuring the architectural state is never inconsistent.

---

##  System Modules

### 1. SoC Top (System-on-Chip)
The integration layer connecting the CPU to the "Outside World".
* **Memory Architecture:** Physically split Instruction (ROM) and Data (RAM) blocks to prevent **Structural Hazards** during simultaneous Fetch and Load/Store operations.
* **Preemption Timer:** A hardware "heartbeat" that enforces the time slice. It asserts `timerInterrupt` when `timerCount >= TIMER_LIMIT` (50 cycles).

### 2. The Controller (The Brain)
Updated to support the **Trap & Return** lifecycle.
* **Trap Vectoring:** When `isTrap` is asserted, the PC Multiplexer forcibly jumps to `0x00000010` while simultaneously saving the current address to the `mepc` (Machine Exception PC) CSR.
* **Return Logic:** Decodes the `mret` instruction to restore `PC <= mepc`, resuming the user program exactly where it left off.

### 3. Bus Interconnect
* **Topology:** Multi-Master capable (CPU + DMA support logic).
* **Arbitration:** Fixed-priority arbitration logic is present to allow future DMA expansion, though the current demo prioritizes CPU traffic to guarantee instruction fetch timing.

---

##  Verification (Phase 3)

The waveform below demonstrates the "Reflex" action of the CPU:
1.  **Trigger:** `timerInterrupt` goes HIGH.
2.  **Response:** The PC effectively "hijacks" the next fetch, jumping to `0x10` (Trap Handler).
3.  **Restoration:** After the scheduler runs, `mret` restores the PC to the saved address.

![Phase 3 Reflex](images/reflex_proof.png)

> **Note:** Current simulation uses a verification hex file to exercise the PC jump logic. Full C-based RTOS scheduler integration is the final milestone.

---

##  How to Run

This project uses **Verilator** for fast C++ based simulation and **GTKWave** for debugging.

### Prerequisites
* **MacOS:** `brew install verilator gtkwave`
* **Linux:** `sudo apt install verilator gtkwave`

### Simulation Script
A unified build script `run.sh` handles compilation and execution.

```bash
# 1. Make the script executable
chmod +x run.sh

# 2. Run the Full SoC Simulation
./run.sh soc_top

# 3. View the Waveform
open waveform.vcd