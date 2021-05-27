#ifndef SEQPROF_H
#define SEQPROF_H

#include "rca.h"
#include "iid.h"
#include "dfg.h"
#include "subgrid.h"

#define MAX_STORED_SEQ_PROFS        50

typedef struct{
    uint32_t loop_start_addr;
    uint32_t num_instrs;
    uint32_t taken_count;

    bool seq_supported; 
    bool in_hw_profiler;

    dfg_t* dfg;

    uint32_t cpu_time_per_iter;
    uint32_t acc_time_per_iter; 

    uint32_t pr_time;

    uint32_t num_annealing_attempts;
    bool is_grid_implemented;

    sub_grid_t* sub_grid;

    bool is_accelerated;
}seq_profile_t;


seq_profile_t profile_seq(instr_seq_t* seq, profiler_entry_t* profiler_entry);
int32_t calc_advantage(seq_profile_t seq_prof);
void sort_profiles_by_advantage(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_valid_profs);
int32_t calc_improvement(seq_profile_t seq_prof);
uint32_t get_area_cost(seq_profile_t seq);
int32_t calc_improvement_diff(seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t num_acc_seqs, seq_profile_t potential_seq_to_accelerate);
void select_accelerators(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs, profiler_entry_t profiler_entries[NUM_PROFILER_ENTRIES]);
int32_t find_seq_profile(uint32_t loop_start_addr, seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS]);
void print_seq_profile(seq_profile_t seq_prof);


#endif //SEQPROF_H