#import "template.typ": *

#show: project.with(
  title: "Laboratory 5",
  authors: (
    (
      name: "龚开宸",
      email: "gongkch2024@shanghaitech.edu.cn"
    ),
  ),
)

= The whole process

using the scoreboard to control the whole exection process

- fetch instruction using pc
- using scoreboard to check dependency
 - if no collsion, issue the inst and mark the correponding flags in function unit. 
  - (how to design api to map inst to function unit: 4 add units, 4 mul units, 1 mem unit)
  - need to understand what each flag does
    - Busy: Indicates whether the unit is busy or not
    - Op: Operation to perform in the unit (e.g., + or –)
    - Fi: Destination register
    - Fj Fk: Source-register numbers
    - Qj, Qk: Functional units producing source registers Fj, Fk (record whether source register depends on the previous FU output)
    - Rj, Rk: Flags indicating when Fj, Fk are ready
- how to keep track of the whole process of issued instrucion (in which states) using data structure like queue


= Scoreboard integrity design

The scoreboard contains three data structrues, for the detail of each data structure, refer to the spec
1. instruction unit
2. functional unit status
3. register result table

I will list what each stage does in every cycle.
- Issue(ID1) stage
 - First, there should be a `queue` called `instruction status` recorded instrucion being executed and their current stage (ISSUE, READ_OPERAND, EXECUTE, WB), which can be use to detect whether any branch instruction is in execution. skip issue if exists to avoid control hazard.
 - Next, fetch inst based on the pc, decode the inst and check the status of the `functional unit status` in scoreboard (How to check will be listed as follows).
  - dispatch function unit for the inst (ALU, MEM or MUL)
   - noticed we have 4 ALU units and 4 MUL units which means four inst that need ALU can run in parallel, and to properly handle this, we needs to properly record which ALU or MUL has been occupied, perhaps using queue to record, if any better work around, please correct me.
   - make sure the rd of the inst and no other active instrucion has the same destination register to avoid WAW by checking `register result table`.
   - if the above condition is satisfied, we could issue the inst
    - add inst to `instruction status` and mark its condition as ISSUE.
    - marks the allocated FU as busy in `functional unit status`.
    - mark rd in `register result status table` (used to record rd register value comes from which FU)
 
```cpp

void Simulator::issue() {
  // 1. check whether `scoreboard->instructionQueue` contains branch instruction
  //    stall the pipeline if find any.

  for (auto inst : this->scoreboard->instructionQueue) {
    if (isBranch(inst.opType) || isJump(inst.opType)) {
      if (this->scoreboard->instructionQueue.size() > 1) {
        dbgprintf("stall due to control hazard.\n");
        return;
      }
    }
  }

  // Update. if we have any stall instruction
  for (auto& inst: this->scoreboard->instructionQueue) {
    if (inst.state == InstructionState::STALL) {
      bool success = this->scoreboard->decode(inst.inst, this->reg, &inst);
      if (!success) {
        dbgprintf("stall due to occupied function unit or WAW hazards.\n");
        return; 
      }
      inst.state = InstructionState::ISSUE;
      this->pc += 4;
      return;
    }
  }

  // 2. fetch inst based on pc

  uint64_t pc;
  uint32_t inst;

  pc = this->pc;
  inst = memory->getInt(pc);

  // 3. do decode
  //    check whether
  Instruction instruction = Instruction();
  instruction.pc = pc;
  instruction.inst = inst;
  bool success = this->scoreboard->decode(inst, this->reg, &instruction);
  if (!success) {
    instruction.state = InstructionState::STALL;
    this->scoreboard->instructionQueue.push_back(instruction);
    dbgprintf("stall due to occupied function unit or WAW hazards.\n");
    return; 
  }

  instruction.state = InstructionState::ISSUE;
  this->scoreboard->instructionQueue.push_back(instruction);

  // update pc
  this->pc += 4;
}
```

- read operand (ID2) stage
  - iterate instruction queue, for the first inst that in ISSUE stage (because we add inst to scoreboard sequentially, I think this will help statisfy in order issue contraints, If my thoughts wrong, correct me.)
   - record Fj and Fk (src1 and src2) in `functional unit status`
   - check whether the source registers is ready (don't have correspond FU in `register result table`)
    - if not ready mark, Qj, Qk in `functional unit status` by looking up `register result table`.
   - if condition satisfies, change the instruction from ISSUE to READ_OPERAND in `instruction status`

```cpp
void Simulator::read_operand() {
  
  Instruction* inst = nullptr;

  for (auto& ins: this->scoreboard->instructionQueue) {
    if (ins.state == InstructionState::ISSUE) {
      inst = & ins;
      break;
    }
  }

  if (inst == nullptr) {
    return;
  }

  // bind inst with dispatched function unit
  std::string funit = inst->processingUnit;
  FunctionalUnit& funcEntry = this->scoreboard->functionalUnits[funit];

  // avoid RAW hazard
  if (funcEntry.qj == "" && funcEntry.qk == "") {
    // what does these do?
      // avoid WAR hazard in write stage
    funcEntry.rj = false;
    funcEntry.rk = false;
    inst->state = InstructionState::READ_OPERANDS;
  } else {
    dbgprintf("stall due to data hazard\n");
  }
  
}

```

- EX stage
  - iterate instruction queue, for all inst that in EX stage, update their execution states, if FU unit finnish executing.
    - ALU MEM finsih exection instantly.
    - MUL takes 5 cycle to finish.
  - iterate instruction queue, for the first inst that in READ_OPERAND stage, begin execution. (because we add inst to scoreboard sequentially, I think this will avoid in order issue contraints, If my thoughts was wrong, correct me.)
  
```cpp
void Simulator::execute() {
  for (auto& ins: this->scoreboard->instructionQueue) {
    if (ins.state == InstructionState::EXECUTE) {
      if (ins.remainingExecCycles > 0) {
        ins.remainingExecCycles--;
      } else {
        // using scoreboard to do calculate
        scoreboard->exec(ins.processingUnit, &ins, this);
        ins.state = InstructionState::WRITE_BACK;
      }
    }
    if (ins.state == InstructionState::READ_OPERANDS) {
      ins.state = InstructionState::EXECUTE;
    }
  }
}

```

- WB stage
  - iterate instruction queue, for all inst that in EX stage, use scoreboard to tell whether any instruction finnish execution
   - if find any, then check the `function unit status` to find any instruction are reading the register that now trying to write to
    - we must wait after reading is done to solve WAR.
   - after solving collsion, do write back
    - update `register result table` (clear FU)
    - update `function unit status` flag (set Fi, Fj flag) 

```cpp
void Simulator::writeBack() {
  for (auto& ins: this->scoreboard->instructionQueue) {
    if (ins.state == InstructionState::WRITE_BACK) {
      // using scoreboard to tell wether register the scoreboard trying to write, is going to read by any other instruction.     
      bool isWAR = scoreboard->checkWAR(ins.destReg);
      if (isWAR == false) {
        if (ins.op.writeReg) {
          this->reg[ins.destReg] = ins.op.out;
          dbgprintf("R[%d] = %x\n", ins.destReg, ins.op.out);
        }
        scoreboard->writeBack(ins.processingUnit);
        ins.state = InstructionState::FINNISH;
      } else {
        dbgprintf("stall due to WAR hazard for %d\n", ins.destReg);
      }
    }
  }

  std::vector<Instruction> new_queue;

  // remove all the finnish inst and build new vector
  for (auto ins: this->scoreboard->instructionQueue) {
    if (ins.state != InstructionState::FINNISH) {
      new_queue.push_back(ins);
    }
  } 

  this->scoreboard->instructionQueue.clear();
  this->scoreboard->instructionQueue = new_queue;
}

```

= Scoreboard design 

```cpp
#ifndef SIMULATOR_SCOREBOARD_H
#define SIMULATOR_SCOREBOARD_H

#include <vector>
#include <unordered_map>
#include <string>
#include "riscv.h"


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
    std::vector<Instruction> instructionQueue;  
};
#endif
```

= running result


