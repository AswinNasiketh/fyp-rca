#ifndef SUBGRID_H
#define SUBGRID_H

#include "rca.h"
#include "static_region.h"
#include "dfg.h"
#include <math.h>

#define MAX_ANNEALING_ATTEMPTS      5
#define ANNEAL_TEMP_THRESH          0.05f

typedef struct{
    ou_t ou;
    uint32_t node_id;
}pr_slot_t;

typedef struct{
    bool is_output;

    bool is_inp;
    bool is_reg;

    uint32_t inp_value;
    uint32_t output_reg_addr;
    // io_gci_mux_inp_addr_t io_gci_mux_inp;
    io_row_mux_inp_addr_t io_row_mux_inp;

    bool wait_for_ls_submit;

    uint32_t inp_node_id;
    uint32_t out_node_id;
}io_unit_cfg_t;

typedef pr_slot_t* pr_row_t;
typedef struct{
    grid_mux_inp_addr_t arr[2][NUM_GRID_COLS];
}row_xbar_cfg_t;


typedef struct{
    uint32_t num_rows;
    pr_row_t* grid_slots;
    io_unit_cfg_t* io_unit_cfgs;
    row_xbar_cfg_t* row_xbar_cfgs;
}sub_grid_t;

uint32_t estimate_grid_size(dfg_t dfg);
void move_lsus_to_left(sub_grid_t* sub_grid);
bool scatter_dfg(sub_grid_t* sub_grid, dfg_t dfg);
uint32_t calculate_gaps_cost(sub_grid_t* sub_grid, dfg_t dfg);
void find_io_unit(sub_grid_t* sub_grid, uint32_t node_id, uint32_t* pos, bool output);
void find_node_pos(sub_grid_t* sub_grid, uint32_t node_id, uint32_t* row, uint32_t* col, ou_t node_ou);
bool find_path(sub_grid_t* sub_grid, uint32_t from_node_row, uint32_t to_node_row, uint32_t from_node_id, bool from_node_is_input, bool to_node_is_output);
uint32_t unconnected_nets_cost(sub_grid_t* sub_grid, dfg_t dfg);
uint32_t calculate_total_cost(sub_grid_t* sub_grid, dfg_t dfg);
uint32_t get_randint_in_range(int32_t upper, int32_t lower);
void make_random_change_ou(sub_grid_t* sub_grid, uint32_t swap_dist_row, uint32_t swap_dist_col);
void make_random_change_io(sub_grid_t* sub_grid, uint32_t swap_dist_row);
void make_random_change(sub_grid_t* sub_grid, uint32_t swap_dist_row, uint32_t swap_dist_col);
void init_sub_grid(sub_grid_t* grid, uint32_t num_rows);
void copy_sub_grid(sub_grid_t* dest, sub_grid_t* src);
void free_sub_grid(sub_grid_t* grid);
float calculate_std_dev(uint32_t* costs, uint32_t num_costs);
float select_init_temp(sub_grid_t* sub_grid, dfg_t dfg);
bool can_exit_anneal(float curr_temp, sub_grid_t* grid, dfg_t dfg);
float new_temp_modifier(float prop_accepted);
float new_swap_dist_modifier(float prop_accepted);
uint32_t anneal(sub_grid_t* grid, dfg_t dfg, uint32_t iters_per_temp, uint32_t init_swap_dist_row, uint32_t init_swap_dist_col, float init_temp);
bool infer_mux_cfgs(sub_grid_t* sub_grid, dfg_t dfg);
bool gen_sub_grid(dfg_t dfg, sub_grid_t* sub_grid);
void print_sub_grid(sub_grid_t* sub_grid);
void print_mux_cfgs(sub_grid_t* sub_grid);

#endif //SUBGRID_H