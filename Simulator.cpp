#include "Simulator.h"

// --- Assembler Helpers ---
Instruction Assembler::ADD(int rd, int rs, int rt) {
    return Instruction(::ADD, rd, rs, rt, 0, "ADD R" + std::to_string(rd) + ", R" + std::to_string(rs) + ", R" + std::to_string(rt));
}
Instruction Assembler::SUB(int rd, int rs, int rt) {
    return Instruction(::SUB, rd, rs, rt, 0, "SUB R" + std::to_string(rd) + ", R" + std::to_string(rs) + ", R" + std::to_string(rt));
}
Instruction Assembler::MUL(int rd, int rs, int rt) {
    return Instruction(::MUL, rd, rs, rt, 0, "MUL R" + std::to_string(rd) + ", R" + std::to_string(rs) + ", R" + std::to_string(rt));
}
Instruction Assembler::ADDI(int rt, int rs, int imm) {
    return Instruction(::ADDI, rt, rs, 0, imm, "ADDI R" + std::to_string(rt) + ", R" + std::to_string(rs) + ", " + std::to_string(imm));
}
Instruction Assembler::LW(int rt, int rs, int imm) {
    return Instruction(::LW, rt, rs, 0, imm, "LW R" + std::to_string(rt) + ", " + std::to_string(imm) + "(R" + std::to_string(rs) + ")");
}
Instruction Assembler::SW(int rt, int rs, int imm) {
    return Instruction(::SW, 0, rs, rt, imm, "SW R" + std::to_string(rt) + ", " + std::to_string(imm) + "(R" + std::to_string(rs) + ")");
}
Instruction Assembler::BEQ(int rs, int rt, int imm) {
    return Instruction(::BEQ, 0, rs, rt, imm, "BEQ R" + std::to_string(rs) + ", R" + std::to_string(rt) + ", " + std::to_string(imm));
}
Instruction Assembler::BNE(int rs, int rt, int imm) {
    return Instruction(::BNE, 0, rs, rt, imm, "BNE R" + std::to_string(rs) + ", R" + std::to_string(rt) + ", " + std::to_string(imm));
}
Instruction Assembler::J(int imm) {
    return Instruction(::J, 0, 0, 0, imm, "J " + std::to_string(imm));
}
Instruction Assembler::HALT() {
    return Instruction(::HALT, 0, 0, 0, 0, "HALT");
}
Instruction Assembler::NOP() {
    return Instruction(::NOP, 0, 0, 0, 0, "NOP");
}


// --- Simulator ---

Simulator::Simulator(Config cfg, size_t mem_size) : config(cfg) {
    registers.resize(32, 0);
    data_memory.resize(mem_size, 0);
    reset();
}

void Simulator::reset() {
    std::fill(registers.begin(), registers.end(), 0);
    std::fill(data_memory.begin(), data_memory.end(), 0);
    pc = 0;
    halted = false;
    stats = ProfilerStats();

    if_id = IF_ID();
    id_ex = ID_EX();
    ex_mem = EX_MEM();
    mem_wb = MEM_WB();
}

void Simulator::load_program(const std::vector<Instruction>& prog) {
    inst_memory = prog;
    reset();
}

void Simulator::decode_instruction(const Instruction& inst, ID_EX& next) {
    next.inst = inst;
    next.valid = true;
    next.rs = inst.rs;
    next.rt = inst.rt;
    next.rd = inst.rd; // Use the configured rd
    next.imm = inst.imm;

    next.reg_write = false;
    next.mem_read = false;
    next.mem_write = false;
    next.mem_to_reg = false;
    next.is_branch = false;
    next.is_jmp = false;

    switch (inst.op) {
        case ADD: case SUB: case MUL:
        case ADDI:
            next.reg_write = true;
            break;
        case LW:
            next.reg_write = true;
            next.mem_read = true;
            next.mem_to_reg = true;
            break;
        case SW:
            next.mem_write = true;
            break;
        case BEQ: case BNE:
            next.is_branch = true;
            break;
        case J:
            next.is_jmp = true;
            break;
        case NOP: case HALT:
            break;
    }
}

void Simulator::run() {
    while (!halted) {
        step();
    }
}

void Simulator::step() {
    if (halted) return;
    stats.total_cycles++;

    // To prevent infinite loop if halt is never reached
    if (stats.total_cycles > 10000) {
        halted = true;
        return;
    }

    // Pipeline registers for NEXT cycle
    IF_ID next_if_id = if_id;
    ID_EX next_id_ex = id_ex;
    EX_MEM next_ex_mem = ex_mem;
    MEM_WB next_mem_wb = mem_wb;

    bool stall = false;
    bool flush_if_id = false;
    bool flush_id_ex = false;

    // ==========================================
    // 5. Write Back (WB) Stage
    // ==========================================
    if (mem_wb.valid && mem_wb.inst.op != NOP) {
        if (mem_wb.reg_write && mem_wb.rd != 0) {
            registers[mem_wb.rd] = mem_wb.mem_to_reg ? mem_wb.mem_data : mem_wb.alu_result;
        }
        if (mem_wb.inst.op == HALT) {
            halted = true;
        }
        stats.instructions_executed++;
    }

    // ==========================================
    // 4. Memory (MEM) Stage
    // ==========================================
    next_mem_wb.valid = ex_mem.valid;
    next_mem_wb.inst = ex_mem.inst;
    if (ex_mem.valid) {
        next_mem_wb.alu_result = ex_mem.alu_result;
        next_mem_wb.rd = ex_mem.rd;
        next_mem_wb.reg_write = ex_mem.reg_write;
        next_mem_wb.mem_to_reg = ex_mem.mem_to_reg;

        if (ex_mem.mem_write) {
            if (ex_mem.alu_result >= 0 && ex_mem.alu_result < data_memory.size()) {
                data_memory[ex_mem.alu_result] = ex_mem.rt_val;
            }
        }
        if (ex_mem.mem_read) {
            if (ex_mem.alu_result >= 0 && ex_mem.alu_result < data_memory.size()) {
                next_mem_wb.mem_data = data_memory[ex_mem.alu_result];
            } else {
                next_mem_wb.mem_data = 0; // out of bounds
            }
        }
    } else {
        next_mem_wb.inst = Assembler::NOP();
    }

    // ==========================================
    // 3. Execute (EX) Stage
    // ==========================================
    next_ex_mem.valid = id_ex.valid;
    next_ex_mem.inst = id_ex.inst;
    if (id_ex.valid) {
        int rs_val = id_ex.rs_val;
        int rt_val = id_ex.rt_val;

        // Forwarding Logic
        if (config.enable_forwarding) {
            // EX to EX forwarding
            if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs) {
                rs_val = ex_mem.alu_result;
            }
            if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == id_ex.rt) {
                rt_val = ex_mem.alu_result;
            }
            // MEM to EX forwarding
            if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0 && mem_wb.rd == id_ex.rs) {
                // EX to EX has priority
                if (!(ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == id_ex.rs)) {
                    rs_val = mem_wb.mem_to_reg ? mem_wb.mem_data : mem_wb.alu_result;
                }
            }
            if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0 && mem_wb.rd == id_ex.rt) {
                if (!(ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == id_ex.rt)) {
                    rt_val = mem_wb.mem_to_reg ? mem_wb.mem_data : mem_wb.alu_result;
                }
            }
        } else {
            // Check for hazards when forwarding is disabled
            // If ID reads a register that an older instruction is going to write, stall.
            // (Wait, this should be detected in ID stage, but since values are read in ID, we do hazard detection in ID block)
        }

        next_ex_mem.rt_val = rt_val; // For SW
        next_ex_mem.rd = id_ex.rd;
        next_ex_mem.reg_write = id_ex.reg_write;
        next_ex_mem.mem_read = id_ex.mem_read;
        next_ex_mem.mem_write = id_ex.mem_write;
        next_ex_mem.mem_to_reg = id_ex.mem_to_reg;

        // ALU Execution
        switch (id_ex.inst.op) {
            case ADD: next_ex_mem.alu_result = rs_val + rt_val; break;
            case SUB: next_ex_mem.alu_result = rs_val - rt_val; break;
            case MUL: next_ex_mem.alu_result = rs_val * rt_val; break;
            case ADDI: next_ex_mem.alu_result = rs_val + id_ex.imm; break;
            case LW: 
            case SW:   next_ex_mem.alu_result = rs_val + id_ex.imm; break;
            default:   next_ex_mem.alu_result = 0; break;
        }

        // Branch Resolution (Using "Always Not Taken" predict logic implicitly by fetching PC+1)
        if (id_ex.is_branch || id_ex.is_jmp) {
            bool branch_taken = false;
            if (id_ex.inst.op == BEQ && rs_val == rt_val) branch_taken = true;
            if (id_ex.inst.op == BNE && rs_val != rt_val) branch_taken = true;
            if (id_ex.is_jmp) branch_taken = true;

            if (branch_taken) {
                stats.branch_mispredicts++;
                stats.control_hazards++;
                flush_id_ex = true; // signal to flush the wrong path
                
                if (id_ex.is_jmp) {
                    pc = id_ex.imm;
                } else {
                    pc = id_ex.pc + id_ex.imm; // Relative branch
                }
            }
        }
    } else {
        next_ex_mem.inst = Assembler::NOP();
    }

    // ==========================================
    // 2. Instruction Decode (ID) Stage
    // ==========================================
    if (if_id.valid) {
        decode_instruction(if_id.inst, next_id_ex);
        next_id_ex.pc = if_id.pc;
        
        // Read registers
        next_id_ex.rs_val = registers[next_id_ex.rs];
        next_id_ex.rt_val = registers[next_id_ex.rt];

        // Hazard Detection Unit
        if (config.enable_forwarding) {
            // Load-Use Hazard
            if (id_ex.valid && id_ex.mem_read && id_ex.rd != 0) {
                if (id_ex.rd == next_id_ex.rs || id_ex.rd == next_id_ex.rt) {
                    stall = true;
                    stats.data_hazards++;
                }
            }
        } else {
            // If forwarding is disabled, we stall on ANY RAW hazard. 
            // Check if older instructions (EX, MEM, WB) will write to rs or rt.
            bool hazard = false;
            if (next_id_ex.rs != 0) {
                if (id_ex.valid && id_ex.reg_write && id_ex.rd == next_id_ex.rs) hazard = true;
                if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd == next_id_ex.rs) hazard = true;
                if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd == next_id_ex.rs) hazard = true;
            }
            if (next_id_ex.rt != 0) {
                if (id_ex.valid && id_ex.reg_write && id_ex.rd == next_id_ex.rt) hazard = true;
                if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd == next_id_ex.rt) hazard = true;
                if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd == next_id_ex.rt) hazard = true;
            }
            
            if (hazard && next_id_ex.inst.op != NOP && next_id_ex.inst.op != J && next_id_ex.inst.op != HALT) {
                stall = true;
                stats.data_hazards++;
            }
        }
    } else {
        next_id_ex.valid = false;
        next_id_ex.inst = Assembler::NOP();
    }

    // ==========================================
    // 1. Instruction Fetch (IF) Stage
    // ==========================================
    if (!stall) {
        if (pc < inst_memory.size()) {
            next_if_id.inst = inst_memory[pc];
            next_if_id.pc = pc;
            next_if_id.valid = true;
            pc++;
        } else {
            next_if_id.valid = false;
            next_if_id.inst = Assembler::NOP();
        }
    }

    // ==========================================
    // Apply Pipeline Updates
    // ==========================================
    if (flush_id_ex) {
        stall = false; // Ignore any stalls generated by the flushed instruction
    }

    if (stall) {
        stats.stalls++;
        next_id_ex.valid = false;
        next_id_ex.inst = Assembler::NOP();
        next_id_ex.reg_write = false;
        next_id_ex.mem_read = false;
        next_id_ex.mem_write = false;
        next_if_id = if_id; // preserve
    }

    if (flush_id_ex) {
        // Flush the instruction that was trailing the branch
        next_id_ex.valid = false;
        next_id_ex.inst = Assembler::NOP();
        next_id_ex.reg_write = false;
        next_id_ex.mem_read = false;
        next_id_ex.mem_write = false;
    }

    if_id = next_if_id;
    id_ex = next_id_ex;
    ex_mem = next_ex_mem;
    mem_wb = next_mem_wb;
}

