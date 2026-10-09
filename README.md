# CPU Instruction Set Simulator & Performance Profiler

A lightweight yet highly robust 5-stage pipeline CPU simulator written in C++. It models a classic RISC architecture (like MIPS) complete with programmable pipeline registers, dynamic execution modeling, control and data hazard detection, and configurable data forwarding routes.

## Features

- **Standard 5-Stage Pipeline**: Simulates the Instruction Fetch (`IF`), Instruction Decode (`ID`), Execute (`EX`), Memory (`MEM`), and Write-Back (`WB`) stages, accurately mimicking sub-cycle latencies through reversed-pipeline simulation updates.
- **Configurable Forwarding**: The forwarding data-path can be dynamically toggled.
  - **With Forwarding**: Resolves standard RAW hazards with `EX-to-EX` and `MEM-to-EX` paths. Injects 1-cycle architectural stalls cleanly for `Load-Use` scenarios.
  - **Without Forwarding**: Stalls the pipeline fully on all Read-After-Write scenarios until instructions clear the appropriate WB stage.
- **Micro-architecture Simulation**: Simulates an actual 32-word Register File (with `R0` hardwired to 0) and virtualized Main Memory arrays.
- **Branch Prediction Strategy**: Uses an implicit "Predict Not-Taken" branching model. On a taken branch, the execution stage statically flushes the erroneous pipeline paths (clearing `IF` and `ID` pipelines), injecting realistic 2-cycle control penalties.
- **Performance Profiling**: Exhaustively tracks metrics such as Total Cycles, Instructions Executed, Cycles Per Instruction (CPI), Total Data Stalls, and Miss-predict Penalties.

## Simulated Instruction Set Architecture (ISA)

Provides a lightweight, programmable C++ Object assembler that allows raw configuration of:

- **ALU Operations**: `ADD`, `SUB`, `MUL`
- **Immediate ALU**: `ADDI`
- **Memory Operations**: `LW`, `SW`
- **Control Flow**: `BEQ`, `BNE`, `J`
- **Operations**: `NOP`, `HALT`

## Benchmarks & Analysis

The repository contains pre-written benchmarking programs embedded within `main.cpp` that showcase structural correctness under high-load pipeline scenarios. 

1. **Dependency Chain `(RAW Hazard)`**: Demonstrates intense cascading dependencies.
   - *Without Forwarding*: Requires 6 stalls dynamically; CPI ~3.5
   - *With Forwarding*: 0 Stalls (Execution paths completely bypass registers); CPI ~2.0
2. **Load-Use Demonstration**: Accurately simulates the fundamental hardware rule requiring 1 mandatory stall cycle even *with* forwarding netlists because the requested Memory block isn't readable until the end of the MEM stage.
3. **Fibonacci(10) Generator Loop**: Simulates tight Array calculations with jumping/looping. Effectively shows Branch Forwarding penalties.

## Build and Run

To build the executable, all you need is a standard C++ compiler like `g++` or `clang++`.

```bash
# Compile the CPU Simulator and the embedded Benchmarks
g++ Simulator.cpp main.cpp -o sim

# Run the metrics visualization suite
./sim
```
