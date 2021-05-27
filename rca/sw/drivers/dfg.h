#ifndef DFG_H
#define DFG_H

#include "rca.h"
#include "static_region.h"
#include "iid.h"

#define CPU_CLK_FREQ                50000000
#define FRAME_RECONF_TIME_MS        6
#define RCA_PACKET_LSQ_DEPTH        8
#define CPU_LOAD_LATENCY            11 //taken from taiga latency diagram + FIFO depth

typedef struct dfg_node_t dfg_node_t;

struct dfg_node_t{
    bool is_input;
    bool is_output;

    bool reg;
    bool is_literal_input_used; //used to disable literal input nodes when squashing them
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

void add_node(dfg_node_t* node_lst, dfg_node_t* node);
void init_node(dfg_node_t* node);
void init_edge(dfg_edge_t* edge);
void add_edge(dfg_edge_t* last_edge, dfg_edge_t* new_edge);

void init_new_node_reg(dfg_node_t* new_node, uint32_t reg_addr, uint32_t node_id);
void init_new_node_lit(dfg_node_t* new_node, uint32_t lit_val, uint32_t node_id);
void init_new_node_op(dfg_node_t* new_node, ou_t op, uint32_t node_id);
void init_new_edge(dfg_edge_t* new_edge, uint32_t fromNode, uint32_t toNode, grid_slot_inp_t slot_inp);
void init_new_node_output(dfg_node_t* new_node, uint32_t reg_addr, uint32_t node_id);

void process_instr_inputs_normal(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge);
void process_instr_inputs_auipc(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge, uint32_t instr_addr);
void process_instr_inputs_load(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge);
void process_instr_inputs_store(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge);
void process_instr_inputs(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge, uint32_t curr_instr_addr);
void lls_to_arrays(dfg_node_t* first_node, dfg_edge_t* first_edge, uint32_t* num_nodes, uint32_t* num_edges, dfg_node_t** node_arr_ptr, dfg_edge_t** edge_arr_ptr);

dfg_t create_dfg(instr_seq_t* instr_seq);
uint32_t find_node_depth_cpu(dfg_t dfg, uint32_t nodeID, bool recursive_call);
uint32_t find_node_depth_acc(dfg_t dfg, uint32_t nodeID, uint32_t num_ls_ops, bool recursive_call);
uint32_t calc_pr_time(dfg_t dfg);

void remap_edges(dfg_t* dfg, uint32_t node_id_to_replace, uint32_t new_node_id);
void squash_literal_inputs(dfg_t* dfg);
uint32_t get_num_op_nodes(dfg_t dfg);
void print_dfg(dfg_t dfg);

#endif //DFG_H