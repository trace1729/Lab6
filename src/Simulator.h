#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <cstdarg>
#include <cstdint>
#include <ratio>
#include <string>
#include <vector>

#include "MemoryManager.h"
#include "riscv.h"
#include <nlohmann/json.hpp>

class Scoreboard;

class Simulator {
public:
  bool isSingleStep;
  bool verbose;
  bool shouldDumpHistory;
  uint64_t pc;
  uint64_t reg[RISCV::REGNUM];
  uint32_t stackBase;
  uint32_t maximumStackSize;
  MemoryManager *memory;
  Scoreboard* scoreboard;

  Simulator(MemoryManager *memory);
  ~Simulator();

  void initStack(uint32_t baseaddr, uint32_t maxSize);

  void simulate();

  void dumpHistory();

  void printInfo();

  void printStatistics();

  Pipe_Op *decode_op, *execute_op, *mem_op, *wb_op;
  // control hazard
  bool waitForBranch; // signal for fetch stage to stall, and need to turn to false when shouldRecoverBranch
  bool shouldRecoverBranch;
  int64_t branchNextPC; 
  // data hazard
  bool waitForData; // only when no waitForBranch will set, and every cycle to detect
  RISCV::RegId datahazard_execute_op_dest;
  RISCV::RegId datahazard_mem_op_dest;
  RISCV::RegId datahazard_wb_op_dest;

  struct History {
    uint32_t instCount;
    uint32_t cycleCount;

    uint32_t dataHazardCount;
    uint32_t controlHazardCount;
    uint32_t memoryHazardCount;

    std::vector<std::string> instRecord;
    std::vector<std::string> regRecord;
  } history;

  void fetch();
  void decode();
  void execute();
  void memoryAccess();
  void writeBack();

  void issue();
  void commit();
  // Other members...

  nlohmann::json simulationData; // JSON array to store cycle data

  Simulator();
  void saveCycleData(int currentCycle); // Save data for the current cycle
  void saveSimulationData(const std::string& filename) const; // Write all data to file

  void pipeRecover(uint32_t destPC); // record jump pc and update pc next cycle
  // void detectDataHazard(RISCV::RegId destReg); //banned
  int64_t handleSystemCall(int64_t op1, int64_t op2);

  std::string getRegInfoStr();
  void panic(const char *format, ...);
};

#endif
