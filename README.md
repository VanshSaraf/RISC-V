# COL216_Assignment2

# RISC-V Pipeline Processor Simulator

This project implements a RISC-V pipeline processor simulator in C++ with support for both forwarding and no-forwarding execution modes.

## Architecture Overview

The simulator implements a classic 5-stage pipeline:
1. **Instruction Fetch (IF)**: Fetches instructions from memory
2. **Instruction Decode (ID)**: Decodes instructions and reads registers
3. **Execute (EX)**: Performs ALU operations
4. **Memory (MEM)**: Performs memory operations (load/store)
5. **Write Back (WB)**: Writes results back to registers

## Features

### Supported RISC-V Instructions
- **R-Type Instructions**: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA
- **I-Type Instructions**: ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI, Load instructions (LW)
- **S-Type Instructions**: Store instructions (SW)
- **B-Type Instructions**: BEQ, BNE, BLT, BGE, BLTU, BGEU
- **J-Type Instructions**: JAL
- **Other Instructions**: JALR

### Pipeline Hazard Handling
- **Data Hazards**: Detected and resolved using stalls (in NoForwardingProcessor) or data forwarding (in ForwardingProcessor)
- **Control Hazards**: Handled by flushing the pipeline and inserting NOPs when branch instructions are executed
- **Structural Hazards**: Minimal due to separate instruction and data memories

### Execution Modes
1. **No Forwarding Mode**: Data hazards are resolved using pipeline stalls
2. **Forwarding Mode**: Data hazards are minimized using register value forwarding

### Visualization
- Provides detailed cycle-by-cycle pipeline state visualization
- Generates a pipeline diagram showing the progress of each instruction through the pipeline stages

## Implementation Details
We are not implementing the actual ripes simulator i.e. tracking register values, deciding branches on the basis of register values, jumping to different instructions. Assumption made by us is whenever we get a branch or jump instruction we should have a stall of one cycle after that because it is given that branches are resolved in ID stage. We assume that after branch/jump is resolved it will always be not taken.

### Data Structures

#### Control Signals
```cpp
struct ControlSignals {
    bool regWrite;  // Register write enable
    bool memRead;   // Memory read enable
    bool memWrite;  // Memory write enable
    int aluSrc;     // ALU source selector (0: register, 1: immediate)
    int aluOp;      // ALU operation type
    int memToReg;   // Memory to register selector (0: ALU result, 1: memory data)
    int branch;     // Branch control signal
};
```

#### Pipeline Registers
The processor uses pipeline registers to hold data between stages:
- IF/ID: Holds fetched instruction and PC
- ID/EX: Holds decoded instruction data, register values, and control signals
- EX/MEM: Holds ALU results, branch conditions, and memory operation control
- MEM/WB: Holds memory read data and register write back data

### ALU Operations
The Execute stage supports a wide range of ALU operations for different instruction types:
- Arithmetic operations (ADD, SUB)
- Logical operations (AND, OR, XOR)
- Shift operations (SLL, SRL, SRA)
- Comparison operations (SLT, SLTU)

### Memory System
The processor uses a simplified memory model:
- Word-addressable instruction memory
- Word-addressable data memory

### Branch Handling
Branches are resolved in the Execute stage with the following strategy:
1. Branch target address is calculated in Decode stage
2. Branch condition is evaluated in Execute stage
3. If branch is taken, the pipeline is flushed and the PC is updated

### Stall and Forwarding Logic
- **NoForwardingProcessor**: Implements stall detection for RAW hazards across any pipeline stage
- **ForwardingProcessor**: Implements data forwarding from EX/MEM and MEM/WB stages to minimize stalls

## Usage

### Input Format
Input files should contain RISC-V instructions in the following format:
```
address:machine_code instruction_text
```
Example:
```
0:0001a083 lw x1 0 x3
4:00208063 beq x1 x2 0
```

### Running the Simulator
```bash
./noforward <input_file> <cycles>
./forward <input_file> <cycles>
```
Where:
- `input_file` is the path to the input file containing instructions
- `cycles` is the maximum number of cycles to simulate

### Output Format
The simulator generates an output file showing the pipeline stages each instruction passes through:
```
instruction_text;cycle1;cycle2;...
```
Where each cycle entry is one of:
- `IF`: Instruction Fetch
- `ID`: Instruction Decode
- `EX`: Execute
- `MEM`: Memory
- `WB`: Write Back
- `-`: Stalled
- ` `: Not in pipeline

## Future Improvements
- Cache simulation with miss penalties
- Branch prediction
- Out-of-order execution
- Support for floating-point instructions
- Support for system calls and exceptions