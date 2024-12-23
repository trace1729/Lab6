// Tomasulo.h
#ifndef TOMASULO_H
#define TOMASULO_H

#include <iostream>
#include <vector>
#include <string>
#include <queue>

// Re-Order Buffer (ROB)
struct ROBEntry {
    std::string instructionType;
    int destination;
    int value;
    bool ready = false;
    bool busy = false;
};

// Reservation Station (RS)
struct ReservationStation {
    std::string op;
    int vj = 0, vk = 0;
    int qj = -1, qk = -1; // -1 indicates the value is ready
    int dest = -1;        // ROB index
    bool busy = false;
};

// Register Status Data Structure (RSD)
struct RegisterStatus {
    int robIndex = -1; // -1 indicates the register value is ready
};

class Tomasulo {
public:
    std::vector<ROBEntry> ROB;
    int ROBHead = 0, ROBTail = 0;
    
    std::vector<ReservationStation> RS;
    std::vector<RegisterStatus> RSD;

    Tomasulo(int robSize, int rsSize, int regCount);

    // Methods for ROB
    int allocateROBEntry(const std::string& instructionType, int destination);
    void updateROBEntry(int index, int value);
    bool commitROBEntry();

    // Methods for Reservation Stations
    int allocateRS(const std::string& op, int dest, int qj, int qk);
    void updateRSOperands(int robIndex, int value);

    // Methods for Register Status
    void updateRegisterStatus(int regIndex, int robIndex);
    void clearRegisterStatus(int regIndex);
    int getRegisterStatus(int regIndex);

    // Debugging Utilities
    void printROB();
    void printRS();
    void printRSD();
};

#endif // TOMASULO_H