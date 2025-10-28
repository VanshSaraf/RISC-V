   #include <bits/stdc++.h>

using namespace std;

// Struct for control signals (unchanged)
struct ControlSignals {
    bool regWrite = false;
    bool memRead = false;
    bool memWrite = false;
    int aluSrc = 0; // 0: use register value, 1: use immediate value
    int aluOp = 0; // 0: add, 1: beq
    int memToReg = 0; // 0: use ALU result, 1: use memory read value
    int branch = 0; // 0: no branch, 1: branch
};

// Struct for pipeline registers (modified)
struct PipelineRegisters {
    struct IF_ID { uint32_t instr = 0; uint32_t pc = 0; bool valid = false;bool nop = false; };
    struct ID_EX { 
        uint32_t pc = 0;
        uint32_t rs1 = 0, rs2 = 0, rd = 0, imm = 0, opcode = 0;
        uint32_t rs1_val = 0, rs2_val = 0; // Added for forwarding
        ControlSignals ctrl;
        bool valid = false;
        bool stall = false;
    };
    struct EX_MEM { uint32_t alu_result = 0, rs2=0, rs2_val = 0, rd = 0, pc = 0; bool stall = false; ControlSignals ctrl; bool valid = false; bool is_zero = false; }; // rs2 replaced with rs2_val
    struct MEM_WB { uint32_t mem_data = 0, alu_result = 0, rd = 0, pc = 0; bool stall = false; ControlSignals ctrl; bool valid = false; };
    
    IF_ID if_id;
    ID_EX id_ex;
    EX_MEM ex_mem;
    MEM_WB mem_wb;
};

// Unchanged functions
void setControlSignals(uint32_t instr, ControlSignals& ctrl, uint32_t& opcode) {
    opcode = instr & 0x7F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t funct7 = (instr >> 25) & 0x7F;
    switch (opcode) {
        case 0x33: // R-type
            ctrl.regWrite = true; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 0; 
            ctrl.aluOp = 1; 
            ctrl.memToReg = 0; 
            ctrl.branch = 0;
            break;
        case 0x03: // Load
            ctrl.regWrite = true; 
            ctrl.memRead = true; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 1; 
            ctrl.branch = 0;
            break;
        case 0x13: // Immediate
            ctrl.regWrite = true; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 0;
            break;
        case 0x23: // Store
            ctrl.regWrite = false; 
            ctrl.memRead = false; 
            ctrl.memWrite = true;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 0;
            break;
        case 0x63: // Branch
            ctrl.regWrite = false; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 0; 
            ctrl.aluOp = 1; 
            ctrl.memToReg = 0; 
            ctrl.branch = 1;
            break;
        case 0x37: // lui
            ctrl.regWrite = true; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 0;
            break;
        case 0x67: // jalr
            ctrl.regWrite = true; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 1;
            break;
        case 0x6F: // jal
            ctrl.regWrite = true; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 1;
            break;
        case 0x17: // auipc
            ctrl.regWrite = true; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 1; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 0;
            break;
        default:
            ctrl.regWrite = false; 
            ctrl.memRead = false; 
            ctrl.memWrite = false;
            ctrl.aluSrc = 0; 
            ctrl.aluOp = 0; 
            ctrl.memToReg = 0; 
            ctrl.branch = 0;
            break;
    }
}

void generateImmediate(uint32_t instr, PipelineRegisters& tempRegs) {
    switch (tempRegs.id_ex.opcode) {
        case 0x13: 
        case 0x03: 
        case 0x67: // addi, lb, jalr
            tempRegs.id_ex.imm = static_cast<int32_t>(instr >> 20);
            tempRegs.id_ex.imm = (tempRegs.id_ex.imm & 0x800) ? (tempRegs.id_ex.imm | 0xFFFFF000) : tempRegs.id_ex.imm;
            break;
        case 0x63: // beq
            tempRegs.id_ex.imm = ((instr >> 31) << 12) | (((instr >> 25) & 0x3F) << 5) |
                                 (((instr >> 8) & 0xF) << 1) | (((instr >> 7) & 0x1) << 11);
            tempRegs.id_ex.imm = (tempRegs.id_ex.imm & 0x800) ? (tempRegs.id_ex.imm | 0xFFFFF000) : tempRegs.id_ex.imm;
            break;
        case 0x6F: // jal
            tempRegs.id_ex.imm = ((instr >> 31) << 20) | (((instr >> 12) & 0xFF) << 12) |
                                 (((instr >> 20) & 0x1) << 11) | (((instr >> 21) & 0x3FF) << 1);
            tempRegs.id_ex.imm = (tempRegs.id_ex.imm & 0x80000) ? (tempRegs.id_ex.imm | 0xFFF00000) : tempRegs.id_ex.imm;
            break;
        case 0x23: // Store (S-type)
            tempRegs.id_ex.imm = ((instr >> 25) & 0x7F) << 5 | ((instr >> 7) & 0x1F);
            tempRegs.id_ex.imm = (tempRegs.id_ex.imm & 0x800) ? (tempRegs.id_ex.imm | 0xFFFFF000) : tempRegs.id_ex.imm;
            break;
    }
}

// Forwarding Processor class
class ForwardingProcessor {
private:
    vector<uint32_t> registers; // 32 registers
    vector<uint32_t> instrMemory; // Instruction memory
    vector<uint32_t> dataMemory; // Data memory
    vector<string> instructions; // Instruction strings for display
    PipelineRegisters pipeRegs;
    uint32_t pc = 0;
    bool is_stall=false;
    bool is_nop = false;
    // bool upcoming_nop = false;
    uint32_t branch_pc=0;
    bool branch_taken=false;
    bool is_branch=false;

    // Check for stalls (only load-use hazards)
    bool check_stall(PipelineRegisters& pipeRegs) {
        if (!pipeRegs.if_id.valid) {
            return false; // No instruction in ID
        }

        uint32_t instr = pipeRegs.if_id.instr;
        uint32_t opcode = instr & 0x7F;
        uint32_t rs1 = (instr >> 15) & 0x1F;
        uint32_t rs2 = (instr >> 20) & 0x1F;

        bool uses_rs1 = (opcode != 0x37 && opcode != 0x17 && opcode != 0x6F); // not lui, auipc, jal
        bool uses_rs2 = (opcode == 0x33 || opcode == 0x63 || opcode == 0x23); // R-type, branch, store

        // Stall only for load-use hazard
        if (pipeRegs.id_ex.valid && pipeRegs.id_ex.ctrl.memRead && pipeRegs.id_ex.rd != 0) {
            if ((uses_rs1 && pipeRegs.id_ex.rd == rs1) || (uses_rs2 && pipeRegs.id_ex.rd == rs2)) {
                return true;
            }
        }
        return false;
    }

    // Check if branch is taken
    bool is_branch_taken(PipelineRegisters::ID_EX& id_ex){
        if(id_ex.ctrl.branch){
            if(id_ex.opcode==0x67 || id_ex.opcode==0x6F){
                return true;
            }
            else if(id_ex.opcode==0x63){
                registers[id_ex.rs1] == registers[id_ex.rs2];
            }
        }
        return false;
    }

    // Instruction Fetch
    void fetch(PipelineRegisters& pipeRegs, PipelineRegisters& tempRegs) {
        if (is_stall) {
            // Stall: Do not fetch a new instruction, keep tempRegs.if_id unchanged
            if(!is_nop){
                tempRegs.if_id = pipeRegs.if_id; // Retain the current IF/ID register content
                return;
            }
        }
        if(is_branch){
            tempRegs.if_id.nop=true;
        }
        else{
            tempRegs.if_id.nop=false;
        }

        if (pc / 4 < instrMemory.size()) {
            tempRegs.if_id.pc = pc;
            tempRegs.if_id.instr = instrMemory[pc / 4]; // Fetch new instruction
            tempRegs.if_id.valid = true;
            // cout<<"here in IF  parametres are" <<"    is_branch: " << is_branch<<"    branch_taken: "<<branch_taken<<"    branch_pc: "<<(branch_pc/4)+1<<"   is_nop: " << is_nop <<"   is_stall: " << is_stall<<endl;

            if(is_branch){
                pc = branch_pc;
                is_branch = false;
                branch_taken = false;
            }
            else{
                pc += 4; // Increment program counter
            }

        } 
        else {
            tempRegs.if_id.valid = false; // No more instructions to fetch
        }
    }

    // Instruction Decode 
    void decode(PipelineRegisters& pipeRegs, PipelineRegisters& tempRegs) {
        if (!pipeRegs.if_id.valid) {
            tempRegs.id_ex.valid = false;
            return;
        }
        if (is_stall) {
            tempRegs.id_ex = PipelineRegisters::ID_EX();
            tempRegs.id_ex.valid = true;
            tempRegs.id_ex.stall = true;
            return;
        } else {
            tempRegs.id_ex.stall = false;
        }

        uint32_t instr = pipeRegs.if_id.instr;
        tempRegs.id_ex.pc = pipeRegs.if_id.pc;
        tempRegs.id_ex.opcode = instr & 0x7F;
        tempRegs.id_ex.rd = (instr >> 7) & 0x1F;
        tempRegs.id_ex.rs1 = (instr >> 15) & 0x1F;
        tempRegs.id_ex.rs2 = (instr >> 20) & 0x1F;

        // generateImmediate(instr, tempRegs);
        setControlSignals(instr, tempRegs.id_ex.ctrl, tempRegs.id_ex.opcode);

        // update pc_src based on opcode and branch condition
        if(tempRegs.id_ex.ctrl.branch){
            is_branch = true;
            if(tempRegs.id_ex.opcode==0x63 || tempRegs.id_ex.opcode==0x6F){
                // branch_pc = tempRegs.id_ex.pc + tempRegs.id_ex.imm;
            }
            else if(tempRegs.id_ex.opcode==0x67){
                // branch_pc = registers[tempRegs.id_ex.rs1] + tempRegs.id_ex.imm;
            }
            
            // branch_taken = is_branch_taken(tempRegs.id_ex);
            branch_taken = false;
            if(branch_taken == false){
                branch_pc = tempRegs.id_ex.pc + 4;
            }
        }
        
        // Read register values for forwarding
        // tempRegs.id_ex.rs1_val = registers[tempRegs.id_ex.rs1];
        // tempRegs.id_ex.rs2_val = registers[tempRegs.id_ex.rs2];
        tempRegs.id_ex.valid = true;
    }

    // Execute 
    void execute(PipelineRegisters& pipeRegs, PipelineRegisters& tempRegs) {
        if (!pipeRegs.id_ex.valid) {
            tempRegs.ex_mem.valid = false;
            return;
        }
        else if (pipeRegs.id_ex.stall) {
            tempRegs.ex_mem.valid = true;
            tempRegs.ex_mem.stall = true;
            return;
        }
        else if(!pipeRegs.id_ex.stall){
            tempRegs.ex_mem.stall=false;
        }


        tempRegs.ex_mem.pc = pipeRegs.id_ex.pc;
        tempRegs.ex_mem.rd = pipeRegs.id_ex.rd;
        tempRegs.ex_mem.rs2 = pipeRegs.id_ex.rs2;
        tempRegs.ex_mem.rs2_val = pipeRegs.id_ex.rs2_val;
        tempRegs.ex_mem.ctrl = pipeRegs.id_ex.ctrl;
        tempRegs.ex_mem.alu_result = 0; // Default value
        tempRegs.ex_mem.is_zero = false; // Default value

        uint32_t rs1 = pipeRegs.id_ex.rs1;
        uint32_t rs2 = pipeRegs.id_ex.rs2;

        // Forwarding for operand1
        // uint32_t operand1;
        // if (pipeRegs.ex_mem.valid && pipeRegs.ex_mem.ctrl.regWrite && pipeRegs.ex_mem.rd == rs1 && rs1 != 0) {
        //     operand1 = pipeRegs.ex_mem.alu_result;
        // } else if (pipeRegs.mem_wb.valid && pipeRegs.mem_wb.ctrl.regWrite && pipeRegs.mem_wb.rd == rs1 && rs1 != 0) {
        //     operand1 = (pipeRegs.mem_wb.ctrl.memToReg) ? pipeRegs.mem_wb.mem_data : pipeRegs.mem_wb.alu_result;
        // } else {
        //     operand1 = pipeRegs.id_ex.rs1_val;
        // }

        // Forwarding for operand2
        // uint32_t operand2;
        // if (pipeRegs.id_ex.ctrl.aluSrc) {
        //     operand2 = pipeRegs.id_ex.imm;
        // } else {
        //     if (pipeRegs.ex_mem.valid && pipeRegs.ex_mem.ctrl.regWrite && pipeRegs.ex_mem.rd == rs2 && rs2 != 0) {
        //         operand2 = pipeRegs.ex_mem.alu_result;
        //     } else if (pipeRegs.mem_wb.valid && pipeRegs.mem_wb.ctrl.regWrite && pipeRegs.mem_wb.rd == rs2 && rs2 != 0) {
        //         operand2 = (pipeRegs.mem_wb.ctrl.memToReg) ? pipeRegs.mem_wb.mem_data : pipeRegs.mem_wb.alu_result;
        //     } else {
        //         operand2 = pipeRegs.id_ex.rs2_val;
        //     }
        // }

        // For store instructions, compute rs2_val with forwarding
        // if (pipeRegs.id_ex.ctrl.memWrite) {
        //     uint32_t store_val;
        //     if (pipeRegs.ex_mem.valid && pipeRegs.ex_mem.ctrl.regWrite && pipeRegs.ex_mem.rd == rs2 && rs2 != 0) {
        //         store_val = pipeRegs.ex_mem.alu_result;
        //     } else if (pipeRegs.mem_wb.valid && pipeRegs.mem_wb.ctrl.regWrite && pipeRegs.mem_wb.rd == rs2 && rs2 != 0) {
        //         store_val = (pipeRegs.mem_wb.ctrl.memToReg) ? pipeRegs.mem_wb.mem_data : pipeRegs.mem_wb.alu_result;
        //     } else {
        //         store_val = pipeRegs.id_ex.rs2_val;
        //     }
        //     tempRegs.ex_mem.rs2_val = store_val;
        // } else {
        //     tempRegs.ex_mem.rs2_val = 0; // Not used
        // }

        // ALU operation
        // if (pipeRegs.id_ex.ctrl.aluOp == 0) { // add
        //     tempRegs.ex_mem.alu_result = operand1 + operand2;
        // } else if (pipeRegs.id_ex.ctrl.aluOp == 1) { // beq
        //     tempRegs.ex_mem.is_zero = (operand1 == operand2);
        // }

        tempRegs.ex_mem.valid = true;
    }

    // Memory
    void memory(PipelineRegisters& pipeRegs, PipelineRegisters& tempRegs) {
        if (!pipeRegs.ex_mem.valid) {
            tempRegs.mem_wb.valid = false;
            return;
        }
        else if(pipeRegs.ex_mem.stall){
            tempRegs.mem_wb.valid = true;
            tempRegs.mem_wb.stall=true;
            return;
        }
        else if(!pipeRegs.ex_mem.stall){
            tempRegs.mem_wb.stall=false;
        }
        
        tempRegs.mem_wb.alu_result = pipeRegs.ex_mem.alu_result;
        tempRegs.mem_wb.rd = pipeRegs.ex_mem.rd;
        tempRegs.mem_wb.ctrl = pipeRegs.ex_mem.ctrl;
        tempRegs.mem_wb.pc = pipeRegs.ex_mem.pc;

        if (pipeRegs.ex_mem.ctrl.memRead) {
            // tempRegs.mem_wb.mem_data = dataMemory[pipeRegs.ex_mem.alu_result / 4]; // Word-addressable
        } else if (pipeRegs.ex_mem.ctrl.memWrite) {
            // dataMemory[pipeRegs.ex_mem.alu_result / 4] = pipeRegs.ex_mem.rs2_val;
        }
        tempRegs.mem_wb.valid = true;
    }

    // Write Back
    void writeBack(PipelineRegisters& pipeRegs, PipelineRegisters& tempRegs) {
        if (!pipeRegs.mem_wb.valid) {
            return; // Nothing to write back, no need to update tempRegs.mem_wb
        }
        else if(pipeRegs.mem_wb.stall){
            return;
        }

        if (pipeRegs.mem_wb.ctrl.regWrite) {
            // uint32_t writeData = (pipeRegs.mem_wb.ctrl.memToReg) ? pipeRegs.mem_wb.mem_data : pipeRegs.mem_wb.alu_result;
            if (pipeRegs.mem_wb.rd != 0) { // x0 is hardwired to 0
                // registers[pipeRegs.mem_wb.rd] = writeData;
            }
        }
    }

    // Update pipeline registers
    void updatePipelineRegisters(PipelineRegisters& pipeRegs, PipelineRegisters& tempRegs) {
        
    
        // Update pipeline registers
        pipeRegs.if_id = tempRegs.if_id;
        pipeRegs.id_ex = tempRegs.id_ex;
        pipeRegs.ex_mem = tempRegs.ex_mem;
        pipeRegs.mem_wb = tempRegs.mem_wb;
    
        // Print post-cycle state: pc, instruction number, and tempRegs
        cout << "Post-Cycle State:\n";
        cout << "PC: " << pc/4 +1 << "\n";
        cout << "IF/ID: ";
        if (tempRegs.if_id.valid) {
            cout << (tempRegs.if_id.pc / 4 + 1);
                
        } else {
            cout << "invalid";
        }
        cout << "\nID/EX: ";
        if (tempRegs.id_ex.valid) {
            if (tempRegs.id_ex.stall) {
                cout << "noOp";
            } else {
                cout << (tempRegs.id_ex.pc / 4 + 1);
            }
        } else {
            cout << "invalid";
        }
        cout << "\nEX/MEM: ";
        if (tempRegs.ex_mem.valid) {
            if (tempRegs.ex_mem.stall) {
                cout << "noOp";
            } else {
                cout << (tempRegs.ex_mem.pc / 4 + 1);
            }
        } else {
            cout << "invalid";
        }
        cout << "\nMEM/WB: ";
        if (tempRegs.mem_wb.valid) {
            if (tempRegs.mem_wb.stall) {
                cout << "noOp";
            } else {
                cout << (tempRegs.mem_wb.pc / 4 + 1);
            }
        } else {
            cout << "invalid";
        }
        cout << "\n----------------------------------------\n";
    }

    // print current stages
    void print_current_stages(PipelineRegisters& pipeRegs,std::vector<std::vector<std::string>>& pipeline, std::vector<int>& previous_instruction , int cycle){
        // Print current stage instructions from pipeRegs (before update) and update pipeline diagram
        cout << "Current Stage Instructions (pipeRegs):\n";
        if(pc/4 < instructions.size()){

            if(previous_instruction[0] == pc/4){
                pipeline[cycle][pc / 4] = "-";
            }
            else{
                pipeline[cycle][pc / 4] = "IF";
            }
        }
        
        cout << "IF: ";
        if (pc / 4 < instructions.size()) {
            previous_instruction[0] = pc / 4;
            cout << (pc / 4 + 1);
        } else {
            cout << "invalid";
        }

        if (pipeRegs.if_id.valid && !pipeRegs.if_id.nop) {
            if(previous_instruction[1]==pipeRegs.if_id.pc/4 && pipeRegs.if_id.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.if_id.pc / 4] = "-";
            }
            else if(pipeRegs.if_id.pc/4 < instructions.size() ){
                pipeline[cycle][pipeRegs.if_id.pc / 4] = "ID";
            }
        }
        cout << "\nID: ";
        if (pipeRegs.if_id.valid) {
            if(pipeRegs.if_id.nop){
                previous_instruction[1] = -1;
                cout << "noOp";
            }
            else{
                previous_instruction[1] = (pipeRegs.if_id.pc / 4);
                cout << (pipeRegs.if_id.pc / 4 + 1);
            }
        } else {
            previous_instruction[1] = -1;
            cout << "invalid";
        }
        
        if (pipeRegs.id_ex.valid && !pipeRegs.id_ex.stall) {
            if(previous_instruction[2]==pipeRegs.id_ex.pc/4 && pipeRegs.id_ex.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.id_ex.pc / 4] = "-";
            }
            else if(pipeRegs.id_ex.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.id_ex.pc / 4] = "EX";
            }
        }

        cout << "\nEX: ";
        if (pipeRegs.id_ex.valid) {
            if (pipeRegs.id_ex.stall) {
                previous_instruction[2] = -1;
                cout << "noOp";
            } else {
                previous_instruction[2] = (pipeRegs.id_ex.pc / 4);
                cout << (pipeRegs.id_ex.pc / 4 + 1);
            }
        } else {
            previous_instruction[2] = -1;
            cout << "invalid";
        }
    
        if (pipeRegs.ex_mem.valid && !pipeRegs.ex_mem.stall) {
            if(previous_instruction[3]==pipeRegs.ex_mem.pc/4 && pipeRegs.ex_mem.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.ex_mem.pc / 4] = "-";
            }
            else if(pipeRegs.ex_mem.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.ex_mem.pc / 4] = "MEM";
            }
        }

        cout << "\nMEM: ";
        if (pipeRegs.ex_mem.valid) {
            if (pipeRegs.ex_mem.stall) {
                previous_instruction[3] = -1;
                cout << "noOp";
            } else {
                previous_instruction[3] = (pipeRegs.ex_mem.pc / 4);
                cout << (pipeRegs.ex_mem.pc / 4 + 1);
            }
        } else {
            previous_instruction[3] = -1;
            cout << "invalid";
        }
        
        if(pipeRegs.mem_wb.valid && !pipeRegs.mem_wb.stall){
            if(previous_instruction[4]==pipeRegs.mem_wb.pc/4 && pipeRegs.mem_wb.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.mem_wb.pc / 4] = "-";
            }
            else if(pipeRegs.mem_wb.pc/4 < instructions.size()){
                pipeline[cycle][pipeRegs.mem_wb.pc / 4] = "WB";
            }
        }

        cout << "\nWB: ";
        if (pipeRegs.mem_wb.valid) {
            if (pipeRegs.mem_wb.stall) {
                previous_instruction[4] = -1;
                cout << "noOp";
            } else {
                previous_instruction[4] = (pipeRegs.mem_wb.pc / 4);
                cout << (pipeRegs.mem_wb.pc / 4 + 1);
            }
        } else {
            previous_instruction[4] = -1;
            cout << "invalid";
        }
        cout << "\n";
    }

public:
    ForwardingProcessor(const string& filename) : registers(32, 0), dataMemory(1024, 0) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error opening file: " << filename << endl;
            exit(1);
        }
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string addr, code, instr;
            getline(ss, addr, ':');
            getline(ss, code, ' ');
            ss >> ws;
            getline(ss, instr);
            instrMemory.push_back(stoul(code, nullptr, 16));
            instructions.push_back(instr);
        }
        file.close();
    }

    void simulate(int cycles, ostream& out) {
        vector<vector<string>> pipeline(cycles, vector<string>(instructions.size(), " "));
        PipelineRegisters tempRegs;
        vector<int> previous_instruction(5, -1); // Store previous instruction index for each stage

        for (int cycle = 0; cycle < cycles; cycle++) {
            cout << "Cycle " << cycle+1 << ":\n";
            // Check for stall condition
            is_stall = check_stall(pipeRegs);

            cout << "****Stall: " << is_stall << "****\n";
            cout << "****NOP: " << is_nop << "****\n";


            print_current_stages(pipeRegs, pipeline, previous_instruction, cycle);
            cout<< "branch taken ******************     "<<branch_taken<<endl;

            // Execute stages in reverse order (WB first, IF last)
            writeBack(pipeRegs, tempRegs);
            memory(pipeRegs, tempRegs);
            execute(pipeRegs, tempRegs);
            decode(pipeRegs, tempRegs);
            fetch(pipeRegs, tempRegs);


            // Update pipeline registers
            updatePipelineRegisters(pipeRegs, tempRegs);


            // Check if pipeline is empty
            if (!pipeRegs.if_id.valid && !pipeRegs.id_ex.valid && !pipeRegs.ex_mem.valid && 
                !pipeRegs.mem_wb.valid && pc / 4 >= instrMemory.size()) {
                break;
            }
        }

        for (int instr = 0; instr < instructions.size(); instr++) {
            out << instructions[instr]; // Print the instruction string
            for (int cycle = 0; cycle < pipeline.size(); cycle++) {
                out << ";" << pipeline[cycle][instr];
            }
            out << endl;
        }  
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <cycles>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    int cycles = stoi(argv[2]);
    ofstream outFile("../outputfiles/forward_out.txt");
    if (!outFile.is_open()) {
        cerr << "Error opening output.txt" << endl;
        return 1;
    }

    ForwardingProcessor processor(inputFile);
    processor.simulate(cycles, outFile);

    outFile.close();
    return 0;
}