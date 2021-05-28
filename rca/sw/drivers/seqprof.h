#ifndef SEQPROF_H
#define SEQPROF_H

#include "rca.h"
#include "iid.h"
#include "dfg.h"
#include "subgrid.h"
#include "grid_manager.h"

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
void sort_profiles_by_improvement(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_valid_profs);
int32_t calc_improvement(seq_profile_t seq_prof);
uint32_t get_area_cost(seq_profile_t seq);
int32_t calc_improvement_diff(seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t num_acc_seqs, seq_profile_t potential_seq_to_accelerate, uint32_t* num_accs_to_replace);
void calc_improvement_diffs(seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t num_acc_seqs, seq_profile_t* potential_seqs_to_accelerate, uint32_t num_pot_seqs, int32_t* improvement_diffs, uint32_t* num_accs_to_replace);
uint32_t count_positive_entries(int32_t* arr, uint32_t num_els);
uint32_t find_max(int32_t* arr, uint32_t num_els);
void get_acc_and_rep_seqs(seq_profile_t* sorted_seq_profs, uint32_t num_seq_profs, seq_profile_t accelerated_seqs[NUM_RCAS], uint32_t* next_acc_seq_index, seq_profile_t replacement_seqs[NUM_PROFILER_ENTRIES], uint32_t* next_replacement_seq_index);
bool set_seq_accelerated(uint32_t loop_start_addr, seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs, bool accelerated);
void select_accelerators(seq_profile_t seq_profs[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs);
int32_t find_seq_profile(uint32_t loop_start_addr, seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS], uint32_t num_seq_profs);
void print_seq_profile(seq_profile_t seq_prof);


#endif //SEQPROF_H