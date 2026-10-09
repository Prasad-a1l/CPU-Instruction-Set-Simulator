#include "Simulator.h"
#include <iostream>

void run_benchmark(const std::string& name, const std::vector<Instruction>& prog) {
    std::cout << "====================================\n";
    std::cout << "Running Benchmark: " << name << "\n";
    std::cout << "====================================\n";

    Config cfg_no_fwd;
    cfg_no_fwd.enable_forwarding = false;
    Simulator sim_no_fwd(cfg_no_fwd);
    
    std::cout << ">> Configuration: NO Forwarding (Stall on RAW hazards)\n";
    sim_no_fwd.load_program(prog);
    sim_no_fwd.run();
    sim_no_fwd.print_stats();

    Config cfg_fwd;
    cfg_fwd.enable_forwarding = true;
    Simulator sim_fwd(cfg_fwd);

    std::cout << ">> Configuration: WITH Forwarding\n";
    sim_fwd.load_program(prog);
    sim_fwd.run();
    sim_fwd.print_stats();
}

int main() {
    std::cout << "CPU Instruction Set Simulator and Performance Profiler\n\n";

    std::vector<Instruction> dependency_prog = {
        Assembler::ADDI(1, 0, 5),   // R1 = 5
        Assembler::ADDI(2, 1, 10),  // R2 = R1 + 10  (RAW hazard on R1)
        Assembler::ADD(3, 2, 1),    // R3 = R2 + R1  (RAW hazard on R2, R1)
        Assembler::HALT()
    };

    std::vector<Instruction> fibonacci_prog = {
        // 0: Init
        Assembler::ADDI(1, 0, 10),  // R1 = 10 (N)
        Assembler::ADDI(2, 0, 0),   // R2 = 0  (curr)
        Assembler::ADDI(3, 0, 1),   // R3 = 1  (next)
        Assembler::ADDI(4, 0, 0),   // R4 = 0  (i)
        Assembler::ADDI(10, 0, 0),  // R10 = 0 (base array)

        // Loop:
        // 5: BEQ to END (relative 8 instructions)
        Assembler::BEQ(4, 1, 8),    // if (R4 == R1) goto 13
        // 6: SW curr to Array[R10]
        Assembler::SW(2, 10, 0),
        // 7: R5 = R2 + R3
        Assembler::ADD(5, 2, 3),
        // 8: R2 = R3
        Assembler::ADD(2, 3, 0),
        // 9: R3 = R5
        Assembler::ADD(3, 5, 0),
        // 10: R10++
        Assembler::ADDI(10, 10, 1),
        // 11: R4++
        Assembler::ADDI(4, 4, 1),
        // 12: Jump to Loop
        Assembler::J(5),
        // 13: End
        Assembler::HALT()
    };
    
    std::vector<Instruction> load_use_prog = {
        Assembler::ADDI(10, 0, 100), // R10 = 100 base address
        Assembler::ADDI(5, 0, 42),   // R5 = 42
        Assembler::SW(5, 10, 0),     // Mem[100] = 42
        Assembler::LW(1, 10, 0),     // R1 = Mem[100]
        Assembler::ADD(3, 1, 5),     // R3 = R1 + R5 (Load-Use hazard!)
        Assembler::HALT()
    };

    run_benchmark("Dependency Chain (RAW)", dependency_prog);
    std::cout << "\n";
    run_benchmark("Fibonacci(10) Array Generator", fibonacci_prog);
    std::cout << "\n";
    run_benchmark("Load-Use Hazard", load_use_prog);

    return 0;
}
