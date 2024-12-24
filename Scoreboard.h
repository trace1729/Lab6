#ifndef SIMULATOR_SCOREBOARD_H
#define SIMULATOR_SCOREBOARD_H

#include <vector>
#include <unordered_map>
#include <string>
#include "riscv.h"
#include <nlohmann/json.hpp>

// Instruction states
enum class InstructionState { STALL, ISSUE, READ_OPERANDS, EXECUTE, WRITE_BACK, FINNISH};
enum class FunctionUnitType { NONE, ALU, MUL, MEM};

// Instruction structure
struct Instruction {
    int pc;            
    int destReg;       // Destination register (Fi)
    int srcReg1;       // Source register 1 (Fj)
    int srcReg2;       // Source register 2 (Fk)
    InstructionState state;  
    int remainingExecCycles = 0;  
    RISCV::InstType opType;
    uint32_t inst;
    std::string processingUnit = "";
    Pipe_Op op; //TODO contains duplicate, fix it later
    std::string instStr = "";
};

// Functional Unit structure
struct FunctionalUnit {
    bool busy = false;  // Is the FU busy?
    RISCV::InstType op;     // Operation being performed
    int fi = -1;        // Destination register
    int fj = -1;        // Source register 1
    int fk = -1;        // Source register 2
    std::string qj = "";  // FU producing Fj
    std::string qk = "";  // FU producing Fk
    bool rj = true;       // Is Fj ready?
    bool rk = true;       // Is Fk ready?
};

class Simulator;

// Scoreboard structure
class Scoreboard {
public:
    std::unordered_map<std::string, FunctionalUnit> functionalUnits;  // Map FU names to units
    std::unordered_map<int, std::string> registerResultStatus;        // Register FU mapping
    std::vector<Instruction> instructionQueue;                        // Track instructions in the pipeline
    
    Scoreboard();
    ~Scoreboard();

    FunctionUnitType mapInstructionToFU(RISCV::InstType type);
    std::string findAvailableFU(FunctionUnitType f_type);

    bool decode(uint32_t inst, uint64_t* reg, Instruction* score_inst, Simulator* simu); 
    bool exec(std::string funit, Instruction* inst, Simulator* simu);
    bool execMem(Instruction* inst, Simulator* simu);
    bool execArthimetic(std::string funit, Instruction* inst, Simulator* simu);
    void writeBack(std::string funit);
    bool checkWAR(int destReg);
    std::string toString() const;
    nlohmann::json toJson() const;
};


#endif