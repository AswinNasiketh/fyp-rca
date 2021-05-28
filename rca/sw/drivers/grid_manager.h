#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include "rca.h"
#include "static_region.h"
#include "seqprof.h"

void init_grid();
uint32_t get_next_free_row();
void apply_partial_region_cfg(seq_profile_t* seq_profile, uint32_t row_offset);
void clear_ous_in_rows(uint32_t row_start, uint32_t row_end);
bool is_fb_output(uint32_t node_id, seq_profile_t* seq_profile);
void apply_static_region_cfg(seq_profile_t* seq_profile, uint32_t row_offset, rca_t rca);
void disable_acc_att(rca_t rca);
void enable_acc_att(seq_profile_t* seq_profile, rca_t rca);
bool add_accelerator(seq_profile_t* seq_profile);
bool disable_accelerator(rca_t rca);
int32_t find_accelerator(seq_profile_t* seq_profile);
bool wipe_accelerator(seq_profile_t* seq_profile);
bool replace_accelerators(seq_profile_t** accs_to_replace, uint32_t num_accs_to_replace, seq_profile_t* replacing_acc);

#endif //GRID_MANAGER_H