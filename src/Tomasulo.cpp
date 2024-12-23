// Tomasulo.cpp
#include "Tomasulo.h"

Tomasulo::Tomasulo(int robSize, int rsSize, int regCount) : RSD(regCount, {-1}) {
    ROB.resize(robSize);
    RS.resize(rsSize);
}

int Tomasulo::allocateROBEntry(const std::string& instructionType, int destination) {
    if (ROB[ROBTail].busy) return -1; // ROB is full

    ROB[ROBTail] = {instructionType, destination, 0, false, true};
    int allocatedIndex = ROBTail;
    ROBTail = (ROBTail + 1) % ROB.size();
    return allocatedIndex;
}

void Tomasulo::updateROBEntry(int index, int value) {
    ROB[index].value = value;
    ROB[index].ready = true;
}

bool Tomasulo::commitROBEntry() {
    if (!ROB[ROBHead].busy || !ROB[ROBHead].ready) return false;

    ROB[ROBHead].busy = false;
    ROBHead = (ROBHead + 1) % ROB.size();
    return true;
}

int Tomasulo::allocateRS(const std::string& op, int dest, int qj, int qk) {
    for (int i = 0; i < RS.size(); ++i) {
        if (!RS[i].busy) {
            RS[i] = {op, 0, 0, qj, qk, dest, true};
            return i;
        }
    }
    return -1; // No free reservation station
}

void Tomasulo::updateRSOperands(int robIndex, int value) {
    for (auto& rs : RS) {
        if (rs.qj == robIndex) {
            rs.qj = -1;
            rs.vj = value;
        }
        if (rs.qk == robIndex) {
            rs.qk = -1;
            rs.vk = value;
        }
    }
}

void Tomasulo::updateRegisterStatus(int regIndex, int robIndex) {
    RSD[regIndex].robIndex = robIndex;
}

void Tomasulo::clearRegisterStatus(int regIndex) {
    RSD[regIndex].robIndex = -1;
}

int Tomasulo::getRegisterStatus(int regIndex) {
    return RSD[regIndex].robIndex;
}

void Tomasulo::printROB() {
    std::cout << "ROB:\n";
    for (const auto& entry : ROB) {
        std::cout << "Type: " << entry.instructionType
                  << ", Dest: " << entry.destination
                  << ", Value: " << entry.value
                  << ", Ready: " << entry.ready
                  << ", Busy: " << entry.busy << "\n";
    }
}

void Tomasulo::printRS() {
    std::cout << "Reservation Stations:\n";
    for (const auto& rs : RS) {
        std::cout << "Op: " << rs.op
                  << ", Vj: " << rs.vj
                  << ", Vk: " << rs.vk
                  << ", Qj: " << rs.qj
                  << ", Qk: " << rs.qk
                  << ", Dest: " << rs.dest
                  << ", Busy: " << rs.busy << "\n";
    }
}

void Tomasulo::printRSD() {
    std::cout << "Register Status:\n";
    for (int i = 0; i < RSD.size(); ++i) {
        std::cout << "Reg " << i << ": ROB Index: " << RSD[i].robIndex << "\n";
    }
}