#include "subgrid.h"

uint32_t estimate_grid_size(dfg_t dfg){
    uint32_t num_inputs = 0;
    uint32_t num_outputs = 0;
    uint32_t num_ls = 0;

    for(int i = 0; i < dfg.num_nodes; i++){
        if(dfg.nodes[i].is_input && dfg.nodes[i].reg){
            num_inputs++;
        }else if(dfg.nodes[i].is_input && !dfg.nodes[i].reg && dfg.nodes[i].is_literal_input_used){
            num_inputs++;
        }else if(!dfg.nodes[i].is_input && !dfg.nodes[i].is_output && is_ls_op(dfg.nodes[i].op)){
            num_ls++;
        }

        if(dfg.nodes[i].is_output){
            num_outputs++;
        }
    }

    uint32_t rows_needed = 0;
    if(num_inputs > num_outputs){
        rows_needed = num_inputs;
    }else{
        rows_needed = num_outputs;
    }

    if(num_ls > rows_needed){
        rows_needed = num_ls;
    }

    return rows_needed;
}

void move_lsus_to_left(sub_grid_t* sub_grid){
    uint32_t next_ls_free_row;
    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            if(is_ls_op(sub_grid->grid_slots[i][j].ou) && j != 0){

                for(int k = 0; k < sub_grid->num_rows; k++){
                    if(!is_ls_op(sub_grid->grid_slots[k][0].ou)){
                        next_ls_free_row = k;
                        break;
                    }
                }

                uint32_t tmp_node_id = sub_grid->grid_slots[i][j].node_id;
                ou_t tmp_ou = sub_grid->grid_slots[i][j].ou;

                sub_grid->grid_slots[i][j].node_id = sub_grid->grid_slots[next_ls_free_row][0].node_id;
                sub_grid->grid_slots[i][j].ou = sub_grid->grid_slots[next_ls_free_row][0].ou;

                sub_grid->grid_slots[next_ls_free_row][0].node_id = tmp_node_id;
                sub_grid->grid_slots[next_ls_free_row][0].ou = tmp_ou;
            }
        }
    }
}

bool scatter_dfg(sub_grid_t* sub_grid, dfg_t dfg){

    uint32_t next_grid_slot = 0;
    uint32_t next_input_io_unit = 0;
    uint32_t next_output_io_unit = 0;

    for(int i = 0; i <= sub_grid->num_rows; i++){
        sub_grid->io_unit_cfgs[i].wait_for_ls_submit = false;
    }

    for(int i = 0; i < dfg.num_nodes; i++){
        uint32_t row;
        uint32_t col;
        grid_slot_to_coord(next_grid_slot, &row, &col);

        if(row > sub_grid->num_rows){
            return false;
        }

        if(dfg.nodes[i].is_input && dfg.nodes[i].reg){
            sub_grid->io_unit_cfgs[next_input_io_unit].inp_node_id = dfg.nodes[i].node_id;
            sub_grid->io_unit_cfgs[next_input_io_unit].is_inp = true;
            sub_grid->io_unit_cfgs[next_input_io_unit].is_reg = true;
            sub_grid->io_unit_cfgs[next_input_io_unit].inp_value = dfg.nodes[i].inp_value;
            next_input_io_unit++;
        }else if(dfg.nodes[i].is_input && !dfg.nodes[i].reg && dfg.nodes[i].is_literal_input_used){
            sub_grid->io_unit_cfgs[next_input_io_unit].inp_node_id = dfg.nodes[i].node_id;
            sub_grid->io_unit_cfgs[next_input_io_unit].is_inp = true;
            sub_grid->io_unit_cfgs[next_input_io_unit].is_reg = false;
            sub_grid->io_unit_cfgs[next_input_io_unit].inp_value = dfg.nodes[i].inp_value;
            next_input_io_unit++;
        }else if(!dfg.nodes[i].is_output){
            sub_grid->grid_slots[row][col].node_id = dfg.nodes[i].node_id;
            sub_grid->grid_slots[row][col].ou = dfg.nodes[i].op;
            next_grid_slot++;
        }

        if(dfg.nodes[i].is_output){
            sub_grid->io_unit_cfgs[next_output_io_unit].out_node_id = dfg.nodes[i].node_id;
            sub_grid->io_unit_cfgs[next_output_io_unit].output_reg_addr = dfg.nodes[i].inp_value; //will be holding output reg address
            sub_grid->io_unit_cfgs[next_output_io_unit].is_output = true;
            next_output_io_unit++;
        }       
    }

    move_lsus_to_left(sub_grid);

    return true;
}


uint32_t calculate_gaps_cost(sub_grid_t* sub_grid, dfg_t dfg){
    uint32_t gap_cost = 0;
    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            if(sub_grid->grid_slots[i][j].ou == PASSTHROUGH){
                gap_cost++;
            }
        }
    }

    return  gap_cost;
}

void find_io_unit(sub_grid_t* sub_grid, uint32_t node_id, uint32_t* pos, bool output){
    for(int i = 0; i <= sub_grid->num_rows; i++){
        if(output){
            if(sub_grid->io_unit_cfgs[i].out_node_id == node_id && sub_grid->io_unit_cfgs[i].is_output){            
                *pos = i;
                return;
            }
        }else{
            if(sub_grid->io_unit_cfgs[i].inp_node_id == node_id&& sub_grid->io_unit_cfgs[i].is_inp){            
                *pos = i;
                return;
            }
        }
        
    }

    printf("Couldn't find IO Unit with node ID %u (Output: %u) \n\r", node_id, output);
    while(1);
}

void find_node_pos(sub_grid_t* sub_grid, uint32_t node_id, uint32_t* row, uint32_t* col, ou_t node_ou){
    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            if(sub_grid->grid_slots[i][j].node_id == node_id && sub_grid->grid_slots[i][j].ou == node_ou){
                *row = i;
                *col = j;
                return;
            }
        }
    }

    printf("Couldn't find node with ID %u \n\r", node_id);
    while(1);
}

bool create_passthrough(sub_grid_t* sub_grid, uint32_t from_node_id, uint32_t on_row, bool* passthrough_created, uint32_t* passthrough_col){
        //check if row has passthrough for from node       
        *passthrough_created = false;
        for(int i = 0; i < NUM_GRID_COLS; i++){
            if(sub_grid->grid_slots[on_row][i].ou == PASSTHROUGH && sub_grid->grid_slots[on_row][i].node_id == from_node_id){
                return true;
            }
        }

        //Try to create passthrough for from node in row if passthrough is not found
        for(int i = 0; i < NUM_GRID_COLS; i++){
            if(sub_grid->grid_slots[on_row][i].ou == UNUSED){
                *passthrough_created = true;
                *passthrough_col = i;
                return true;
            }
        }

        return false;
}

bool find_path(sub_grid_t* sub_grid, uint32_t from_node_row, uint32_t to_node_row, uint32_t from_node_id, bool from_node_is_input, bool to_node_is_output){

    // printf("Finding path between row %u and %u \n\r", from_node_row, to_node_row);
    uint32_t row = from_node_row;
    bool path_found = false;    

    // source/destination OU is on the same row as IO unit
    if(from_node_row > to_node_row){
        // printf("From node is lower than to node - can't make path\n\r");
        path_found = false;
        return path_found;
    }
    if(to_node_row == from_node_row && (from_node_is_input || to_node_is_output)){
            // printf("Found path from/to IO unit on same row\n\r");
            path_found = true;
            return path_found;
    }

    if(to_node_row == from_node_row && !(from_node_is_input || to_node_is_output)){//if both OU nodes are on same row then they should've been picked up by LSI, if not, then path cannot be found
            // printf("Two OUs on same row with no LSI\n\r");
            path_found = false;
            return path_found;
    }

    //loop should only be entered for connections not on same row
    bool passthrough_created[NUM_GRID_ROWS];
    uint32_t passthrough_col[NUM_GRID_ROWS];

    // printf("loop\n\r");
    for(int i = 0; i < sub_grid->num_rows; i++){
        passthrough_created[i] = false;
    }

    if(from_node_is_input){
        create_passthrough(sub_grid, from_node_id, row, &passthrough_created[row], &passthrough_col[row]);
    }

    while(!path_found){
        row++;
        // printf("Checking row %u\n\r", row);

        if(row >= sub_grid->num_rows) {
            // printf("Grid size exceeded without finding row, breaking out of loop\n\r");
            break; //grid size exceeded
        }      
        
        if(row == to_node_row){
            // printf("Found path!\n\r");
            path_found = true;           
            break;
        }   

        bool pt_found = create_passthrough(sub_grid, from_node_id, row, &passthrough_created[row], &passthrough_col[row]);

        //if couldn't create a passthrough exit loop with path found as false
        if(!pt_found) break;
    }

    if(to_node_is_output && path_found){//need to create passthrough for output IO unit on its row 
        create_passthrough(sub_grid, from_node_id, row, &passthrough_created[row], &passthrough_col[row]);
    }

    //apply passthroughs if a path has been found
    if(path_found){
        // printf("Applying Passthroughs\n\r");
        for(int i = 0; i < sub_grid->num_rows; i++){
            // printf("Checking for passthrough in row %u\n\r", i);
            if(passthrough_created[i]){
                // printf("Passthrough added in row %u in column %u\n\r", i, passthrough_col[i]);
                sub_grid->grid_slots[i][passthrough_col[i]].node_id = from_node_id;
                sub_grid->grid_slots[i][passthrough_col[i]].ou = PASSTHROUGH;
            }
        }
    }    

    // free(passthrough_created);
    // free(passthrough_col);

    // printf("Path found %u\n\r", path_found);

    return path_found;
}

//will make changes to sub grid
uint32_t unconnected_nets_cost(sub_grid_t* sub_grid, dfg_t dfg){
    //filter feedback paths

    uint32_t cost = 0;

    for(int i = 0; i < dfg.num_edges; i++){
        bool edge_made = false;
        uint32_t from_node = dfg.edges[i].fromNode;
        uint32_t to_node = dfg.edges[i].toNode;

        uint32_t from_node_row, from_node_col;
        uint32_t to_node_row, to_node_col;
    
        

        if(dfg.nodes[from_node-1].is_input){
            find_io_unit(sub_grid, from_node, &from_node_row, false);
            find_node_pos(sub_grid, to_node, &to_node_row, &to_node_col, dfg.nodes[to_node-1].op);
        }

        if(dfg.nodes[to_node-1].is_output){
            find_node_pos(sub_grid, from_node, &from_node_row, &from_node_col, dfg.nodes[from_node-1].op);
            find_io_unit(sub_grid, to_node, &to_node_row, true);
        }

        if(!dfg.nodes[to_node-1].is_output && !dfg.nodes[from_node-1].is_input){
            find_node_pos(sub_grid, from_node, &from_node_row, &from_node_col, dfg.nodes[from_node-1].op);
            find_node_pos(sub_grid, to_node, &to_node_row, &to_node_col, dfg.nodes[to_node-1].op);

            //check for LSI datapath if edge doesn't involve IO units
            if(from_node_row == to_node_row && (from_node_col+1) == to_node_col){
                edge_made = true;
                // printf("LSI Connection found between nodes %u and %u\n\r", from_node, to_node);
            }    
        }

        if(!edge_made){
            edge_made = find_path(sub_grid, from_node_row, to_node_row, from_node, dfg.nodes[from_node-1].is_input, dfg.nodes[to_node-1].is_output);
            // printf("Path found from node %u to %u? : %u\n\r", from_node, to_node, edge_made);
        }

        if(!edge_made){
            cost++;
        }
    }

    return cost;
}

uint32_t calculate_total_cost(sub_grid_t* sub_grid, dfg_t dfg){
    uint32_t uconn_nets_cost = unconnected_nets_cost(sub_grid, dfg);
    uint32_t gaps_cost = calculate_gaps_cost(sub_grid, dfg);
    // print_sub_grid(sub_grid);

    // printf("Gaps Cost: %u, Uconn Nets Cost %u \n\r", gaps_cost, uconn_nets_cost);
    //("%u\n\r", uconn_nets_cost);
    // if(uconn_nets_cost < 2) print_sub_grid(sub_grid);
    // return gaps_cost+uconn_nets_cost;
    return uconn_nets_cost;
}

uint32_t get_randint_in_range(int32_t upper, int32_t lower){
    int32_t range_size = upper - lower + 1;
    // printf("Generating random int between %d and %d, range : %d\n\r", upper, lower, range_size);
    return (rand() % range_size) + lower;
}

void make_random_change_ou(sub_grid_t* sub_grid, uint32_t swap_dist_row, uint32_t swap_dist_col){
    //select co-ordinates of units for swapping
    uint32_t row = rand() % sub_grid->num_rows;
    uint32_t col = rand() % NUM_GRID_COLS;

    while(sub_grid->grid_slots[row][col].ou == UNUSED){
        row = rand() % sub_grid->num_rows;
        col = rand() % NUM_GRID_COLS;
    }

    int32_t next_row_ub = row+swap_dist_row;
    int32_t next_row_lb = row-swap_dist_row;

    int32_t next_col_ub = col+swap_dist_col;
    int32_t next_col_lb = row-swap_dist_col;

    if(next_row_lb < 0){
        next_row_lb = 0;
    }

    if(next_row_ub >= sub_grid->num_rows){
        next_row_ub = sub_grid->num_rows - 1;
    }

    if(next_col_lb < 0){
        next_col_lb = 0;
    }

    if(next_col_ub >= NUM_GRID_COLS){
        next_col_ub = NUM_GRID_COLS-1;
    }

    // printf("Getting First Unit to Swap: row: %u, col: %u\n\r", row, col);
    pr_slot_t unit1 = sub_grid->grid_slots[row][col];
    uint32_t next_row;
    uint32_t next_col;
    
    next_col = get_randint_in_range(next_col_ub, next_col_lb);
    next_row = get_randint_in_range(next_row_ub, next_row_lb);
    // printf("Getting Second Unit to Swap: row: %u, col: %u\n\r", next_row, next_col);
    pr_slot_t unit2 = sub_grid->grid_slots[next_row][next_col];

    //if we are swapping an LS OU, it must be in column 0
    if(is_ls_op(unit1.ou) || is_ls_op(unit2.ou)){
        // printf("Swapping LS OU, setting columns to 0");
        col = 0;
        next_col = 0;
        unit1 = sub_grid->grid_slots[row][col];
        unit2 = sub_grid->grid_slots[next_row][next_col];

        // sub_grid->io_unit_cfgs[row].wait_for_ls_submit = is_ls_op(unit2.ou);
        // sub_grid->io_unit_cfgs[next_col].wait_for_ls_submit = is_ls_op(unit1.ou);
    }

    //if we are swapping a passthrough, just delete it
    if(unit1.ou == PASSTHROUGH){
        unit1.ou = UNUSED;
        unit1.node_id = 0;
    }

    if(unit2.ou == PASSTHROUGH){
        unit2.ou = UNUSED;
        unit2.node_id = 0;
    }
    
    sub_grid->grid_slots[row][col] = unit2;
    sub_grid->grid_slots[next_row][next_col] = unit1; 
    // printf("OU Swap Complete! \n\r");

    // print_sub_grid(sub_grid);
}

void make_random_change_io(sub_grid_t* sub_grid, uint32_t swap_dist_row){
    //select co-ordinates of units for swapping x2 - one for input node and one for output node
    uint32_t row1 = rand() % sub_grid->num_rows;
    uint32_t row2 = rand() % sub_grid->num_rows;

    int32_t next_row1_ub = row1+swap_dist_row;
    int32_t next_row1_lb = row1-swap_dist_row;

    int32_t next_row2_ub = row2+swap_dist_row;
    int32_t next_row2_lb = row2-swap_dist_row;

    if(next_row1_lb < 0){
        next_row1_lb = 0;
    }

    if(next_row2_lb < 0){
        next_row2_lb = 0;
    }

    if(next_row1_ub >= sub_grid->num_rows){
        next_row1_ub = sub_grid->num_rows - 1;
    }

    if(next_row2_ub >= sub_grid->num_rows){
        next_row2_ub = sub_grid->num_rows - 1;
    }

    io_unit_cfg_t unit1_inp = sub_grid->io_unit_cfgs[row1];
    uint32_t next_row1;

    next_row1 = get_randint_in_range(next_row1_ub, next_row1_lb);
    io_unit_cfg_t unit2_inp = sub_grid->io_unit_cfgs[next_row1];

    io_unit_cfg_t unit1_out = sub_grid->io_unit_cfgs[row2];
    uint32_t next_row2;

    next_row2 = get_randint_in_range(next_row2_ub, next_row2_lb);
    io_unit_cfg_t unit2_out = sub_grid->io_unit_cfgs[next_row2];

    //swap input config of 2 nodes
    sub_grid->io_unit_cfgs[row1].inp_node_id = unit2_inp.inp_node_id;
    sub_grid->io_unit_cfgs[row1].is_inp = unit2_inp.is_inp;
    sub_grid->io_unit_cfgs[row1].is_reg = unit2_inp.is_reg;
    sub_grid->io_unit_cfgs[row1].inp_value = unit2_inp.inp_value;

    sub_grid->io_unit_cfgs[next_row1].inp_node_id = unit1_inp.inp_node_id;
    sub_grid->io_unit_cfgs[next_row1].is_inp = unit1_inp.is_inp;
    sub_grid->io_unit_cfgs[next_row1].is_reg = unit1_inp.is_reg;
    sub_grid->io_unit_cfgs[next_row1].inp_value = unit1_inp.inp_value;

    //swap output config of 2 nodes
    sub_grid->io_unit_cfgs[row2].out_node_id = unit2_out.out_node_id;
    sub_grid->io_unit_cfgs[row2].is_output = unit2_out.is_output;
    sub_grid->io_unit_cfgs[row2].output_reg_addr = unit2_out.output_reg_addr;

    sub_grid->io_unit_cfgs[next_row2].out_node_id = unit1_out.out_node_id;
    sub_grid->io_unit_cfgs[next_row2].is_output = unit1_out.is_output;
    sub_grid->io_unit_cfgs[next_row2].output_reg_addr = unit1_out.output_reg_addr;
}


void make_random_change(sub_grid_t* sub_grid, uint32_t swap_dist_row, uint32_t swap_dist_col){
    make_random_change_ou(sub_grid, swap_dist_row, swap_dist_col);
    make_random_change_io(sub_grid, swap_dist_row);
    // print_sub_grid(sub_grid);
}

void init_sub_grid(sub_grid_t* grid, uint32_t num_rows){
    grid->num_rows = num_rows;
    grid->grid_slots = malloc(grid->num_rows*sizeof(pr_row_t));
    grid->io_unit_cfgs = malloc((grid->num_rows + 1)*sizeof(io_unit_cfg_t));
    grid->row_xbar_cfgs = malloc(grid->num_rows*sizeof(row_xbar_cfg_t));

    for(int i = 0; i < grid->num_rows; i++){
        pr_slot_t* col_slot_arr = malloc(NUM_GRID_COLS*sizeof(pr_slot_t));
        grid->grid_slots[i] = col_slot_arr;
    }
}

//NOTE: does not copy row_xbar_cfgs
void copy_sub_grid(sub_grid_t* dest, sub_grid_t* src){
    memcpy(dest->io_unit_cfgs, src->io_unit_cfgs, (dest->num_rows + 1)*sizeof(io_unit_cfg_t));

    for(int i = 0; i < dest->num_rows; i++){
        memcpy(dest->grid_slots[i], src->grid_slots[i], NUM_GRID_COLS*sizeof(pr_slot_t));
    }
}

void free_sub_grid(sub_grid_t* grid){
    for(int i = 0; i < grid->num_rows; i++){
        free(grid->grid_slots[i]);
    }

    free(grid->row_xbar_cfgs);
    free(grid->io_unit_cfgs);
    free(grid->grid_slots);
}


float calculate_std_dev(uint32_t* costs, uint32_t num_costs){
    float sum, mean, std;
    sum = 0.0;
    std = 0.0;

    for(int i = 0; i < num_costs; i++){
        sum += costs[i];
    }

    mean = sum/((float) num_costs);

    for(int i = 0; i < num_costs; i++){
        std += powf(((float) costs[i]) - mean, 2);
    }

    std = sqrtf(std/((float) num_costs));
    return std;
}

float select_init_temp(sub_grid_t* sub_grid, dfg_t dfg){
    uint32_t* cost_arr = malloc(dfg.num_nodes*sizeof(uint32_t));
    uint32_t cost;

    sub_grid_t grid_cpy;
    init_sub_grid(&grid_cpy, sub_grid->num_rows);
    
    for(int i = 0; i < dfg.num_nodes; i++){
        make_random_change(sub_grid, sub_grid->num_rows, NUM_GRID_COLS);
        copy_sub_grid(&grid_cpy, sub_grid);
        cost = calculate_total_cost(&grid_cpy, dfg);
        cost_arr[i] = cost;
    }

    // printf("Printing Costs during intial temp selection\n\r");
    // for(int i = 0; i < dfg.num_nodes; i++){
    //     printf("%u\n\r", cost_arr[i]);
    // }

    float std_dev = calculate_std_dev(cost_arr, dfg.num_nodes);

    free_sub_grid(&grid_cpy);
    free(cost_arr);

    return round(20.0*std_dev);
}

bool can_exit_anneal(float curr_temp, sub_grid_t* grid, dfg_t dfg){
    if(curr_temp < ANNEAL_TEMP_THRESH) return true;

    sub_grid_t temp_grid;
    init_sub_grid(&temp_grid, grid->num_rows);
    copy_sub_grid(&temp_grid, grid);

    uint32_t unconn_net_cost = unconnected_nets_cost(&temp_grid, dfg);
    free_sub_grid(&temp_grid);
    return (unconn_net_cost == 0);
}

float new_temp_modifier(float prop_accepted){
    if(prop_accepted > 0.96f){
        return 0.5f;
    }else if(prop_accepted > 0.8f && prop_accepted <= 0.96f){
        return 0.9f;
    }else if(prop_accepted > 0.15f && prop_accepted <= 0.8f){
        return 0.95f;
    }else{
        return 0.8f;
    }
}

float new_swap_dist_modifier(float prop_accepted){
    return 1.0 - 0.44 + prop_accepted;
}

uint32_t anneal(sub_grid_t* grid, dfg_t dfg, uint32_t iters_per_temp, uint32_t init_swap_dist_row, uint32_t init_swap_dist_col, float init_temp){
    sub_grid_t temp_grid;
    init_sub_grid(&temp_grid, grid->num_rows);
    copy_sub_grid(&temp_grid, grid);

    uint32_t curr_cost = calculate_total_cost(&temp_grid, dfg);

    float temp = init_temp;

    uint32_t swap_dist_row = init_swap_dist_row;
    uint32_t swap_dist_col = init_swap_dist_col;
    
    uint32_t new_cost;
    uint32_t n_accepted;
    int32_t cost_diff;

    float acceptance_prob;
    float random_num;
    float prop_accepted;
    float swap_dist_mod;

    while(!can_exit_anneal(temp, grid, dfg)){
        printf("Current Temperature %f\n\r", temp);
        n_accepted = 0;

        for(int i = 0; i < iters_per_temp; i++){
            copy_sub_grid(&temp_grid, grid);
            make_random_change(&temp_grid, swap_dist_row, swap_dist_col);
            new_cost = calculate_total_cost(&temp_grid, dfg);

            cost_diff = new_cost - curr_cost;
            acceptance_prob = expf(-((float) cost_diff)/temp);
            random_num = (float)rand()/(float) RAND_MAX;

            if(cost_diff < 0 || random_num < acceptance_prob){
                copy_sub_grid(grid, &temp_grid);
                curr_cost = new_cost;
                n_accepted++;
                // if(cost_diff < 0){
                //     printf("ac\n\r");
                // }else if(random_num < acceptance_prob){
                //     printf("cd:%d, ar r:%.10f, ap:%.10f\n\r", cost_diff, random_num, acceptance_prob);
                // }
                
            }
        }

        prop_accepted = (float) n_accepted/ (float) iters_per_temp;
        temp = new_temp_modifier(prop_accepted) * temp;
        swap_dist_mod = new_swap_dist_modifier(prop_accepted);
        swap_dist_row = round(((float) swap_dist_row) * swap_dist_mod);
        swap_dist_col = round(((float) swap_dist_col) * swap_dist_mod);
    }

    free_sub_grid(&temp_grid);

    return curr_cost;
}

int32_t find_pt_col(sub_grid_t* sub_grid, uint32_t on_row, uint32_t node_id){
    for(int i = 0; i < NUM_GRID_COLS; i++){
        if(sub_grid->grid_slots[on_row][i].ou == PASSTHROUGH && sub_grid->grid_slots[on_row][i].node_id == node_id){
            return i;
        }
    }

    return -1;
}

bool infer_mux_cfgs(sub_grid_t* sub_grid, dfg_t dfg){
    sub_grid->row_xbar_cfgs = malloc(sub_grid->num_rows*sizeof(row_xbar_cfg_t));

    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            sub_grid->row_xbar_cfgs[i].arr[0][j] = 0;
            sub_grid->row_xbar_cfgs[i].arr[1][j] = 0;
        }
    }
    
    for(int i = 0; i < dfg.num_edges; i++){
        uint32_t from_node_row, from_node_col;
        uint32_t to_node_row, to_node_col;  

         printf("Edge from: %u, to: %u, slot input: %u\n\r",
        dfg.edges[i].fromNode,
        dfg.edges[i].toNode,
        dfg.edges[i].slot_inp
        );


        bool from_node_is_input = dfg.nodes[dfg.edges[i].fromNode-1].is_input;
        bool to_node_is_output = dfg.nodes[dfg.edges[i].toNode-1].is_output;

        if(from_node_is_input){
            find_io_unit(sub_grid, dfg.edges[i].fromNode, &from_node_row, false);
        }else{
            find_node_pos(sub_grid, dfg.edges[i].fromNode, &from_node_row, &from_node_col, dfg.nodes[dfg.edges[i].fromNode-1].op);
        }
        
        if(to_node_is_output){
            find_io_unit(sub_grid, dfg.edges[i].toNode, &to_node_row, true);
        }else{
            find_node_pos(sub_grid, dfg.edges[i].toNode, &to_node_row, &to_node_col, dfg.nodes[dfg.edges[i].toNode-1].op);
        }        

        //check for LSI path
        if(!from_node_is_input && !to_node_is_output && from_node_row == to_node_row && (from_node_col+1) == to_node_col){
            sub_grid->row_xbar_cfgs[to_node_row].arr[dfg.edges[i].slot_inp][to_node_col] = LSI;
            continue;
        }

        if(to_node_row == from_node_row && from_node_is_input){
            sub_grid->row_xbar_cfgs[to_node_row].arr[dfg.edges[i].slot_inp][to_node_col] = IO_UNIT;
            continue;
        }

        if(to_node_row == from_node_row && to_node_is_output){
            sub_grid->io_unit_cfgs[to_node_row].io_row_mux_inp = from_node_col;
            continue;
        }

        uint32_t row = from_node_row;
        bool path_found = false;
        int32_t pt_col = -1;

        if(from_node_is_input){
            pt_col = find_pt_col(sub_grid, row, dfg.edges[i].fromNode);
            if(pt_col == -1){
                printf("Couldn't find first passthrough for input IO unit \n\r");
                return false;
            }else{
                sub_grid->row_xbar_cfgs[row].arr[PT_SLOT_INPUT][pt_col] = IO_UNIT;
            }
        }
        while(!path_found){
            row++;

            if(row >= sub_grid->num_rows){
                printf("Error inferring xbar cfgs, sub grid rows exceeded \n\r");
                return false;
            }            

            if(row == to_node_row){
                path_found = true;
                if(!to_node_is_output){
                    if(pt_col != -1){
                        sub_grid->row_xbar_cfgs[row].arr[dfg.edges[i].slot_inp][to_node_col] = pt_col;
                    }else{
                        sub_grid->row_xbar_cfgs[row].arr[dfg.edges[i].slot_inp][to_node_col] = from_node_col;
                    }
                }else{
                    uint32_t op_pt_col = find_pt_col(sub_grid, row, dfg.edges[i].toNode);
                    if(op_pt_col != -1){
                        if(pt_col != -1){
                            sub_grid->row_xbar_cfgs[row].arr[PT_SLOT_INPUT][op_pt_col] = pt_col;
                        }else{
                            sub_grid->row_xbar_cfgs[row].arr[PT_SLOT_INPUT][op_pt_col] = from_node_col;
                        }

                        pt_col = op_pt_col;
                    }else{
                        printf("Error inferring XBAR CFGs: Couldn't find output PT\n\r");
                        return false;
                    }
                }
                break;
            }

            int32_t next_pt_col = find_pt_col(sub_grid, row,  dfg.edges[i].fromNode);
            if(next_pt_col != -1){
                if(pt_col != -1){
                    sub_grid->row_xbar_cfgs[row].arr[PT_SLOT_INPUT][next_pt_col] = pt_col;

                }else if(row == (from_node_row+1)){
                    sub_grid->row_xbar_cfgs[row].arr[PT_SLOT_INPUT][next_pt_col] = from_node_col;
                }
                pt_col = next_pt_col;
            }else{
                printf("Error inferring xbar cfgs, couldn't find a required passthrough from %u to %u on row %u\n\r", dfg.edges[i].fromNode, dfg.edges[i].toNode, row);
                print_sub_grid(sub_grid);
                return false;
            }
        }

        if(path_found && to_node_is_output){
            sub_grid->io_unit_cfgs[to_node_row].io_row_mux_inp = pt_col;
        }

    }

    return true;
}

void convert_pts_to_and(sub_grid_t* sub_grid){
    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            if(sub_grid->grid_slots[i][j].ou == PASSTHROUGH){
                sub_grid->grid_slots[i][j].ou = AND; //passthrough module not available so set to and
                sub_grid->row_xbar_cfgs[i].arr[0][j] = sub_grid->row_xbar_cfgs[i].arr[1][j]; //set both inputs to same
            }
        }
    }
}

void infer_ls_waits(sub_grid_t* sub_grid){
    for(int i = 0; i < sub_grid->num_rows; i++){
        sub_grid->io_unit_cfgs[i].wait_for_ls_submit = is_ls_op(sub_grid->grid_slots[i][0].ou);
    }
}

bool gen_sub_grid(dfg_t dfg, sub_grid_t* sub_grid){
    ("Generating Sub Grid\n\r");
    //allocate memory
    init_sub_grid(sub_grid, sub_grid->num_rows);

    //initialise grid slots and io units
    for(int i = 0; i < sub_grid->num_rows; i++){
        sub_grid->io_unit_cfgs[i].is_inp = false;
        sub_grid->io_unit_cfgs[i].is_output = false;
        sub_grid->io_unit_cfgs[i].wait_for_ls_submit = false;

        for(int j = 0; j < NUM_GRID_COLS; j++){
            sub_grid->grid_slots[i][j].ou = UNUSED;
            sub_grid->grid_slots[i][j].node_id = 0; //id 0 will never be used by a dfg node
        }
    }

    //scatter DFG into grid
    print_dfg(dfg);
    ("Scattering DFG\n\r");
    scatter_dfg(sub_grid, dfg);
    print_sub_grid(sub_grid);
    //do annealing
    ("Selecting intial Temp\n\r");
    float init_temp = select_init_temp(sub_grid, dfg);    
    uint32_t iters_per_temp = round(10*(powf((float) dfg.num_nodes, 1.1f)));
    //("Initial temp selected: %f, iters per temp: %u, performing annealing\n\r", init_temp, iters_per_temp);
    uint32_t final_cost = anneal(sub_grid, dfg, iters_per_temp, sub_grid->num_rows, NUM_GRID_COLS, init_temp);

    //check if there were any unconnected nets
    sub_grid_t temp_grid;
    init_sub_grid(&temp_grid, sub_grid->num_rows);
    copy_sub_grid(&temp_grid, sub_grid);

    uint32_t unconn_nets = unconnected_nets_cost(&temp_grid, dfg);
    ("After Annealing num uconn nets: %u\n\r", unconn_nets);
    if(unconn_nets > 0){
        free_sub_grid(sub_grid);
        free_sub_grid(&temp_grid);
        return false;
    }

    //if all nets are connectable, infer the grid mux configs
    ("Inferring Configuration\n\r");
    infer_mux_cfgs(sub_grid, dfg);
    convert_pts_to_and(sub_grid);
    infer_ls_waits(sub_grid);

    ("Configuration inferred\n\r");
    print_sub_grid(sub_grid);
    print_mux_cfgs(sub_grid);

    free_sub_grid(&temp_grid);
    return true;
}


void print_sub_grid(sub_grid_t* sub_grid){
    printf("Printing Sub Grid\n\r");

    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            printf("ID:%u OU:%u\t", sub_grid->grid_slots[i][j].node_id, sub_grid->grid_slots[i][j].ou);
            
        }
        printf("I? %u O? %u I:%u O:%u\n\r", sub_grid->io_unit_cfgs[i].is_inp, sub_grid->io_unit_cfgs[i].is_output, sub_grid->io_unit_cfgs[i].inp_node_id, sub_grid->io_unit_cfgs[i].out_node_id);
    }
}

void print_mux_cfgs(sub_grid_t* sub_grid){
    for(int i = 0; i < sub_grid->num_rows; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            printf("m_in0: %u m_in1: %u\t", sub_grid->row_xbar_cfgs[i].arr[0][j], sub_grid->row_xbar_cfgs[i].arr[1][j]);
        }
        printf("io_row_in:%u\n\r",sub_grid->io_unit_cfgs[i].io_row_mux_inp);
    }
}