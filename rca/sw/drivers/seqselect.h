#ifndef SEQSELECT_H
#define SEQSELECT_H

#include "rca.h"
#include "static_region.h"
#include <stdlib.h>

#define SEQ_PROFILE_THRESH          20
#define MAX_ANNEALING_ATTEMPTS      5

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


typedef struct dfg_node_t dfg_node_t;

struct dfg_node_t{
    bool is_input;
    bool is_output;

    bool reg;
    uint32_t inp_value; //badly named
    //When is_input and reg are true, this corresponds to register address. Otherwise if is_input is only true, then is is the literal value.
    //If is_output is true, this corresponds to register address.

    ou_t op;    

    uint32_t node_id;

    dfg_node_t* next_node; //linked list
};

typedef struct dfg_edge_t dfg_edge_t;

struct dfg_edge_t{
    uint32_t fromNode;
    uint32_t toNode;
    grid_slot_inp_t slot_inp;
    
    dfg_edge_t* next_edge; //linked list
};

typedef struct{
    dfg_node_t* nodes;
    dfg_edge_t* edges;

    uint32_t num_nodes;
    uint32_t num_edges;
}dfg_t;

typedef struct{

}pr_grid_t;

typedef struct{
    uint32_t cpu_time;
    uint32_t acc_time; 
    uint32_t reconf_time;

    uint32_t init_interval;
    uint32_t num_rows;

    uint32_t num_annealing_attempts;

    dfg_t* dfg;
    pr_grid_t* pr_grid;
}seq_profile_t;

//INSTRUCION ID
bool analyse_instr_seq(uint32_t branch_addr, int32_t branch_offset, instr_seq_t* instr_seq);
bool analyse_instr(uint32_t raw_instr, instr_t* instr);
void get_operands_i(uint32_t raw_instr, instr_t* instr);
void get_operands_u(uint32_t raw_instr, instr_t* instr);
void get_operands_r(uint32_t raw_instr, instr_t* instr);
void get_operands_s(uint32_t raw_instr, instr_t* instr);

//DFG
dfg_t create_dfg(instr_seq_t* instr_seq);


#endif //SEQSELECT_H