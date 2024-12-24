#include "Scoreboard.h"
#include "Simulator.h"
#include "riscv.h"

#include <iomanip>
#include <sstream>
#include "Debug.h"

using namespace RISCV;

Scoreboard::Scoreboard() {

}

Scoreboard::~Scoreboard() {

}

FunctionUnitType Scoreboard::mapInstructionToFU(InstType type) {
    if (isReadMem(type) || isWriteMem(type)) {
        return FunctionUnitType::MEM;
    }
    
    if (isMul(type)) {
        return FunctionUnitType::MUL;
    }
    
    return FunctionUnitType::ALU;
}

std::string Scoreboard::findAvailableFU(FunctionUnitType f_type) {
    std::vector<std::string> alus = {"alu0", "alu1", "alu2", "alu3"};
    std::vector<std::string> muls = {"mul0", "mul1", "mul2", "mul3"};

    if (f_type == FunctionUnitType::ALU) {
        for (auto alu_name: alus) {
            if (!functionalUnits[alu_name].busy) {
                return alu_name;
            }
        }   
        dbgprintf("alu is unavailable\n");
    } else if (f_type == FunctionUnitType::MUL) {
        for (auto mul_name: muls) {
            if (!functionalUnits[mul_name].busy) {
                return mul_name;
            }
        }
        dbgprintf("mul is unavailable\n");
    } else {
        if (!functionalUnits["mem"].busy) {
            return "mem";
        }
        dbgprintf("mem is unavailable\n");
    }
    return "";
}

bool Scoreboard::checkWAR(int destReg) {
  for (auto& entry: functionalUnits) {
    if (entry.second.fj == destReg && entry.second.rj == false)  {
      return true;
    }
    if (entry.second.fk == destReg && entry.second.rj == false) {
      return true;
    }
  }
  return false;
}

void Scoreboard::writeBack(std::string funit) {
  FunctionalUnit& unit = this->functionalUnits[funit];

  unit.busy = false;

  dbgprintf("clear %d of %s\n", unit.fi, registerResultStatus[unit.fi].c_str());
  registerResultStatus[unit.fi] = ""; 
  
  for (auto& entry: functionalUnits) {
    if (entry.second.qj == funit) {
      entry.second.qj = "";
    }
    if (entry.second.qk == funit) {
      entry.second.qk = "";
    }
  }


}


bool Scoreboard::execArthimetic(std::string funit, Instruction* inst, Simulator* simu) {
  // inst, Simulator
  FunctionalUnit unit = this->functionalUnits[funit];
  

  InstType instType = unit.op;
  int64_t op1 = inst->op.op1;
  int64_t op2 = inst->op.op2;
  int64_t offset = inst->op.offset;
  int64_t current_pc = inst->pc;
  
  RegId destReg = unit.fi;
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

bool Scoreboard::execMem(Instruction* score_inst, Simulator* simu) {
  
  FunctionalUnit funit = this->functionalUnits["mem"];
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

bool Scoreboard::exec(std::string funit, Instruction* inst, Simulator* simu) {

  FunctionalUnit& funcEntry = this->functionalUnits[funit];
  funcEntry.rj = true;
  funcEntry.rk = true;

  if (funit.compare("mem") == 0) {
    return execMem(inst, simu); 
  } else {
    return execArthimetic(funit, inst, simu);
  }
}

bool Scoreboard::decode(uint32_t inst, uint64_t* reg, Instruction* score_inst, Simulator* simu) {
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

    // dispatch function unit based on instruction type
    FunctionUnitType unit = this->mapInstructionToFU(instType);
    std::string funit = this->findAvailableFU(unit);

    // if find available unit, we could do further check.
    if (funit != "") {
        // for better debugging
        score_inst->destReg = destReg;
        score_inst->srcReg1 = reg1;
        score_inst->srcReg2 = reg2;
        score_inst->opType = instType;
        score_inst->instStr = inststr;

        if (this->registerResultStatus[destReg] != "") {
            dbgprintf("rd %d is been occupied!\n", destReg);
            // rd is been occupied
            return false;
        }
        score_inst->processingUnit = funit;
        // for later executing
        // op1 op2 instType 没有必要存储
        score_inst->op.offset = offset;
        score_inst->op.op1 = op1;
        score_inst->op.op2 = op2;
        this->functionalUnits[funit].busy = true;
        this->functionalUnits[funit].op = instType;
        // fijk 都是序号
        this->functionalUnits[funit].fi = destReg;
        this->functionalUnits[funit].fj = reg1;
        this->functionalUnits[funit].fk = reg2;

        // qik 都是 functional unit
        this->functionalUnits[funit].qj = this->registerResultStatus[reg1];
        this->functionalUnits[funit].qk = this->registerResultStatus[reg2];

        this->registerResultStatus[destReg] = funit; 
        // rjk 暂时不知道是干嘛的
        this->functionalUnits[funit].rj = true;
        this->functionalUnits[funit].rk = true;

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
        return true;
    }
    // funit is been occupied
    return false;
}



// Utility to create a fixed-width string
std::string fixedWidth(const std::string& str, int width) {
    std::ostringstream oss;
    oss << std::left << std::setw(width) << str;
    return oss.str();
}

// Overloaded version for integers
std::string fixedWidth(int value, int width) {
    std::ostringstream oss;
    oss << std::left << std::setw(width) << value;
    return oss.str();
}

// Overloaded version for integers
std::string fixedWidthHex(int value, int width) {
    std::ostringstream oss;
    oss << std::left << std::hex << std::setw(width) << value;
    return oss.str();
}

std::string Scoreboard::toString() const {
    std::ostringstream oss;

    // === Instructions Table ===
    oss << "\n=== Instruction Queue ===\n";
    oss << "+------+--------------------+------+-------+-------+------------------+------------+\n";
    oss << "| PC   | Inst               | Dest | Src1  | Src2  | State            | ExecCycles |\n";
    oss << "+------+--------------------+------+-------+-------+------------------+------------+\n";

    for (const auto& inst : instructionQueue) {
        oss << "| " << fixedWidthHex(inst.pc, 5)
            << "| " << fixedWidth(inst.instStr, 19)
            << "| " << fixedWidth(REGNAME[inst.destReg], 5)
            << "| " << fixedWidth(REGNAME[inst.srcReg1], 6)
            << "| " << fixedWidth(REGNAME[inst.srcReg2], 6)
            << "| " << fixedWidth(
               inst.state == InstructionState::STALL ? "STALL" :
                   inst.state == InstructionState::ISSUE ? "ISSUE" :
                   inst.state == InstructionState::READ_OPERANDS ? "READ_OPERANDS" :
                   inst.state == InstructionState::EXECUTE ? "EXECUTE" : 
                   inst.state == InstructionState::WRITE_BACK? "WRITE_BACK" :
                   "FINNISH",
                   17)
            << "| " << fixedWidth(inst.remainingExecCycles, 11)
            << "|\n";
    }
    oss << "+------+--------------------+------+-------+-------+------------------+------------+\n";

    // === Functional Unit Table ===
    oss << "\n=== Functional Units ===\n";
    oss << "+----------+------+-------------+------+-------+-------+-------+-------+-------+-------+\n";
    oss << "| FU Name  | Busy | Op          | Dest | Src1  | Src2  | Qj    | Qk    | Rj    | Rk    |\n";
    oss << "+----------+------+-------------+------+-------+-------+-------+-------+-------+-------+\n";

    for (const auto& entry : functionalUnits) {
        const auto& fu = entry.second;
        oss << "| " << fixedWidth(entry.first, 9)
            << "| " << fixedWidth(fu.busy ? "Yes" : "No", 5)
            << "| " << fixedWidth(fu.busy? INSTNAME[fu.op]: "None", 12)
            << "| " << fixedWidth(fu.fi, 5)
            << "| " << fixedWidth(fu.fj, 6)
            << "| " << fixedWidth(fu.fk, 6)
            << "| " << fixedWidth(fu.qj.empty() ? "-" : fu.qj, 6)
            << "| " << fixedWidth(fu.qk.empty() ? "-" : fu.qk, 6)
            << "| " << fixedWidth(fu.rj ? "Ready" : "Not Ready", 6)
            << "| " << fixedWidth(fu.rk ? "Ready" : "Not Ready", 6)
            << "|\n";
    }
    oss << "+----------+------+-------------+------+-------+-------+-------+-------+-------+-------+\n";

    // === Register Result Status ===

    oss << "\n=== Register Result Status ===\n";
        oss << "+------+------+------+------+------+------+------+------";
        oss << "+------+------+------+------+------+------+------+------+\n";

    for (int row = 0; row < 32; row += 16) {
        // Register Names Row
        oss << "|";
        for (int col = 0; col < 16; ++col) {
            oss << fixedWidth("R" + std::to_string(row + col), 6) << "|";
        }
        oss << "\n";

        // Divider Row
        oss << "+------+------+------+------+------+------+------+------";
        oss << "+------+------+------+------+------+------+------+------+\n";

        // Register FU Mapping Row
        oss << "|";
        for (int col = 0; col < 16; ++col) {
            int regIndex = row + col;
            std::string fu = registerResultStatus.count(regIndex) ? registerResultStatus.at(regIndex) : "-";
            oss << fixedWidth(fu, 6) << "|";
        }
        oss << "\n";

        // Divider Row
        oss << "+------+------+------+------+------+------+------+------";
        oss << "+------+------+------+------+------+------+------+------+\n";
    }
    return oss.str();
}

using json = nlohmann::json;

void to_json(json& j, const Instruction& inst) {
    j = json{
        {"pc", inst.pc},
        {"destReg", inst.destReg},
        {"srcReg1", inst.srcReg1},
        {"srcReg2", inst.srcReg2},
        {"state", inst.state}, // Ensure InstructionState is serializable
        {"remainingExecCycles", inst.remainingExecCycles},
        {"processingUnit", inst.processingUnit},
        {"instStr", inst.instStr}
        // Add other members as needed
    };
}

void to_json(json& j, const FunctionalUnit& fu) {
    j = json{
        {"busy", fu.busy},
        {"op", fu.op}, // Ensure RISCV::InstType is serializable
        {"fi", fu.fi},
        {"fj", fu.fj},
        {"fk", fu.fk},
        {"qj", fu.qj},
        {"qk", fu.qk},
        {"rj", fu.rj},
        {"rk", fu.rk}
    };
}


json Scoreboard::toJson() const {
    json j;
    j["functionalUnits"] = functionalUnits;
    j["registerResultStatus"] = registerResultStatus;
    j["instructionQueue"] = instructionQueue;
    return j;
}
