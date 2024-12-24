#include "tomasulo.h"
#include <iostream>
#include <string>
#include <vector>

Tomasulo::Tomasulo(int robSize, int rsSize, int regCount) : 
    rob(robSize), rs(rsSize), registerStatus(regCount) {}

Tomasulo::~Tomasulo() {}

int Tomasulo::allocateROBEntry(const std::string& instType, int destination) {
    // Try to allocate an entry in the ROB
    if (rob[robTail].busy) return -1; // ROB is full
    rob[robTail].busy = true;
    rob[robTail].instructionType = instType;
    rob[robTail].destination = destination;
    rob[robTail].ready = false;
    int allocatedIndex = robTail;
    robTail = (robTail + 1) % rob.size();
    return allocatedIndex;
}

int Tomasulo::allocateRS(const std::string& op, int dest, int qj, int qk) {
    for (int i = 0; i < rs.size(); ++i) {
        if (!rs[i].busy) {
            rs[i] = {op, 0, 0, qj, qk, dest, true};
            return i;
        }
    }
    return -1; // No free reservation station
}

void Tomasulo::updateRegisterStatus(int regIndex, int robIndex) {
    registerStatus[regIndex].robIndex = robIndex;
    registerStatus[regIndex].busy = true;
}

void Tomasulo::clearRegisterStatus(int regIndex) {
    registerStatus[regIndex].robIndex = -1;
    registerStatus[regIndex].busy = false;
}

void Tomasulo::issue() {
    // Fetch the next instruction (simulated by incrementing the program counter)
    // Decode the instruction and allocate ROB and RS entries

    this->pc += 4; // Simulate instruction fetch
    // Example, assume we have an ADD instruction
    std::string instType = "ADD";
    int rd = 2; // destination register for the ADD instruction
    int rs1 = 3, rs2 = 4; // source registers for the ADD instruction

    // Try allocating ROB and RS entries
    int robIndex = allocateROBEntry(instType, rd);
    if (robIndex == -1) return; // ROB is full

    int rsIndex = allocateRS(instType, robIndex, -1, -1);
    if (rsIndex == -1) return; // RS is full

    // Handle operands for source registers
    if (registerStatus[rs1].busy) {
        int robIndexSrc1 = registerStatus[rs1].robIndex;
        rs[rsIndex].qj = robIndexSrc1;
    } else {
        rs[rsIndex].vj = reg[rs1];
        rs[rsIndex].qj = -1;
    }

    if (registerStatus[rs2].busy) {
        int robIndexSrc2 = registerStatus[rs2].robIndex;
        rs[rsIndex].qk = robIndexSrc2;
    } else {
        rs[rsIndex].vk = reg[rs2];
        rs[rsIndex].qk = -1;
    }

    // Update RS and ROB accordingly
    rob[robIndex].instructionType = instType;
    rob[robIndex].destination = rd;
    rob[robIndex].ready = false;
}

void Tomasulo::execute() {
    for (int i = 0; i < rs.size(); ++i) {
        if (!rs[i].busy) continue;

        if (rs[i].qj == -1 && rs[i].qk == -1) {
            // Operands are ready, perform the operation
            int result = 0;
            if (rs[i].op == "ADD") {
                result = rs[i].vj + rs[i].vk;
            } else if (rs[i].op == "SUB") {
                result = rs[i].vj - rs[i].vk;
            }
            // Update the result in the ROB
            rob[rs[i].dest].value = result;
            rob[rs[i].dest].ready = true;

            // Clear the RS entry
            rs[i].busy = false;
        }
    }
}

void Tomasulo::writeBack() {
    // Iterate over the RS and check if any instruction finished executing
    for (int i = 0; i < rs.size(); ++i) {
        if (!rs[i].busy && rs[i].dest != -1) {
            // Get corresponding ROB entry
            int robIndex = rs[i].dest;
            rob[robIndex].value = rs[i].vj; // or rs[i].vk based on the type of instruction
            rob[robIndex].ready = true;
        }
    }
}

void Tomasulo::commit() {
    // Commit the instruction at the head of the ROB
    if (!rob[robHead].ready) return; // The instruction is not ready yet to commit

    // Write result to the register file or memory
    int dest = rob[robHead].destination;
    reg[dest] = rob[robHead].value;  // Write back to register file

    // Clear the ROB entry after commit
    rob[robHead].busy = false;
    robHead = (robHead + 1) % rob.size();
}

void Tomasulo::printROB() {
    std::cout << "Reorder Buffer (ROB):\n";
    for (auto& entry : rob) {
        std::cout << "Type: " << entry.instructionType
                  << ", Dest: " << entry.destination
                  << ", Ready: " << entry.ready
                  << ", Value: " << entry.value
                  << ", Busy: " << entry.busy << "\n";
    }
}

void Tomasulo::printRS() {
    std::cout << "Reservation Stations (RS):\n";
    for (auto& station : rs) {
        std::cout << "Op: " << station.op
                  << ", Vj: " << station.vj
                  << ", Vk: " << station.vk
                  << ", Qj: " << station.qj
                  << ", Qk: " << station.qk
                  << ", Dest: " << station.dest
                  << ", Busy: " << station.busy << "\n";
    }
}

void Tomasulo::printRegisterStatus() {
    std::cout << "Register Status:\n";
    for (int i = 0; i < registerStatus.size(); ++i) {
        std::cout << "Reg " << i << ": ROB Index: " << registerStatus[i].robIndex << ", Busy: " << registerStatus[i].busy << "\n";
    }
}

bool Tomasulo::isArithmeticInstruction(const std::string& op) {
    return (op == "ADD" || op == "SUB");
}

bool Tomasulo::isLoadInstruction(const std::string& op) {
    return (op == "LW" || op == "LD");
}

bool Tomasulo::isStoreInstruction(const std::string& op) {
    return (op == "SW" || op == "SD");
}
