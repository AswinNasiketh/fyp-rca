#ifndef IID_H
#define IID_H

#include "rca.h"
#include "stdbool.h"
#include "stdlib.h"

typedef struct{
    bool reg; //true for reg, false for literal
    int32_t value; //reg addr if reg, literal value if literal
    //if reg is true and value is -1, operand is not used
}operand_t;

typedef struct{
    ou_t ou;
    operand_t input_operands[2];
    int32_t rd_addr; //if rd is -1, rd is not used
}instr_t;

typedef struct{
    instr_t* instr_arr;
    uint32_t num_instrs;
    uint32_t loop_start_addr;
    uint32_t loop_branch_addr;
}instr_seq_t;


void print_instr_seq(instr_seq_t seq);
bool analyse_instr_seq(uint32_t branch_addr, int32_t branch_offset, instr_seq_t* instr_seq);
bool analyse_instr(uint32_t raw_instr, instr_t* instr);
void get_operands_i(uint32_t raw_instr, instr_t* instr);
void get_operands_u(uint32_t raw_instr, instr_t* instr);
void get_operands_r(uint32_t raw_instr, instr_t* instr);
void get_operands_s(uint32_t raw_instr, instr_t* instr);
int32_t get_branch_offset(uint32_t branch_instr);

bool is_ls_op(ou_t op);

//encodings
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

#endif //IID_H