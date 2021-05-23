#ifndef ENCODINGS_H
#define ENCODINGS_H

#include "rca.h"
#include "seqselect.h"

//defines taken from riscv-opc.h in binutils
#define MATCH_LUI 0x37
#define MASK_LUI  0x7f
#define MATCH_AUIPC 0x17
#define MASK_AUIPC  0x7f
#define MATCH_ADDI 0x13
#define MASK_ADDI  0x707f
#define MATCH_SLLI 0x1013
#define MASK_SLLI  0xfc00707f
#define MATCH_SLTI 0x2013
#define MASK_SLTI  0x707f
#define MATCH_SLTIU 0x3013
#define MASK_SLTIU  0x707f
#define MATCH_XORI 0x4013
#define MASK_XORI  0x707f
#define MATCH_SRLI 0x5013
#define MASK_SRLI  0xfc00707f
#define MATCH_SRAI 0x40005013
#define MASK_SRAI  0xfc00707f
#define MATCH_ORI 0x6013
#define MASK_ORI  0x707f
#define MATCH_ANDI 0x7013
#define MASK_ANDI  0x707f
#define MATCH_ADD 0x33
#define MASK_ADD  0xfe00707f
#define MATCH_SUB 0x40000033
#define MASK_SUB  0xfe00707f
#define MATCH_SLL 0x1033
#define MASK_SLL  0xfe00707f
#define MATCH_SLT 0x2033
#define MASK_SLT  0xfe00707f
#define MATCH_SLTU 0x3033
#define MASK_SLTU  0xfe00707f
#define MATCH_XOR 0x4033
#define MASK_XOR  0xfe00707f
#define MATCH_SRL 0x5033
#define MASK_SRL  0xfe00707f
#define MATCH_SRA 0x40005033
#define MASK_SRA  0xfe00707f
#define MATCH_OR 0x6033
#define MASK_OR  0xfe00707f
#define MATCH_AND 0x7033
#define MASK_AND  0xfe00707f

#define MATCH_LB 0x3
#define MASK_LB  0x707f
#define MATCH_LH 0x1003
#define MASK_LH  0x707f
#define MATCH_LW 0x2003
#define MASK_LW  0x707f
#define MATCH_LBU 0x4003
#define MASK_LBU  0x707f
#define MATCH_LHU 0x5003
#define MASK_LHU  0x707f
#define MATCH_SB 0x23
#define MASK_SB  0x707f
#define MATCH_SH 0x1023
#define MASK_SH  0x707f
#define MATCH_SW 0x2023
#define MASK_SW  0x707f


#define NUM_SUPPORTED_INSTS 29
const uint32_t masks[NUM_SUPPORTED_INSTS] = {
    MASK_ADDI,
    MASK_SLTIU,
    MASK_SLTI,
    MASK_ANDI,
    MASK_ORI,
    MASK_XORI,
    MASK_SLLI,
    MASK_SRLI,
    MASK_SRAI,
    MASK_LUI,
    MASK_AUIPC,
    MASK_ADD,
    MASK_SLT,
    MASK_SLTU,
    MASK_SLL,
    MASK_SUB,
    MASK_AND,
    MASK_OR,
    MASK_XOR,
    MASK_SRL,
    MASK_SRA,

    MASK_LB,
    MASK_LH,
    MASK_LW,
    MASK_LBU,
    MASK_LHU,
    MASK_SB,
    MASK_SH,
    MASK_SW
};
const uint32_t matches[NUM_SUPPORTED_INSTS] = {
    MATCH_ADDI,
    MATCH_SLTIU,
    MATCH_SLTI,
    MATCH_ANDI,
    MATCH_ORI,
    MATCH_XORI,
    MATCH_SLLI,
    MATCH_SRLI,
    MATCH_SRAI,
    MATCH_LUI,
    MATCH_AUIPC,
    MATCH_ADD,
    MATCH_SLT,
    MATCH_SLTU,
    MATCH_SLL,
    MATCH_SUB,
    MATCH_AND,
    MATCH_OR,
    MATCH_XOR,
    MATCH_SRL,
    MATCH_SRA,

    MATCH_LB,
    MATCH_LH,
    MATCH_LW,
    MATCH_LBU,
    MATCH_LHU,
    MATCH_SB,
    MATCH_SH,
    MATCH_SW
};
const ou_t instr_ou[NUM_SUPPORTED_INSTS] = {
    ADD,
    SLTU,
    SLT,
    AND,
    OR,
    XOR,
    SLL,
    SRL,
    SRA,
    LUI,
    AUIPC,
    ADD,
    SLT,
    SLTU,
    SLL,
    SUB,
    AND,
    OR,
    XOR,
    SRL,
    SRA,

    LB,
    LH,
    LW,
    LBU,
    LHU,
    SB,
    SH,
    SW
};

void (*operand_func[NUM_SUPPORTED_INSTS]) (uint32_t raw_instr, instr_t* instr) = {
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_u,
    get_operands_u,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,

    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_s,
    get_operands_s,
    get_operands_s
};


#endif //ENCODINGS_H