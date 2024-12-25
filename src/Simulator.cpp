#include <iostream>
#include <cstring>
#include <fstream>
#include <string>

#include "Simulator.h"
#include "riscv.h"
#include "Debug.h"
#include "Tomasulo.h"

namespace RISCV {
const char *REGNAME[32] = {
    "zero", // x0
    "ra",   // x1
    "sp",   // x2
    "gp",   // x3
    "tp",   // x4
    "t0",   // x5
    "t1",   // x6
    "t2",   // x7
    "s0",   // x8
    "s1",   // x9
    "a0",   // x10
    "a1",   // x11
    "a2",   // x12
    "a3",   // x13
    "a4",   // x14
    "a5",   // x15
    "a6",   // x16
    "a7",   // x17
    "s2",   // x18
    "s3",   // x19
    "s4",   // x20
    "s5",   // x21
    "s6",   // x22
    "s7",   // x23
    "s8",   // x24
    "s9",   // x25
    "s10",  // x26
    "s11",  // x27
    "t3",   // x28
    "t4",   // x29
    "t5",   // x30
    "t6",   // x31
};

const char *INSTNAME[]{
    "lui",  "auipc", "jal",   "jalr",  "beq",   "bne",  "blt",  "bge",  "bltu",
    "bgeu", "lb",    "lh",    "lw",    "ld",    "lbu",  "lhu",  "sb",   "sh",
    "sw",   "sd",    "addi",  "slti",  "sltiu", "xori", "ori",  "andi", "slli",
    "srli", "srai",  "add",   "sub",   "sll",   "slt",  "sltu", "xor",  "srl",
    "sra",  "or",    "and",   "ecall", "addiw", "mul",  "mulh", "div",  "rem",
    "lwu",  "slliw", "srliw", "sraiw", "addw",  "subw", "sllw", "srlw", "sraw",
};
} // namespace RISCV

using namespace RISCV;

Simulator::Simulator(MemoryManager *memory) {
  this->memory = memory;
  this->pc = 0;
  for (int i = 0; i < REGNUM; ++i) {
    this->reg[i] = 0;
  }
  this->tomasulo = new Tomasulo(5, 9, 32);
}

Simulator::~Simulator() {}

void Simulator::initStack(uint32_t baseaddr, uint32_t maxSize) {
  this->reg[REG_SP] = baseaddr;
  this->stackBase = baseaddr;
  this->maximumStackSize = maxSize;
  for (uint32_t addr = baseaddr; addr > baseaddr - maxSize; addr--) {
    this->memory->setByte(addr, 0);
  }
}

void Simulator::simulate() {
  // Main Simulation Loop
  int cnt = 0;
  while (true) {
    if (this->reg[0] != 0) {
      // Some instruction might set this register to zero
      this->reg[0] = 0;
      // this->panic("Register 0's value is not zero!\n");
    }

    if (this->reg[REG_SP] < this->stackBase - this->maximumStackSize) {
      this->panic("Stack Overflow!\n");
    }
    /* handle branch recoveries */
    if (this->shouldRecoverBranch) {
      if (verbose)
        printf("branch recovery: new pc %08lx\n", this->branchNextPC);

      this->pc = this->branchNextPC;
      this->shouldRecoverBranch = 0;
      this->branchNextPC = 0;

      this->waitForBranch = false;
    }
    // clean data hazard
    this->waitForData = false;
    this->datahazard_execute_op_dest = -1;
    this->datahazard_mem_op_dest = -1;
    this->datahazard_wb_op_dest = -1;

    // THE EXECUTION ORDER of these functions are important!!!
    // Changing them will introduce strange bugs
    this->commit();
    this->writeBack();
    this->execute();
    this->issue();

    saveCycleData(history.cycleCount);
    this->history.cycleCount++;
    this->history.regRecord.push_back(this->getRegInfoStr());
    if (this->history.regRecord.size() >= 100000) { // Avoid using up memory
      this->history.regRecord.clear();
      this->history.instRecord.clear();
    }

    if (verbose) {
      this->printInfo();
    }

    if (this->isSingleStep) {
      // printf("Type d to dump memory in dump.txt, press ENTER to continue: ");
      char ch;
      while ((ch = getchar()) != '\n') {
        if (ch == 'd') {
          this->dumpHistory();
        }
      }
    }
    if (cnt++ > 20) break;
    this->tomasulo->printROB();
    this->tomasulo->printRS();
    this->tomasulo->printRegisterStatus();
  }
  saveSimulationData("simulation.json");
}

Instruction fetchInstruction(uint64_t inst);

void Simulator::issue() {

    uint64_t pc;
    uint32_t inst;

    pc = this->pc;
    inst = memory->getInt(pc);
    Instruction ins;
    bool status = this->tomasulo->decode(inst, this->reg, &ins, this);
    if (!status) {
      dbgprintf("Error!");
      panic("Error");
    }

    InstType instType = ins.opType;        // Example: "ADD", "LW", "SW"
    int rd = ins.destReg;                        // Destination register
    int rs = ins.srcReg1;                        // Source register 1
    int rt = ins.srcReg2;                        // Source register 2 (if applicable)

    // Check for branches or jumps (stall if needed)
    for (auto& entry: tomasulo->rob) {
      // if any jump or branch inst is in rob, do not issue new inst
      if (isJump(entry.inst.opType) || isBranch(entry.inst.opType)) {
        if (entry.busy) {
          return; 
        }
      }
    }

    // Step 1: Allocate ROB Entry
    int robIndex = tomasulo->allocateROBEntry(instType, rd);
    if (robIndex == -1) {
        return; // Stall if ROB is full
    }

    // Step 2: Allocate RS Entry
    int rsIndex = tomasulo->allocateRS(instType, robIndex, -1, -1);
    if (rsIndex == -1) {
        return; // Stall if RS is full
    }

    Tomasulo::ReservationStation& rsTableEntry = tomasulo->rs[rsIndex];

    ins.state = InstructionState::ISSUE;

    // Step 3: Update RS[r] for rs and rt
    if (isIType(instType) || isRType(instType) || isSType(instType) || isBType(instType)) { // If rs is a valid register
        if (tomasulo->registerStatus[rs].busy) {
            int robIndexSrc = tomasulo->registerStatus[rs].robIndex;
            Tomasulo::ROBEntry& robEntry = tomasulo->rob[robIndexSrc];
            if (robEntry.ready) {
                rsTableEntry.vj = robEntry.value;
                rsTableEntry.qj = -1; // Operand is ready
            } else { 
                rsTableEntry.qj = robIndexSrc; // Tag ROB index
            }
        } else {
            rsTableEntry.vj = reg[rs]; // Immediate value from register file
            rsTableEntry.qj = -1;     // Operand is ready
        }
    }

    if (isRType(instType) || isSType(instType) || isBType(instType)) { // If rt is a valid register
        Tomasulo::RegisterStatus& regStatusEntry = tomasulo->registerStatus[rt];
        if (regStatusEntry.busy) {
            int robIndexSrc = regStatusEntry.robIndex;
            Tomasulo::ROBEntry& robEntry = tomasulo->rob[robIndexSrc];
            if (robEntry.ready) {
                rsTableEntry.vk = robEntry.value;
                rsTableEntry.qk = -1; // Operand is ready
            } else {
                rsTableEntry.qk = robIndexSrc; // Tag ROB index
            }
        } else {
            rsTableEntry.vk = reg[rt]; // Immediate value from register file
            rsTableEntry.qk = -1;     // Operand is ready
        }
    }

    // Step 4: Update RS Entry
    rsTableEntry.busy = true;
    rsTableEntry.dest = robIndex; // ROB index for result destination
    rsTableEntry.op = instType;   // Instruction type

    // Step 5: Update ROB Entry
    ins.opType = instType;
    ins.remainingExecCycles = ins.remainingExecCycles;
    tomasulo->rob[robIndex].inst = ins;
    tomasulo->rob[robIndex].ready = false; // Set to true in WriteBack

    // step 7: store immdiate for load / store type
    if (isReadMem(instType)) {
      rsTableEntry.addr = ins.op.offset;
    }

    if (isWriteMem(instType)) {
      rsTableEntry.addr = ins.op.offset;
    }

    // Step 6: if inst contains rd field, register it in register status
    if (isIType(instType) || isRType(instType) || isUType(instType) || isJType(instType)) { // For instructions that have a destination register
        tomasulo->rob[robIndex].destination = rd;
        tomasulo->registerStatus[rd].robIndex = robIndex;
        tomasulo->registerStatus[rd].busy = true;
    }

    pc += 4; // Move to the next instruction
}

void Simulator::execute() {
   for (size_t i = 0; i < tomasulo->rs.size(); ++i) {
        Tomasulo::ReservationStation &currentRS = tomasulo->rs[i];

        // Skip entries that are not busy or already executing
        if (!currentRS.busy) continue;
        int robIndex = currentRS.dest;
        Tomasulo::ROBEntry& robEntry = tomasulo->rob[robIndex];
        // Check if both operands are ready
        if (robEntry.inst.state == InstructionState::ISSUE) {
          // For simplicty, do not consider multiple ALUs or MULs
          robEntry.inst.state = InstructionState::EXECUTE;
        }

        if (robEntry.inst.state == InstructionState::EXECUTE) {
          if (robEntry.inst.remainingExecCycles == 0) {
            if (isReadMem(robEntry.inst.opType)) {
              // check is any store ahead
              if (!tomasulo->hasStoreConflict(robIndex)) {
                robEntry.inst.state = InstructionState::WRITE_BACK;
                tomasulo->execMem(&robEntry.inst, this);
                robEntry.value = robEntry.inst.op.out;
              }
            } else if (isWriteMem(robEntry.inst.opType)) {
                if (currentRS.qj == -1) {
                  robEntry.inst.state = InstructionState::WRITE_BACK;
                  robEntry.addr = currentRS.addr + currentRS.vj;
                }
            } else {
              if (currentRS.qj == -1 && currentRS.qk == -1) {
                robEntry.inst.state = InstructionState::WRITE_BACK;
                tomasulo->execArthimetic(&robEntry.inst, this);
                robEntry.value = robEntry.inst.op.out;
              }
            }
          }
        }
    }
}

void Simulator::writeBack() {
  // Iterate through the Reservation Stations (RS)
  for (size_t i = 0; i < tomasulo->rs.size(); ++i) {
    Tomasulo::ReservationStation &currentRS = tomasulo->rs[i];

    // Skip if the Reservation Station is not busy or has no destination ROB
    // entry
    if (!currentRS.busy || currentRS.dest == -1)
      continue;

    // Check if the corresponding ROB entry is ready
    int robIndex = currentRS.dest;
    Tomasulo::ROBEntry &robEntry = tomasulo->rob[robIndex];

    if (isWriteMem(robEntry.inst.opType)) {
      robEntry.value = currentRS.vk;
      continue;
    }

    // If the ROB entry is ready, write back the result
    if (robEntry.inst.state == InstructionState::WRITE_BACK) {
      // Clear the Reservation Station
      currentRS.busy = false;

      // Forward the result to other instructions waiting on it
      for (auto &rs : tomasulo->rs) {
        if (rs.qj == robIndex) {
          rs.vj = robEntry.value; // Forward the value
          rs.qj = -1;             // Clear dependency
        }
        if (rs.qk == robIndex) {
          rs.vk = robEntry.value; // Forward the value
          rs.qk = -1;             // Clear dependency
        }
      }
    }
  }
}

void Simulator::commit() {
   // Check the instruction at the head of the ROB
    Tomasulo::ROBEntry &headROB = tomasulo->rob[tomasulo->robHead];

    // If the head entry is not busy or not ready, stall commit
    if (!headROB.busy || !headROB.ready) {
        return;
    }

    // Handle commit based on the instruction type
    if (isWriteMem(headROB.inst.opType)) {
        tomasulo->execMem(&headROB.inst, this);
        // For Store, write the value to memory
    } else {
        // For other instructions, write the result to the register file
        int destReg = headROB.destination;
        if (destReg >= 0 && destReg < 32) {
            reg[destReg] = headROB.value;
        }
        // Clear the Register Status Table if this ROB entry is the current register dependency
        if (tomasulo->registerStatus[destReg].robIndex == tomasulo->robHead) {
            tomasulo->registerStatus[destReg].busy = false;
            tomasulo->registerStatus[destReg].robIndex = -1;
        }
    }

    // Mark the ROB entry as no longer busy
    headROB.busy = false;

    // Advance the ROB head pointer (circular buffer logic)
    tomasulo->robHead = (tomasulo->robHead + 1) % tomasulo->rob.size();
}

void Simulator::pipeRecover(uint32_t destPC) {
  /* if there is already a recovery scheduled, it must have come from a later
   * stage (which executes older instructions), hence that recovery overrides
   * our recovery. Simply return in this case. */
  if (this->shouldRecoverBranch)
    return;

  /* schedule the recovery. This will be done once all pipeline stages simulate
   * the current cycle. */
  this->shouldRecoverBranch = 1;
  this->branchNextPC = destPC;
}

int64_t Simulator::handleSystemCall(int64_t op1, int64_t op2) {
  int64_t type = op2; // reg a7
  int64_t arg1 = op1; // reg a0
  switch (type) {
  case 0: { // print string
    uint32_t addr = arg1;
    char ch = this->memory->getByte(addr);
    while (ch != '\0') {
      printf("%c", ch);
      ch = this->memory->getByte(++addr);
    }
    break;
  }
  case 1: // print char
    printf("%c", (char)arg1);
    break;
  case 2: // print num
    printf("%d", (int32_t)arg1);
    break;
  case 3:
  case 93: // exit
    printf("Program exit from an exit() system call\n");
    if (shouldDumpHistory) {
      printf("Dumping history to dump.txt...");
      this->dumpHistory();
    }
    this->printStatistics();
    exit(0);
  case 4: // read char
    scanf(" %c", (char *)&op1);
    break;
  case 5: // read num
    scanf(" %ld", &op1);
    break;
  default:
    this->panic("Unknown syscall type %d\n", type);
  }
  return op1;
}

void Simulator::printInfo() {
  printf("------------ CPU STATE ------------\n");
  printf("PC: 0x%lx\n", this->pc);
  for (uint32_t i = 0; i < 32; ++i) {
    printf("%s: 0x%.8lx(%ld) ", REGNAME[i], this->reg[i], this->reg[i]);
    if (i % 4 == 3)
      printf("\n");
  }
  printf("-----------------------------------\n");
}

void Simulator::printStatistics() {
  printf("------------ STATISTICS -----------\n");
  printf("Number of Instructions: %u\n", this->history.instCount);
  printf("Number of Cycles: %u\n", this->history.cycleCount);
  printf("Avg Cycles per Instrcution: %.4f\n",
         (float)this->history.cycleCount / this->history.instCount);
  printf("Number of Control Hazards: %u\n", this->history.controlHazardCount);
  printf("Number of Data Hazards: %u\n", this->history.dataHazardCount);
  printf("-----------------------------------\n");
}

std::string Simulator::getRegInfoStr() {
  std::string str;
  char buf[65536];

  str += "------------ CPU STATE ------------\n";
  sprintf(buf, "PC: 0x%lx\n", this->pc);
  str += buf;
  for (uint32_t i = 0; i < 32; ++i) {
    sprintf(buf, "%s: 0x%.8lx(%ld) ", REGNAME[i], this->reg[i], this->reg[i]);
    str += buf;
    if (i % 4 == 3) {
      str += "\n";
    }
  }
  str += "-----------------------------------\n";

  return str;
}

void Simulator::dumpHistory() {
  std::ofstream ofile("dump.txt");
  ofile << "================== Excecution History =================="
        << std::endl;
  for (uint32_t i = 0; i < this->history.instRecord.size(); ++i) {
    ofile << this->history.instRecord[i];
    ofile << this->history.regRecord[i];
  }
  ofile << "========================================================"
        << std::endl;
  ofile << std::endl;

  ofile.close();
}

void Simulator::panic(const char *format, ...) {
  saveSimulationData("simulation.json");
  char buf[BUFSIZ];
  va_list args;
  va_start(args, format);
  vsprintf(buf, format, args);
  fprintf(stderr, "%s", buf);
  va_end(args);
  this->dumpHistory();
  fprintf(stderr, "Execution history in dump.txt\n");
  exit(-1);
}

using json = nlohmann::json;

// Convert Instruction to JSON
void to_json(json& j, const Instruction& inst) {
    j = json{
        {"pc", inst.pc},
        {"destReg", inst.destReg},
        {"srcReg1", inst.srcReg1},
        {"srcReg2", inst.srcReg2},
        {"state", static_cast<int>(inst.state)},
        {"remainingExecCycles", inst.remainingExecCycles},
        {"opType", static_cast<int>(inst.opType)},
        {"inst", inst.inst},
        {"processingUnit", inst.processingUnit},
        {"instStr", inst.instStr}
    };
}

// Convert ROBEntry to JSON
void to_json(json& j, const Tomasulo::ROBEntry& entry) {
    j = json{
        {"destination", entry.destination},
        {"value", entry.value},
        {"ready", entry.ready},
        {"busy", entry.busy},
        {"addr", entry.addr},
        {"inst", entry.inst}
    };
}

// Convert ReservationStation to JSON
void to_json(json& j, const Tomasulo::ReservationStation& rs) {
    j = json{
        {"op", static_cast<int>(rs.op)},
        {"vj", rs.vj},
        {"vk", rs.vk},
        {"qj", rs.qj},
        {"qk", rs.qk},
        {"dest", rs.dest},
        {"busy", rs.busy},
        {"addr", rs.addr}
    };
}

// Convert RegisterStatus to JSON
void to_json(json& j, const Tomasulo::RegisterStatus& rs) {
    j = json{
        {"robIndex", rs.robIndex},
        {"busy", rs.busy}
    };
}

void Simulator::saveCycleData(int currentCycle) {
  json cycleJson;
  // Add current cycle number
  cycleJson["cycle"] = currentCycle;
  // Serialize ROB
  cycleJson["rob"] = tomasulo->rob;
  // Serialize Reservation Stations
  cycleJson["rs"] = tomasulo->rs;
  // Serialize Register Status
  cycleJson["registerStatus"] = tomasulo->registerStatus;
  // Add register values
  cycleJson["reg"] = std::vector<uint64_t>(std::begin(reg), std::end(reg));
  // Append to simulationData
  simulationData["cycles"].push_back(cycleJson);
}

void Simulator::saveSimulationData(const std::string& filename) const {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << simulationData.dump(4); // Pretty-print with 4-space indentation
        file.close();
        std::cout << "Simulation data saved to " << filename << std::endl;
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}
