# 32-bit RISC-V Core (SystemVerilog)

A cycle-accurate implementation of a 32-bit RISC-V processor built from scratch in SystemVerilog. This project focuses on understanding the microarchitecture of the RV32I instruction set, verified using C++ testbenches via Verilator.

**Current Status:** Epoch 1 Complete (ALU & Register File Verified)

---

## 🏗 Architecture Modules

### 1. Arithmetic Logic Unit (ALU)
A pure combinational logic block handling all integer arithmetic and logical operations.
* **Features:** 32-bit operations, Zero-flag generation for branching.
* **Supported Ops:** ADD, SUB, AND, OR, XOR, SLT (Set Less Than).

**Verification:**
The ALU was verified against a C++ testbench checking corner cases (overflow, zero outputs) and standard arithmetic.

![ALU Waveform](images/alu.png)
*Figure 1: GTKWave trace demonstrating ALU operation switching between ADD (0x0) and SUB (0x1).*

### 2. Register File
A standard 32x32-bit Register File.
* **Architecture:** Dual Read Ports (rs1, rs2), Single Write Port (rd).
* **Constraint:** Register `x0` is hardwired to 0 (Write attempts are ignored).
* **Clocking:** Asynchronous Read, Synchronous Write.

**Verification:**
Verified read-after-write consistency and `x0` immutability.

![Register File Waveform](images/regfile.png)
*Figure 2: Waveform showing simultaneous reads and a synchronous write operation.*

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

# 2. Run the ALU Testbench
./run.sh alu

# 3. Run the Register File Testbench
./run.sh regfile