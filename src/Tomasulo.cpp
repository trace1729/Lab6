#include <iostream>
#include <string>
#include <vector>
#include "Tomasulo.h"
#include "Simulator.h"
#include "Debug.h"
#include "riscv.h"

Tomasulo::Tomasulo(int robSize, int rsSize, int regCount) : 
    rob(robSize), rs(rsSize), registerStatus(regCount) {}

Tomasulo::~Tomasulo() {}

int Tomasulo::allocateROBEntry(RISCV::InstType instType, int destination) {
    // Try to allocate an entry in the ROB
    if (rob[robTail].busy) return -1; // ROB is full
    rob[robTail].busy = true;
    rob[robTail].inst.opType = instType;
    rob[robTail].destination = destination;
    rob[robTail].ready = false;
    int allocatedIndex = robTail;
    robTail = (robTail + 1) % rob.size();
    return allocatedIndex;
}

int Tomasulo::allocateRS(InstType op, int dest, int qj, int qk) {
    for (size_t i = 0; i < rs.size(); ++i) {
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

FunctionUnitType Tomasulo::mapInstructionToFU(RISCV::InstType type) {
    if (isReadMem(type) || isWriteMem(type)) {
        return FunctionUnitType::MEM;
    }
    
    if (isMul(type)) {
        return FunctionUnitType::MUL;
    }
    
    return FunctionUnitType::ALU;
}



void Tomasulo::issue() {
    
}

void Tomasulo::execute() {
    
}

void Tomasulo::writeBack() {
    
}

void Tomasulo::commit() {
    
}

bool Tomasulo::decode(uint32_t inst, uint64_t* reg, Instruction* score_inst, Simulator* simu) {
  std::string instname = "";
  // std::string &inststr = op->inststr;
  std::string inststr = "";
  std::string deststr, op1str, op2str, offsetstr;
  InstType instType = InstType::UNKNOWN;
  // op constains register value
  int64_t op1 = 0, op2 = 0, offset = 0; // op1, op2 and offset are values
  // destReg reg1 and rs1 contain register index
  RegId destReg = 0, reg1 = 0, reg2 = 0; // reg1 and reg2 are operands

  
    uint32_t opcode = inst & 0x7F;
    uint32_t funct3 = (inst >> 12) & 0x7;
    uint32_t funct7 = (inst >> 25) & 0x7F;
    RegId rd = (inst >> 7) & 0x1F;
    RegId rs1 = (inst >> 15) & 0x1F;
    RegId rs2 = (inst >> 20) & 0x1F;
    int32_t imm_i = int32_t(inst) >> 20;
    int32_t imm_s =
        int32_t(((inst >> 7) & 0x1F) | ((inst >> 20) & 0xFE0)) << 20 >> 20;
    int32_t imm_sb = int32_t(((inst >> 7) & 0x1E) | ((inst >> 20) & 0x7E0) |
                             ((inst << 4) & 0x800) | ((inst >> 19) & 0x1000))
                         << 19 >>
                     19;
    int32_t imm_u = int32_t(inst) >> 12;
    int32_t imm_uj = int32_t(((inst >> 21) & 0x3FF) | ((inst >> 10) & 0x400) |
                             ((inst >> 1) & 0x7F800) | ((inst >> 12) & 0x80000))
                         << 12 >>
                     11;

    switch (opcode) {
    case OP_REG:
      op1 = reg[rs1];
      op2 = reg[rs2];
      reg1 = rs1;
      reg2 = rs2;
      destReg = rd;
      switch (funct3) {
      case 0x0: // add, mul, sub
        if (funct7 == 0x00) {
          instname = "add";
          instType = ADD;
        } else if (funct7 == 0x01) {
          instname = "mul";
          instType = MUL;
        } else if (funct7 == 0x20) {
          instname = "sub";
          instType = SUB;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x1: // sll, mulh
        if (funct7 == 0x00) {
          instname = "sll";
          instType = SLL;
        } else if (funct7 == 0x01) {
          instname = "mulh";
          instType = MULH;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x2: // slt
        if (funct7 == 0x00) {
          instname = "slt";
          instType = SLT;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x3: // sltu
        if (funct7 == 0x00)
        {
          instname = "sltu";
          instType = SLTU;
        }
        else
        {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x4: // xor div
        if (funct7 == 0x00) {
          instname = "xor";
          instType = XOR;
        } else if (funct7 == 0x01) {
          instname = "div";
          instType = DIV;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x5: // srl, sra
        if (funct7 == 0x00) {
          instname = "srl";
          instType = SRL;
        } else if (funct7 == 0x20) {
          instname = "sra";
          instType = SRA;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x6: // or
        if (funct7 == 0x00) {
          instname = "or";
          instType = OR;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      case 0x7: // and
        if (funct7 == 0x00) {
          instname = "and";
          instType = AND;
        } else {
          simu->panic("Unknown funct7 0x%x for funct3 0x%x\n", funct7, funct3);
        }
        break;
      default:
        simu->panic("Unknown Funct3 field %x\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = REGNAME[rs2];
      deststr = REGNAME[rd];
      inststr = instname + " " + deststr + "," + op1str + "," + op2str;
      break;
    case OP_IMM:
      op1 = reg[rs1];
      reg1 = rs1;
      op2 = imm_i;
      destReg = rd;
      switch (funct3) {
      case 0x0:
        instname = "addi";
        instType = ADDI;
        break;
      case 0x2:
        instname = "slti";
        instType = SLTI;
        break;
      case 0x3:
        instname = "sltiu";
        instType = SLTIU;
        break;
      case 0x4:
        instname = "xori";
        instType = XORI;
        break;
      case 0x6:
        instname = "ori";
        instType = ORI;
        break;
      case 0x7:
        instname = "andi";
        instType = ANDI;
        break;
      case 0x1:
        instname = "slli";
        instType = SLLI;
        op2 = op2 & 0x3F;
        break;
      case 0x5:
        if (((inst >> 26) & 0x3F) == 0x0) {
          instname = "srli";
          instType = SRLI;
          op2 = op2 & 0x3F;
        } else if (((inst >> 26) & 0x3F) == 0x10) {
          instname = "srai";
          instType = SRAI;
          op2 = op2 & 0x3F;
        } else {
          simu->panic("Unknown funct7 0x%x for OP_IMM\n", (inst >> 26) & 0x3F);
        }
        break;
      default:
        simu->panic("Unknown Funct3 field %x\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = std::to_string(op2);
      deststr = REGNAME[destReg];
      inststr = instname + " " + deststr + "," + op1str + "," + op2str;
      break;
    case OP_LUI:
      op1 = imm_u;
      op2 = 0;
      offset = imm_u;
      destReg = rd;
      instname = "lui";
      instType = LUI;
      op1str = std::to_string(imm_u);
      deststr = REGNAME[destReg];
      inststr = instname + " " + deststr + "," + op1str;
      break;
    case OP_AUIPC:
      op1 = imm_u;
      op2 = 0;
      offset = imm_u;
      destReg = rd;
      instname = "auipc";
      instType = AUIPC;
      op1str = std::to_string(imm_u);
      deststr = REGNAME[destReg];
      inststr = instname + " " + deststr + "," + op1str;
      break;
    case OP_JAL:
      op1 = imm_uj;
      op2 = 0;
      offset = imm_uj;
      destReg = rd;
      instname = "jal";
      instType = JAL;
      op1str = std::to_string(imm_uj);
      deststr = REGNAME[destReg];
      inststr = instname + " " + deststr + "," + op1str;
      break;
    case OP_JALR:
      op1 = reg[rs1];
      reg1 = rs1;
      op2 = imm_i;
      destReg = rd;
      instname = "jalr";
      instType = JALR;
      op1str = REGNAME[rs1];
      op2str = std::to_string(op2);
      deststr = REGNAME[destReg];
      inststr = instname + " " + deststr + "," + op1str + "," + op2str;
      break;
    case OP_BRANCH:
      op1 = reg[rs1];
      op2 = reg[rs2];
      reg1 = rs1;
      reg2 = rs2;
      offset = imm_sb;
      switch (funct3) {
      case 0x0:
        instname = "beq";
        instType = BEQ;
        break;
      case 0x1:
        instname = "bne";
        instType = BNE;
        break;
      case 0x4:
        instname = "blt";
        instType = BLT;
        break;
      case 0x5:
        instname = "bge";
        instType = BGE;
        break;
      case 0x6:
        instname = "bltu";
        instType = BLTU;
        break;
      case 0x7:
        instname = "bgeu";
        instType = BGEU;
        break;
      default:
        simu->panic("Unknown funct3 0x%x at OP_BRANCH\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = REGNAME[rs2];
      offsetstr = std::to_string(offset);
      inststr = instname + " " + op1str + "," + op2str + "," + offsetstr;
      break;
    case OP_STORE:
      op1 = reg[rs1];
      op2 = reg[rs2];
      reg1 = rs1;
      reg2 = rs2;
      offset = imm_s;
      switch (funct3) {
      case 0x0:
        instname = "sb";
        instType = SB;
        break;
      case 0x1:
        instname = "sh";
        instType = SH;
        break;
      case 0x2:
        instname = "sw";
        instType = SW;
        break;
      case 0x3:
        instname = "sd";
        instType = SD;
        break;
      default:
        simu->panic("Unknown funct3 0x%x for OP_STORE\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = REGNAME[rs2];
      offsetstr = std::to_string(offset);
      inststr = instname + " " + op2str + "," + offsetstr + "(" + op1str + ")";
      break;
    case OP_LOAD:
      op1 = reg[rs1];
      reg1 = rs1;
      op2 = imm_i;
      offset = imm_i;
      destReg = rd;
      switch (funct3) {
      case 0x0:
        instname = "lb";
        instType = LB;
        break;
      case 0x1:
        instname = "lh";
        instType = LH;
        break;
      case 0x2:
        instname = "lw";
        instType = LW;
        break;
      case 0x3:
        instname = "ld";
        instType = LD;
        break;
      case 0x4:
        instname = "lbu";
        instType = LBU;
        break;
      case 0x5:
        instname = "lhu";
        instType = LHU;
        break;
      case 0x6:
        instname = "lwu";
        instType = LWU;
      default:
        simu->panic("Unknown funct3 0x%x for OP_LOAD\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = std::to_string(op2);
      deststr = REGNAME[rd];
      inststr = instname + " " + deststr + "," + op2str + "(" + op1str + ")";
      break;
    case OP_SYSTEM:
      if (funct3 == 0x0 && funct7 == 0x000) {
        instname = "ecall";
        op1 = reg[REG_A0];
        op2 = reg[REG_A7];
        reg1 = REG_A0;
        reg2 = REG_A7;
        destReg = REG_A0;
        instType = ECALL;
      } else {
        simu->panic("Unknown OP_SYSTEM inst with funct3 0x%x and funct7 0x%x\n",
                    funct3, funct7);
      }
      inststr = instname;
      break;
    case OP_IMM32:
      op1 = reg[rs1];
      reg1 = rs1;
      op2 = imm_i;
      destReg = rd;
      switch (funct3) {
      case 0x0:
        instname = "addiw";
        instType = ADDIW;
        break;
      case 0x1:
        instname = "slliw";
        instType = SLLIW;
        break;
      case 0x5:
        if (((inst >> 25) & 0x7F) == 0x0) {
          instname = "srliw";
          instType = SRLIW;
        } else if (((inst >> 25) & 0x7F) == 0x20) {
          instname = "sraiw";
          instType = SRAIW;
        } else {
          simu->panic("Unknown shift inst type 0x%x\n", ((inst >> 25) & 0x7F));
        }
        break;
      default:
        simu->panic("Unknown funct3 0x%x for OP_ADDIW\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = std::to_string(op2);
      deststr = REGNAME[rd];
      inststr = instname + " " + deststr + "," + op1str + "," + op2str;
      break;
    case OP_32: {
      op1 = reg[rs1];
      op2 = reg[rs2];
      reg1 = rs1;
      reg2 = rs2;
      destReg = rd;

      uint32_t temp = (inst >> 25) & 0x7F; // 32bit funct7 field
      switch (funct3) {
      case 0x0:
        if (temp == 0x0) {
          instname = "addw";
          instType = ADDW;
        } else if (temp == 0x20) {
          instname = "subw";
          instType = SUBW;
        } else {
          simu->panic("Unknown 32bit funct7 0x%x\n", temp);
        }
        break;
      case 0x1:
        if (temp == 0x0) {
          instname = "sllw";
          instType = SLLW;
        } else {
          simu->panic("Unknown 32bit funct7 0x%x\n", temp);
        }
        break;
      case 0x5:
        if (temp == 0x0) {
          instname = "srlw";
          instType = SRLW;
        } else if (temp == 0x20) {
          instname = "sraw";
          instType = SRAW;
        } else {
          simu->panic("Unknown 32bit funct7 0x%x\n", temp);
        }
        break;
      default:
        simu->panic("Unknown 32bit funct3 0x%x\n", funct3);
      }
      op1str = REGNAME[rs1];
      op2str = REGNAME[rs2];
      deststr = REGNAME[rd];
      inststr = instname + " " + deststr + "," + op1str + "," + op2str;      
    } break;
    default:
      simu->panic("Unsupported opcode 0x%x!\n", opcode);
    }

    score_inst->destReg = destReg;
    score_inst->srcReg1 = reg1;
    score_inst->srcReg2 = reg2;
    score_inst->opType = instType;
    score_inst->instStr = inststr;
    score_inst->op.offset = offset;
    score_inst->op.op1 = op1;
    score_inst->op.op2 = op2;
    FunctionUnitType unit = this->mapInstructionToFU(instType);

    switch (unit) {
    case FunctionUnitType::ALU:
      score_inst->remainingExecCycles = 0;
      break;
    case FunctionUnitType::MEM:
      score_inst->remainingExecCycles = 0;
      break;
    case FunctionUnitType::MUL:
      score_inst->remainingExecCycles = 5;
      break;
    default:
      break;
    }

    // funit is been occupied
    return true;
}

bool Tomasulo::hasStoreConflict(int robIndex) {
    int current = robHead;
    while (current != robIndex) {
        const ROBEntry &entry = rob[current];

        // Check if it's a Store instruction
        if (isWriteMem(entry.inst.opType)) {
            // Check if the Store has the same address and is ahead in the ROB
           return true;
        }

        // Move to the next ROB entry
        current = (current + 1) % rob.size();
    }
    return false; // No conflict detected
}

bool Tomasulo::execMem(Instruction* score_inst, Simulator* simu) {
  
  InstType opType = score_inst->opType;
  // out op2 memLen
  uint64_t out = score_inst->op.out;
  uint32_t memLen = score_inst->op.memLen;
  uint64_t op2 = score_inst->op.op2;
  uint64_t op1 = score_inst->op.op1;
  uint64_t offset = score_inst->op.offset;

  bool writeMem = false, readMem = false, readSignExt = false;
  score_inst->op.writeReg = false;
  
  switch (opType) {
    case LB:
      readMem = true;
      memLen = 1;
      out = op1 + offset;
      readSignExt = true;
      break;
    case LH:
      readMem = true;
      memLen = 2;
      out = op1 + offset;
      readSignExt = true;
      break;
    case LW:
      readMem = true;
      memLen = 4;
      out = op1 + offset;
      readSignExt = true;
      break;
    case LD:
      readMem = true;
      memLen = 8;
      out = op1 + offset;
      readSignExt = true;
      break;
    case LBU:
      readMem = true;
      memLen = 1;
      out = op1 + offset;
      break;
    case LHU:
      readMem = true;
      memLen = 2;
      out = op1 + offset;
      break;
    case LWU:
      readMem = true;
      memLen = 4;
      out = op1 + offset;
      break;
    case SB:
      writeMem = true;
      memLen = 1;
      out = op1 + offset;
      op2 = op2 & 0xFF;
      break;
    case SH:
      writeMem = true;
      memLen = 2;
      out = op1 + offset;
      op2 = op2 & 0xFFFF;
      break;
    case SW:
      writeMem = true;
      memLen = 4;
      out = op1 + offset;
      op2 = op2 & 0xFFFFFFFF;
      break;
    case SD:
      writeMem = true;
      memLen = 8;
      out = op1 + offset;
      break;
    default:break;
  }
  
  bool good = false;

  if (writeMem){
    dbgprintf("m[%x] = %x\n", out, op2);
    switch (memLen) {
    case 1:
      good = simu->memory->setByte(out, op2);
      break;
    case 2:
      good = simu->memory->setShort(out, op2);
      break;
    case 4:
      good = simu->memory->setInt(out, op2);
      break;
    case 8:
      good = simu->memory->setLong(out, op2);
      break;
    default:
      simu->panic("Unknown memLen %d\n", memLen);
    }
  }

  if (!good) {
    simu->panic("Invalid Mem Access!\n");
  }

  if (readMem) {
    dbgprintf("read from %x\n", out);
    switch (memLen) {
    case 1:
      if (readSignExt) {
        out = (int64_t)simu->memory->getByte(out);
      } else {
        out = (uint64_t)simu->memory->getByte(out);
      }
      break;
    case 2:
      if (readSignExt) {
        out = (int64_t)simu->memory->getShort(out);
      } else {
        out = (uint64_t)simu->memory->getShort(out);
      }
      break;
    case 4:
      if (readSignExt) {
        out = (int64_t)simu->memory->getInt(out);
      } else {
        out = (uint64_t)simu->memory->getInt(out);
      }
      break;
    case 8:
      if (readSignExt) {
        out = (int64_t)simu->memory->getLong(out);
      } else {
        out = (uint64_t)simu->memory->getLong(out);
      }
      break;
    default:
      simu->panic("Unknown memLen %d\n", memLen);
    }
  }
  return true;
}


bool Tomasulo::execArthimetic(Instruction* inst, Simulator* simu) {
  
  InstType instType = inst->opType;
  int64_t op1 = inst->op.op1;
  int64_t op2 = inst->op.op2;
  int64_t offset = inst->op.offset;
  int64_t current_pc = inst->pc;
  
  RegId destReg = inst->destReg;
  int64_t out = 0;
  bool branch = false;
  inst->op.writeReg = true;

  uint64_t jumpPC = inst->pc;

  switch (instType) {
  case LUI:
    out = offset << 12;
    break;
  case AUIPC:
    out = jumpPC + (offset << 12);
    break;
  case JAL:
    out = jumpPC + 4;
    jumpPC = jumpPC + op1;
    branch = true;
    break;
  case JALR:
    out = jumpPC + 4;
    jumpPC = (op1 + op2) & (~(uint64_t)1);
    branch = true;
    break;
  case BEQ:
    if (op1 == op2) {
      branch = true;
      jumpPC = jumpPC + offset;
    }
    break;
  case BNE:
    if (op1 != op2) {
      branch = true;
      jumpPC = jumpPC + offset;
    }
    break;
  case BLT:
    if (op1 < op2) {
      branch = true;
      jumpPC = jumpPC + offset;
    }
    break;
  case BGE:
    if (op1 >= op2) {
      branch = true;
      jumpPC = jumpPC + offset;
    }
    break;
  case BLTU:
    if ((uint64_t)op1 < (uint64_t)op2) {
      branch = true;
      jumpPC = jumpPC + offset;
    }
    break;
  case BGEU:
    if ((uint64_t)op1 >= (uint64_t)op2) {
      branch = true;
      jumpPC = jumpPC + offset;
    }
    break;
  case ADDI:
  case ADD:
    out = op1 + op2;
    break;
  case ADDIW:
  case ADDW:
    out = (int64_t)((int32_t)op1 + (int32_t)op2);
    break;
  case SUB:
    out = op1 - op2;
    break;
  case SUBW:
    out = (int64_t)((int32_t)op1 - (int32_t)op2);
    break;
  case MUL:
    out = op1 * op2;
    break;
  case DIV:
    out = op1 / op2;
    break;
  case SLTI:
  case SLT:
    out = op1 < op2 ? 1 : 0;
    break;
  case SLTIU:
  case SLTU:
    out = (uint64_t)op1 < (uint64_t)op2 ? 1 : 0;
    break;
  case XORI:
  case XOR:
    out = op1 ^ op2;
    break;
  case ORI:
  case OR:
    out = op1 | op2;
    break;
  case ANDI:
  case AND:
    out = op1 & op2;
    break;
  case SLLI:
  case SLL:
    out = op1 << op2;
    break;
  case SLLIW:
  case SLLW:
    out = int64_t(int32_t(op1 << op2));
    break;
    break;
  case SRLI:
  case SRL:
    out = (uint64_t)op1 >> (uint64_t)op2;
    break;
  case SRLIW:
  case SRLW:
    out = uint64_t(uint32_t((uint32_t)op1 >> (uint32_t)op2));
    break;
  case SRAI:
  case SRA:
    out = op1 >> op2;
    break;
  case SRAW:
  case SRAIW:
    out = int64_t(int32_t((int32_t)op1 >> (int32_t)op2));
    break;
  case ECALL:
    out = simu->handleSystemCall(op1, op2);
    break;
  default:
    simu->panic("Unknown instruction type %d\n", instType);
  }
  // change function unit
  inst->op.out = out;
  dbgprintf("The ALU output of this instruction is 0x%x\n", out);
  if (branch == false) {
    jumpPC = current_pc + 4;
  }
  if (isBranch(instType) || isJump(instType)) {
    simu->pc = jumpPC;
    dbgprintf("This inst JUMPs to 0x%x, offset 0x%x\n", jumpPC, inst->op.offset);
  }
  return true;
}

void Tomasulo::printROB() {
    std::cout << "Reorder Buffer (ROB):\n";
    for (auto& entry : rob) {
        std::cout << "Type: " << entry.inst.opType
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
    for (size_t i = 0; i < registerStatus.size(); ++i) {
        std::cout << "Reg " << i << ": ROB Index: " << registerStatus[i].robIndex << ", Busy: " << registerStatus[i].busy << "\n";
    }
}
