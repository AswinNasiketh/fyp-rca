#include "seqprof.h"
#include "grid_manager.h"

seq_profile_t profile_seq(instr_seq_t* seq, profiler_entry_t* profiler_entry){
    dfg_t* dfg = malloc(sizeof(dfg_t));

    *dfg = create_dfg(seq);

    uint32_t cpu_loop_iteration_latency = 0;
    uint32_t output_node_latency;
    for(int i = 0; i < (*dfg).num_nodes; i++){
        if((*dfg).nodes[i].is_output || ((!(*dfg).nodes[i].is_input && !(*dfg).nodes[i].is_output) && ((*dfg).nodes[i].op == SW) || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SB)){
            // printf("Finding CPU Depth of Node %u\n\r", i);
            output_node_latency = find_node_depth_cpu((*dfg), (*dfg).nodes[i].node_id, false);
            if(output_node_latency > cpu_loop_iteration_latency){
                cpu_loop_iteration_latency = output_node_latency;
            }
        }
    }

    uint32_t acc_loop_iteration_latency = 0;
    uint32_t num_ls_ops = 0;
    bool store_ops = false;

    for(int i = 0; i < (*dfg).num_nodes; i++){
        if((*dfg).nodes[i].op == LB || (*dfg).nodes[i].op == LBU|| (*dfg).nodes[i].op == LH || (*dfg).nodes[i].op == LHU || (*dfg).nodes[i].op == LW || (*dfg).nodes[i].op == SB || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SW){
            num_ls_ops++;
            if((*dfg).nodes[i].op == SB || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SW){
                store_ops = true;
            }
        }
    }

    for(int i = 0; i < (*dfg).num_nodes; i++){
        if(((*dfg).nodes[i].is_output && (*dfg).nodes[i].is_input) || ((!(*dfg).nodes[i].is_input && !(*dfg).nodes[i].is_output) && ((*dfg).nodes[i].op == SW) || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SB)){
            output_node_latency = find_node_depth_acc((*dfg), (*dfg).nodes[i].node_id, num_ls_ops, false);
            // printf("Output node ID %u, accelerator latency: %u\n\r", (*dfg).nodes[i].node_id, output_node_latency);
            if(output_node_latency > acc_loop_iteration_latency){
                acc_loop_iteration_latency = output_node_latency;
            }
        }
    }

    if(store_ops){
        acc_loop_iteration_latency += num_ls_ops+1;
    }

    uint32_t pr_time = calc_pr_time((*dfg));

    seq_profile_t seq_prof;

    seq_prof.loop_start_addr = seq->loop_start_addr;
    seq_prof.num_instrs = seq->num_instrs;
    seq_prof.taken_count = profiler_entry->taken_count;

    seq_prof.cpu_time_per_iter = cpu_loop_iteration_latency;
    seq_prof.acc_time_per_iter = acc_loop_iteration_latency;
    seq_prof.pr_time = pr_time;
    seq_prof.dfg = dfg;
    seq_prof.is_accelerated = false;

    seq_prof.num_annealing_attempts = 0;
    seq_prof.sub_grid = malloc(sizeof(sub_grid_t));
    seq_prof.sub_grid->num_rows = estimate_grid_size(*dfg);
    seq_prof.is_grid_implemented = false;

    if(seq_prof.sub_grid->num_rows > NUM_GRID_ROWS){
        printf("Sequence can't fit on grid, setting supported to false\n\r");
        seq_prof.num_annealing_attempts = MAX_ANNEALING_ATTEMPTS; //do not attempt to build grid
        seq_prof.seq_supported = false;
    }

    return seq_prof;
}




int32_t calc_advantage(seq_profile_t seq_prof){
    return (seq_prof.in_hw_profiler && seq_prof.seq_supported) ? seq_prof.cpu_time_per_iter - seq_prof.acc_time_per_iter : 0x80000000; //max negative number
}

//ascending order
void sort_profiles_by_advantage(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_valid_profs){
    bool stable = false;
    seq_profile_t temp;
    int32_t advantage;
    int32_t advantage_prev;

    while(!stable){
        stable = true;
        advantage_prev = calc_advantage(seq_profs[0]);

        for(int i = 1; i < num_valid_profs; i++){
            advantage = calc_advantage(seq_profs[i]);

            if(advantage < advantage_prev){
                temp = seq_profs[i-1];
                seq_profs[i-1] = seq_profs[i];
                seq_profs[i] = temp;
                stable = false;
            }
            advantage_prev = advantage;
        }
    }
}

//ascending order
void sort_profiles_by_improvement(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_valid_profs){
    bool stable = false;
    seq_profile_t temp;
    int32_t improvement;
    int32_t improvement_prev = calc_improvement(seq_profs[0]);

    while(!stable){
        stable = true;
        improvement_prev = calc_improvement(seq_profs[0]);

        for(int i = 1; i < num_valid_profs; i++){
            improvement = calc_advantage(seq_profs[i]);

            if(improvement < improvement_prev){
                temp = seq_profs[i-1];
                seq_profs[i-1] = seq_profs[i];
                seq_profs[i] = temp;
                stable = false;
            }
            improvement_prev = improvement;
        }
    }
}

int32_t calc_improvement(seq_profile_t seq_prof){
    int32_t adv = calc_advantage(seq_prof);
    if(adv == 0x80000000){
        return adv;
    }else{
        return adv * seq_prof.taken_count;
    }
}

//ensures grid is impelemented
uint32_t get_area_cost(seq_profile_t* seq){
    if(seq->is_grid_implemented){
        return seq->sub_grid->num_rows;
    }else{
        if(seq->num_annealing_attempts < MAX_ANNEALING_ATTEMPTS && seq->seq_supported){
            printf("Attempting to generate sub grid through simulated annealing \n\r");
            if(gen_sub_grid(*(seq->dfg), seq->sub_grid)){
                printf("Sub grid generation succeeded \n\r");
                seq->is_grid_implemented = true;
                return seq->sub_grid->num_rows;
            }else{
                printf("Sub grid generation failed \n\r");
                seq->num_annealing_attempts++;
                return 0;
            }
        }else{
            printf("Max anneal attempts for this sequence reached or sequence is unsupported, returning 0 area \n\r");
            return 0;
        }        
    }
}

int32_t calc_improvement_diff(seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t num_acc_seqs, seq_profile_t* potential_seq_to_accelerate, uint32_t* num_accs_to_replace){
    uint32_t area_cost = get_area_cost(potential_seq_to_accelerate);

    if(area_cost == 0){
        printf("Area is 0, returning 0 improvement difference\n\r");
        return 0;
    }

    uint32_t improvement_gain = calc_improvement(*potential_seq_to_accelerate);

    if(improvement_gain == 0x80000000){
        return 0;
    }

    int32_t extra_rows_required = potential_seq_to_accelerate->sub_grid->num_rows - (NUM_GRID_ROWS - get_next_free_row());
    uint32_t improvement_loss = 0;
    int32_t improvement;
    *num_accs_to_replace = 0;
    //accelerated seqs will be in ascending order sorted by improvement
    //if maximum number of accelerators is being implemented, definitely remove the worst performing accelerator, even if there are free rows available
    if (num_acc_seqs == NUM_RCAS){
        *num_accs_to_replace = 1;
        improvement_loss = calc_improvement(accelerated_seqs[0]);
        improvement_loss = (improvement == 0x80000000) ? 0 : improvement;
        extra_rows_required -= accelerated_seqs[0].sub_grid->num_rows;
    }
    
    //replace accelerators until enough rows are available
    for(int i = *num_accs_to_replace; i < num_acc_seqs; i++){
        if(extra_rows_required <= 0) break;

        improvement = calc_improvement(accelerated_seqs[i]);
        improvement_loss += (improvement == 0x80000000) ? 0 : improvement;
        extra_rows_required -= accelerated_seqs[i].sub_grid->num_rows;
        *num_accs_to_replace++;
    }

    //if the number of rows needed is greater than the number of rows in the grid (unlikely), return -1 improvement diff
    if(extra_rows_required > 0){
        return -1;
    }else{
        return improvement_gain - improvement_loss;
    }
}

void calc_improvement_diffs(seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t num_acc_seqs, seq_profile_t* potential_seqs_to_accelerate, uint32_t num_pot_seqs, int32_t* improvement_diffs, uint32_t* num_accs_to_replace){
    for(int i = 0; i < num_pot_seqs; i++){
        improvement_diffs[i] = calc_improvement_diff(accelerated_seqs, num_acc_seqs, &potential_seqs_to_accelerate[i], &num_accs_to_replace[i]);
        printf("Improvement Diff = %d \n\r", improvement_diffs[i]);
    }
}

uint32_t count_positive_entries(int32_t* arr, uint32_t num_els){
    uint32_t num_pos_entries = 0;
    for(int i = 0; i < num_els; i++){
        if(arr[i] >= 0) num_pos_entries++; //TODO: CHANGE BACK TO STRICTLY POSITIVE
    }
    return num_pos_entries;
}

uint32_t find_max(int32_t* arr, uint32_t num_els){
    int32_t curr_max = arr[0];
    uint32_t curr_max_index = 0;

    for(int i = 1; i < num_els; i++){
        if(arr[i] > curr_max){
            curr_max = arr[i];
            curr_max_index = i;
        }
    }

    return curr_max_index;
}

void get_acc_and_rep_seqs(seq_profile_t* sorted_seq_profs, uint32_t num_seq_profs, seq_profile_t* accelerated_seqs, uint32_t* next_acc_seq_index, seq_profile_t* replacement_seqs, uint32_t* next_replacement_seq_index){
    (*next_acc_seq_index) = 0;
    (*next_replacement_seq_index) = 0;
    for(int i = 0; i < num_seq_profs; i++){
        if(sorted_seq_profs[i].is_accelerated){
            // printf("Seq prof is accelerated \n\r");
            accelerated_seqs[*next_acc_seq_index] = sorted_seq_profs[i];
            (*next_acc_seq_index)++;            
        }else if(sorted_seq_profs[i].in_hw_profiler){
            // printf("Seq prof is in HW Profiler \n\r");
            replacement_seqs[*next_replacement_seq_index] = sorted_seq_profs[i];
            (*next_replacement_seq_index)++;
        }
    }
}

bool set_seq_accelerated(uint32_t loop_start_addr, seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs, bool accelerated){
    int32_t seq_index = find_seq_profile(loop_start_addr, seq_profiles, num_seq_profs);

    if(seq_index == -1) return false;

    seq_profiles[seq_index].is_accelerated = accelerated;
    return true;    
}

void select_accelerators(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs){
    seq_profile_t* temp = malloc(num_seq_profs*sizeof(seq_profile_t));

    // printf("Num Seq Profs %u \n\r", num_seq_profs);
    
    memcpy(temp, &seq_profs[0], num_seq_profs*sizeof(seq_profile_t));

    sort_profiles_by_improvement(temp, num_seq_profs);
    
    // printf("Printing sorted seq profs \n\r");
    for(int i = 0; i < num_seq_profs; i++){
        print_seq_profile(temp[i]);
    }

    seq_profile_t accelerated_seqs[NUM_RCAS];
    uint32_t next_acc_seq_index = 0;

    seq_profile_t replacement_seqs[NUM_PROFILER_ENTRIES];
    uint32_t next_replacement_seq_index = 0;

    get_acc_and_rep_seqs(temp, num_seq_profs, accelerated_seqs, &next_acc_seq_index, replacement_seqs, &next_replacement_seq_index);

    printf("Printing %u Accelerated Seq Profiles \n\r", next_acc_seq_index);
    for(int i = 0; i < next_acc_seq_index; i++){
        print_seq_profile(accelerated_seqs[i]);
    }

    printf("Printing %u Replacement Seq Profiles \n\r", next_replacement_seq_index);
    for(int i = 0; i < next_replacement_seq_index; i++){
        print_seq_profile(replacement_seqs[i]);
    }

    int32_t improvement_diffs[NUM_PROFILER_ENTRIES];
    uint32_t num_accs_to_replace[NUM_PROFILER_ENTRIES];

    calc_improvement_diffs(accelerated_seqs, next_acc_seq_index, replacement_seqs, next_replacement_seq_index, improvement_diffs, num_accs_to_replace);

    // printf("Printing improvement diffs\n\r");
    // for(int i = 0; i < next_replacement_seq_index; i++){
    //     print_seq_profile(replacement_seqs[i]);
    //     printf("Improvement Diff = %d \n\r", improvement_diffs[i]);
    // }

    uint32_t num_pos_entries = count_positive_entries(improvement_diffs, next_replacement_seq_index);
    uint32_t best_seq_index;
    while(num_pos_entries > 0){
        best_seq_index = find_max(improvement_diffs, next_replacement_seq_index);
        replace_accelerators(accelerated_seqs, num_accs_to_replace[best_seq_index], &replacement_seqs[best_seq_index]);

        set_seq_accelerated(replacement_seqs[best_seq_index].loop_start_addr, temp, num_seq_profs, true);
        set_seq_accelerated(replacement_seqs[best_seq_index].loop_start_addr, seq_profs, num_seq_profs, true);

        for(int i = 0;  i < num_accs_to_replace[best_seq_index]; i++){
            set_seq_accelerated(accelerated_seqs[i].loop_start_addr, temp, num_seq_profs, false);
            set_seq_accelerated(accelerated_seqs[i].loop_start_addr, seq_profs, num_seq_profs, false);
        }

        get_acc_and_rep_seqs(temp, num_seq_profs, accelerated_seqs, &next_acc_seq_index, replacement_seqs, &next_replacement_seq_index);
        printf("Num Replacement Seqs %u \n\r", next_replacement_seq_index);
        calc_improvement_diffs(accelerated_seqs, next_acc_seq_index, replacement_seqs, next_replacement_seq_index, improvement_diffs, num_accs_to_replace);
        num_pos_entries = count_positive_entries(improvement_diffs, next_replacement_seq_index);
        printf("Num positive entries %u\n\r", num_pos_entries);
    }

    free(temp);
}

int32_t find_seq_profile(uint32_t loop_start_addr, seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs){
    for(int i = 0; i < num_seq_profs; i++){
        if(seq_profiles[i].loop_start_addr == loop_start_addr){
            return i;
        }
    }
    return -1;
}

void print_seq_profile(seq_profile_t seq_prof){
    printf("Printing Sequence Profile\n\r");
    printf("Loop start address: %x\n\r", seq_prof.loop_start_addr);
    printf("Num Instrs: %u\n\r", seq_prof.num_instrs);
    
    // print_dfg(*(seq_prof.dfg));
    printf("CPU time per iteration: %u\n\r", seq_prof.cpu_time_per_iter);
    printf("Acc time per iteration: %u\n\r", seq_prof.acc_time_per_iter);

    printf("Accelerated? %u\n\r", seq_prof.is_accelerated);
    printf("In HW profiler? %u\n\r", seq_prof.in_hw_profiler);

    printf("PR time: %u\n\r", seq_prof.pr_time);
}