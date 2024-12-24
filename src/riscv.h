#ifndef RISCV_H
#define RISCV_H

#include <cstdarg>
#include <cstdint>

namespace RISCV {



const int REGNUM = 32;
extern const char *REGNAME[32];
typedef int32_t RegId; // -1 means none


enum Reg {
  REG_ZERO = 0,
  REG_RA = 1,
  REG_SP = 2,
  REG_GP = 3,
  REG_TP = 4,
  REG_T0 = 5,
  REG_T1 = 6,
  REG_T2 = 7,
  REG_S0 = 8,
  REG_S1 = 9,
  REG_A0 = 10,
  REG_A1 = 11,
  REG_A2 = 12,
  REG_A3 = 13,
  REG_A4 = 14,
  REG_A5 = 15,
  REG_A6 = 16,
  REG_A7 = 17,
  REG_S2 = 18,
  REG_S3 = 19,
  REG_S4 = 20,
  REG_S5 = 21,
  REG_S6 = 22,
  REG_S7 = 23,
  REG_S8 = 24,
  REG_S9 = 25,
  REG_S10 = 26,
  REG_S11 = 27,
  REG_T3 = 28,
  REG_T4 = 29,
  REG_T5 = 30,
  REG_T6 = 31,
};

enum InstLargeType {
  R_TYPE,
  I_TYPE,
  S_TYPE,
  SB_TYPE,
  U_TYPE,
  UJ_TYPE,
};
enum InstType {
  LUI = 0,
  AUIPC = 1,
  JAL = 2,
  JALR = 3,
  BEQ = 4,
  BNE = 5,
  BLT = 6,
  BGE = 7,
  BLTU = 8,
  BGEU = 9,
  LB = 10,
  LH = 11,
  LW = 12,
  LD = 13,
  LBU = 14,
  LHU = 15,
  SB = 16,
  SH = 17,
  SW = 18,
  SD = 19,
  ADDI = 20,
  SLTI = 21,
  SLTIU = 22,
  XORI = 23,
  ORI = 24,
  ANDI = 25,
  SLLI = 26,
  SRLI = 27,
  SRAI = 28,
  ADD = 29,
  SUB = 30,
  SLL = 31,
  SLT = 32,
  SLTU = 33,
  XOR = 34,
  SRL = 35,
  SRA = 36,
  OR = 37,
  AND = 38,
  ECALL = 39,
  ADDIW = 40,
  MUL = 41,
  MULH = 42,
  DIV = 43,
  REM = 44,
  LWU = 45,
  SLLIW = 46,
  SRLIW = 47,
  SRAIW = 48,
  ADDW = 49,
  SUBW = 50,
  SLLW = 51,
  SRLW = 52,
  SRAW = 53,
  UNKNOWN = -1,
};
extern const char *INSTNAME[];

// Opcode field
const int OP_REG = 0x33;
const int OP_IMM = 0x13;
const int OP_LUI = 0x37;
const int OP_BRANCH = 0x63;
const int OP_STORE = 0x23;
const int OP_LOAD = 0x03;
const int OP_SYSTEM = 0x73;
const int OP_AUIPC = 0x17;
const int OP_JAL = 0x6F;
const int OP_JALR = 0x67;
const int OP_IMM32 = 0x1B;
const int OP_32 = 0x3B;

inline bool isBranch(InstType instType) {
  if (instType == BEQ || instType == BNE || instType == BLT || instType == BGE ||
      instType == BLTU || instType == BGEU) {
    return true;
  }
  return false;
}

inline bool isMul(InstType instType) {
  if (instType == MUL || instType == MULH) {
    return true;
  }
  return false;
}
inline bool isJump(InstType instType) {
  if (instType == JAL || instType == JALR) {
    return true;
  }
  return false;
}

inline bool isReadMem(InstType instType) {
  if (instType == LB || instType == LH || instType == LW || instType == LD || instType == LBU ||
      instType == LHU || instType == LWU) {
    return true;
  }
  return false;
}

inline bool isWriteMem(InstType instType) {
  if (instType == SB || instType == SH || instType == SW || instType == SD) {
    return true;
  }
  return false;
}



} // namespace RISCV

typedef struct Pipe_Op {
    // Control Signals
    bool bubble;
    uint32_t stall;

    // fetch 
    uint64_t pc;
    uint32_t pcLen;
    uint32_t inst;

    // decode
    RISCV::InstType instType;
    RISCV::RegId rs1, rs2;
    int64_t op1, op2;
    RISCV::RegId destReg;
    int64_t offset;
      // debug
      // std::string inststr;

    // execute
    int64_t out;
    bool writeReg;
    bool writeMem;
    bool readMem;
    bool readSignExt;
    uint32_t memLen;
    bool branch;


} Pipe_Op;

#endif