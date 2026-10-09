#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <unordered_map>

enum Opcode {
    ADD, SUB, MUL, ADDI, LW, SW, BEQ, BNE, J, HALT, NOP
};

struct Instruction {
    Opcode op;
    int rd, rs, rt; 
    int imm;        
    std::string text; 
    
    Instruction() : op(NOP), rd(0), rs(0), rt(0), imm(0), text("NOP") {}
    Instruction(Opcode op, int rd, int rs, int rt, int imm, std::string text)
        : op(op), rd(rd), rs(rs), rt(rt), imm(imm), text(text) {}
};

struct Config {
    bool enable_forwarding = true;
    bool branch_predict_taken = false; // true = predict taken, false = predict not taken
};

struct ProfilerStats {
    long long total_cycles = 0;
    long long instructions_executed = 0;
    long long stalls = 0;
    long long data_hazards = 0;
    long long control_hazards = 0;
    long long branch_mispredicts = 0;

    void print() const {
        std::cout << "--- Performance Profiler Stats ---\n";
        std::cout << "Total Cycles:          " << total_cycles << "\n";
        std::cout << "Instructions Executed: " << instructions_executed << "\n";
        double cpi = (instructions_executed == 0) ? 0 : (double)total_cycles / instructions_executed;
        std::cout << "CPI:                   " << std::fixed << std::setprecision(3) << cpi << "\n";
        std::cout << "Total Stalls:          " << stalls << "\n";
        std::cout << "Data Hazards Detected: " << data_hazards << "\n";
        std::cout << "Control Hazards:       " << control_hazards << "\n";
        std::cout << "Branch Mispredicts:    " << branch_mispredicts << "\n";
        std::cout << "----------------------------------\n";
    }
};

class Simulator {
private:
    struct IF_ID {
        Instruction inst;
        int pc = 0;
        bool valid = false;
    };

    struct ID_EX {
        Instruction inst;
        int pc = 0;
        int rs_val = 0;
        int rt_val = 0;
        int rd = 0; 
        int rs = 0;
        int rt = 0;
        int imm = 0;
        bool valid = false;
        
        // Control
        bool reg_write = false;
        bool mem_read = false;
        bool mem_write = false;
        bool mem_to_reg = false;
        bool is_branch = false;
        bool is_jmp = false;
    };

    struct EX_MEM {
        Instruction inst;
        int pc = 0;
        int alu_result = 0;
        int rt_val = 0;
        int rd = 0;
        bool valid = false;

        // Control
        bool reg_write = false;
        bool mem_read = false;
        bool mem_write = false;
        bool mem_to_reg = false;
    };

    struct MEM_WB {
        Instruction inst;
        int alu_result = 0;
        int mem_data = 0;
        int rd = 0;
        bool valid = false;

        // Control
        bool reg_write = false;
        bool mem_to_reg = false;
    };

    // State
    std::vector<int> registers;
    std::vector<int> data_memory;
    std::vector<Instruction> inst_memory;
    int pc;
    bool halted;

    // Pipeline registers
    IF_ID if_id;
    ID_EX id_ex;
    EX_MEM ex_mem;
    MEM_WB mem_wb;

    // Config & Stats
    Config config;
    ProfilerStats stats;

    void decode_instruction(const Instruction& inst, ID_EX& next_id_ex);

public:
    Simulator(Config cfg, size_t mem_size = 1024);
    
    void load_program(const std::vector<Instruction>& prog);
    void run();
    void step();
    void reset();
    void print_state() const;
    void print_stats() const { stats.print(); }

    const ProfilerStats& get_stats() const { return stats; }
};

class Assembler {
public:
    // Helper to generate instructions
    static Instruction ADD(int rd, int rs, int rt);
    static Instruction SUB(int rd, int rs, int rt);
    static Instruction MUL(int rd, int rs, int rt);
    static Instruction ADDI(int rt, int rs, int imm);
    static Instruction LW(int rt, int rs, int imm);
    static Instruction SW(int rt, int rs, int imm);
    static Instruction BEQ(int rs, int rt, int imm);
    static Instruction BNE(int rs, int rt, int imm);
    static Instruction J(int imm);
    static Instruction HALT();
    static Instruction NOP();
};

#endif
