#include "seqprof.h"

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

    uint32_t acc_loop_iteration_latency;
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
        seq_prof.num_annealing_attempts = MAX_ANNEALING_ATTEMPTS; //do not attempt to build grid
        seq_prof.seq_supported = false;
    }

    return seq_prof;
}




int32_t calc_advantage(seq_profile_t seq_prof){
    return (seq_prof.in_hw_profiler && seq_prof.seq_supported) ? seq_prof.cpu_time_per_iter - seq_prof.acc_time_per_iter : 0x80000000; //max negative number
}

void sort_profiles_by_advantage(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_valid_profs){
    bool stable = false;
    seq_profile_t temp;
    int32_t advantage;
    int32_t advantage_prev = calc_advantage(seq_profs[0]);

    while(!stable){
        stable = true;
        for(int i = 1; i < num_valid_profs; i++){
            advantage = calc_advantage(seq_profs[i]);

            if(advantage < advantage_prev){
                temp = seq_profs[i-1];
                seq_profs[i-1] = seq_profs[i];
                seq_profs[i] = temp;
                stable = false;
            }
        }
    }
}

int32_t calc_improvement(seq_profile_t seq_prof){    
    if(seq_prof.in_hw_profiler && seq_prof.seq_supported){
        int32_t adv = calc_advantage(seq_prof);
        return adv * seq_prof.taken_count;
    }else{
        return 0x80000000;
    }
}

uint32_t get_area_cost(seq_profile_t seq){
    if(seq.is_grid_implemented){
        return seq.sub_grid->num_rows;
    }else{
        if(seq.num_annealing_attempts < MAX_ANNEALING_ATTEMPTS){
            if(gen_sub_grid(*seq.dfg, seq.sub_grid)){
                seq.is_grid_implemented = true;
                return seq.sub_grid->num_rows;
            }else{
                seq.num_annealing_attempts++;
                return 0;
            }
        }else{
            return 0;
        }        
    }
}

int32_t calc_improvement_diff(seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t num_acc_seqs, seq_profile_t potential_seq_to_accelerate){
    uint32_t area_cost = get_area_cost(potential_seq_to_accelerate);

    if(area_cost == 0){
        return 0;
    }

    uint32_t improvement_gain = calc_improvement(potential_seq_to_accelerate);

    if(improvement_gain == 0x80000000){
        return 0;
    }


}

void select_accelerators(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs, profiler_entry_t profiler_entries[NUM_PROFILER_ENTRIES]){
    seq_profile_t* temp = malloc(num_seq_profs*sizeof(seq_profile_t));
    
    memcpy(temp, &seq_profs[0], num_seq_profs*sizeof(seq_profile_t));

    sort_profiles_by_advantage(temp, num_seq_profs);

    seq_profile_t accelerated_seqs[NUM_RCAS];
    uint32_t next_acc_seq_index = 0;
    for(int i = 0; i < num_seq_profs; i++){
        if(temp[i].is_accelerated){
            accelerated_seqs[next_acc_seq_index] = temp[i];
            next_acc_seq_index++;
            if(next_acc_seq_index == NUM_RCAS){
                    break;
            }
        }
    }

}

int32_t find_seq_profile(uint32_t loop_start_addr, seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS]){
    for(int i = 0; i < MAX_STORED_SEQ_PROFS; i++){
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
    
    print_dfg(*(seq_prof.dfg));
    printf("CPU time per iteration: %u\n\r", seq_prof.cpu_time_per_iter);
    printf("Acc time per iteration: %u\n\r", seq_prof.acc_time_per_iter);

    printf("PR time: %u\n\r", seq_prof.pr_time);
}