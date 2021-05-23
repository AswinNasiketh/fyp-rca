#ifndef SEQSELECT_H
#define SEQSELECT_H

#include "rca.h"
#include <stdlib.h>

#define SEQ_PROFILE_THRESH 20

// void handle_profiler_exception(); // forward declared in board_support.h

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
    bool seq_supported;
}instr_seq_t;

bool analyse_instr_seq(uint32_t branch_addr, int32_t branch_offset, instr_seq_t* instr_seq);
bool analyse_instr(uint32_t raw_instr, instr_t* instr);
void get_operands_i(uint32_t raw_instr, instr_t* instr);
void get_operands_u(uint32_t raw_instr, instr_t* instr);
void get_operands_r(uint32_t raw_instr, instr_t* instr);
void get_operands_s(uint32_t raw_instr, instr_t* instr);

#endif //SEQSELECT_H