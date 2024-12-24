#include <iostream>
#include <cstring>
#include <fstream>
#include <string>

#include "Scoreboard.h"
#include "Simulator.h"
#include "riscv.h"
#include "Debug.h"

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
    // this->writeBack();
    // this->execute();
    // this->read_operand();
    // this->issue();

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
    if (cnt++ > 20) return;
    std::cout << this->scoreboard->toString() << std::endl;
  }
  // saveSimulationData("simulation.json");
}

void Simulator::issue() {
  
  // update pc
  this->pc += 4;
}


void Simulator::execute() {
  
}

void Simulator::writeBack() {
  
}

void Simulator::commit() {
  
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

// void Simulator::saveCycleData(int currentCycle) {
//     json cycleJson;

//     // Add current cycle number
//     cycleJson["cycle"] = currentCycle;

//     cycleJson["scoreboard"] = this->scoreboard->toJson();

//     // Add register values
//     cycleJson["reg"] = std::vector<uint64_t>(std::begin(reg), std::end(reg));

//     // Append to simulationData
//     simulationData["cycles"].push_back(cycleJson);
// }

// void Simulator::saveSimulationData(const std::string& filename) const {
//     std::ofstream file(filename);
//     if (file.is_open()) {
//         file << simulationData.dump(4); // Pretty-print with 4-space indentation
//         file.close();
//         std::cout << "Simulation data saved to " << filename << std::endl;
//     } else {
//         std::cerr << "Failed to open file: " << filename << std::endl;
//     }
// }
