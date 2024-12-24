#ifndef TOMASULO_H
#define TOMASULO_H

#include <vector>
#include <string>
#include "riscv.h"
class Simulator;
using namespace RISCV;

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

class Tomasulo {
public:
    // Re-Order Buffer (ROB) entry structure
    struct ROBEntry {
        int destination;
        int value;
        bool ready = false;   // Whether the result is ready
        bool busy = false;    // Whether the instruction is under execution
        int remainingExecCycles = 0; 
        Pipe_Op op; //TODO contains duplicate, fix it later
        RISCV::InstType instructionType;
        std::string instStr = "";
        InstructionState state;  
    };

    // Reservation Station (RS) entry structure
    struct ReservationStation {
        std::string op;        // Operation type (e.g., ADD, SUB, LOAD, STORE)
        int vj = 0, vk = 0;   // Values for operands
        int qj = -1, qk = -1; // ROB entry indexes for operands, -1 if value is available
        int dest = -1;         // ROB index for result destination
        bool busy = false;     // Whether the functional unit is busy
    };

    // Register Status Data Structure (RSD) for tracking register usage
    struct RegisterStatus {
        int robIndex = -1;     // ROB entry index for this register
        bool busy = false;     // Whether the register is busy
    };

    std::vector<ROBEntry> rob;              // Re-Order Buffer
    std::vector<ReservationStation> rs;     // Reservation Stations
    std::vector<RegisterStatus> registerStatus; // Register Status Data Structure
    int robHead = 0, robTail = 0;           // Head and tail pointers for ROB
    int numFUs = 4;                        // Number of available functional units (e.g., 4 ALUs)
    int pc = 0;                             // Program Counter

    Tomasulo(int robSize, int rsSize, int regCount);
    ~Tomasulo();

    // Methods for managing the pipeline
    void issue();
    void execute();
    void writeBack();
    void commit();

    // Helper methods
    int allocateROBEntry(InstType opType, int destination);
    int allocateRS(InstType opType, int dest, int qj, int qk);
    void updateRegisterStatus(int regIndex, int robIndex);
    void clearRegisterStatus(int regIndex);
    FunctionUnitType mapInstructionToFU(RISCV::InstType type);
    std::string findAvailableFU(FunctionUnitType f_type);
    bool execArthimetic(Instruction* inst, Simulator* simu);
    bool decode(uint32_t inst, uint64_t* reg, Instruction* score_inst, Simulator* simu);


    // Debugging methods
    void printROB();
    void printRS();
    void printRegisterStatus();

    // Helper functions to handle instruction types
    bool isArithmeticInstruction(const std::string& op);
    bool isLoadInstruction(const std::string& op);
    bool isStoreInstruction(const std::string& op);
};

#endif // TOMASULO_H
