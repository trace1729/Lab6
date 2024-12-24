#include <iostream>
#include <string>
#include <vector>
#include "Tomasulo.h"

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
            rs[i].op = op;
            rs[i].vj = 0;
            rs[i].vk = 0;
            rs[i].qj = qj;
            rs[i].qj = qk;
            rs[i].dest = dest;
            rs[i].busy = true;
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
    
}

void Tomasulo::execute() {
    
}

void Tomasulo::writeBack() {
    
}

void Tomasulo::commit() {
    
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
