#include "grid_manager.h"

static static_region_t s_region;
static seq_profile_t* accelerated_sequences[NUM_RCAS];
uint32_t num_accelerated_seqs;

void init_grid(){
    for(int i = 0; i < NUM_RCAS; i++){
        for(int j = 0; j < NUM_WRITE_PORTS; j++){
            s_region.result_mux_sel_fb[i][j] = UNUSED_WRITE_PORT_ADDR;
            s_region.result_mux_sel_nfb[i][j] = UNUSED_WRITE_PORT_ADDR;
        }
    }

    for(int j = 0; j < NUM_RCAS; j++){
        for(int i = 0; i < NUM_IO_UNITS; i++){
            s_region.io_unit_is_input[j][i] = false;
            s_region.ls_mask_fb[j][i] = false;
            s_region.ls_mask_nfb[j][i] = false;
        }
    }   

    num_accelerated_seqs = 0;
    for(int i = 0; i < NUM_RCAS; i++){
        accelerated_sequences[i] = NULL;
    } 
}

uint32_t get_next_free_row(){
    uint32_t next_free_row = 0;
    for(int i = 0; i < num_accelerated_seqs; i++){
        next_free_row += accelerated_sequences[i]->sub_grid->num_rows;
    }
    return next_free_row;
}

void apply_partial_region_cfg(seq_profile_t* seq_profile, uint32_t row_offset){
    uint32_t k;
    for(int i = row_offset; i < row_offset + seq_profile->sub_grid->num_rows; i++){
        k = i - row_offset;
        for(int j = 0; j < NUM_GRID_COLS; j++){
            send_pr_request(seq_profile->sub_grid->grid_slots[k][j].ou, grid_coord_to_slot(i,j));
        }
    }
}

void clear_ous_in_rows(uint32_t row_start, uint32_t row_end){
    for(int i = row_start; i < row_end; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            send_pr_request(UNUSED, grid_coord_to_slot(i, j));
        }
    }
}

bool is_fb_output(uint32_t node_id, seq_profile_t* seq_profile){
    io_unit_cfg_t curr_io_unit;
    for(int i = 0; i < seq_profile->sub_grid->num_rows; i++){
        curr_io_unit = seq_profile->sub_grid->io_unit_cfgs[i];
        if(curr_io_unit.is_inp && curr_io_unit.node_id == node_id){
            return true;
        }
    }

    return false;
}


void apply_static_region_cfg(seq_profile_t* seq_profile, uint32_t row_offset, rca_t rca){
    uint32_t k;
    io_unit_cfg_t curr_io_unit_cfg;

    //initialise config data

    uint32_t next_free_fb_dst_reg_slot = 0;
    uint32_t next_free_nfb_dst_reg_slot = 0;
    uint32_t next_free_src_reg_slot = 0;

    uint32_t fb_dst_reg_addrs[NUM_WRITE_PORTS];
    uint32_t nfb_dst_reg_addrs[NUM_WRITE_PORTS];
    uint32_t src_reg_addrs[NUM_READ_PORTS];

    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        fb_dst_reg_addrs[i] = 0;
        nfb_dst_reg_addrs[i] = 0;
    }

    for(int i = 0; i < NUM_READ_PORTS; i++){
        src_reg_addrs[i] = 0;
    }

    bool ls_mask_fb[NUM_IO_UNITS];
    bool ls_mask_nfb[NUM_IO_UNITS]; //unused for now

    for(int i = 0; i < NUM_IO_UNITS; i++){
        ls_mask_fb[i] = false;
        ls_mask_nfb[i] = false;
    }

    for(int i = row_offset; i < row_offset + seq_profile->sub_grid->num_rows; i++){
        k = i - row_offset;

        //configure grid muxes
        for(int slot_in = 0; slot_in < 2; slot_in++){
            for(int j = 0; j < NUM_GRID_COLS; j++){
                configure_grid_mux(&s_region, i, j, slot_in, seq_profile->sub_grid->row_xbar_cfgs[k].arr[slot_in][j]);
            }
        }
        //configure io units
        curr_io_unit_cfg = seq_profile->sub_grid->io_unit_cfgs[k];
        if(curr_io_unit_cfg.is_output){
            if(is_fb_output(curr_io_unit_cfg.node_id, seq_profile)){
                fb_dst_reg_addrs[next_free_fb_dst_reg_slot] = curr_io_unit_cfg.value;
                configure_fb_result_mux(&s_region, rca, next_free_fb_dst_reg_slot, i);
                next_free_fb_dst_reg_slot++;
            }else{
                nfb_dst_reg_addrs[next_free_fb_dst_reg_slot] = curr_io_unit_cfg.value;
                configure_nfb_result_mux(&s_region, rca, next_free_nfb_dst_reg_slot, i);
                next_free_nfb_dst_reg_slot++;
            }

            configure_io_unit_mux(&s_region, i, curr_io_unit_cfg.io_mux_inp);
        }else if(curr_io_unit_cfg.is_inp){
            if(curr_io_unit_cfg.is_reg){
                src_reg_addrs[next_free_src_reg_slot] = curr_io_unit_cfg.value;
                configure_io_unit_mux(&s_region, i, next_free_src_reg_slot);
                next_free_src_reg_slot++;
            }else{
                configure_input_constant(&s_region, i, curr_io_unit_cfg.value);
                configure_io_unit_mux(&s_region, i, CONST);
            }
        }

        if(curr_io_unit_cfg.wait_for_ls_submit){
            ls_mask_fb[k] = true;
        }
    }

    for(int i = next_free_fb_dst_reg_slot; i < seq_profile->sub_grid->num_rows; i++){
        configure_fb_result_mux(&s_region, rca, i, UNUSED_WRITE_PORT_ADDR);
    }

    for(int i = next_free_nfb_dst_reg_slot; i < seq_profile->sub_grid->num_rows; i++){
        configure_nfb_result_mux(&s_region, rca, i, UNUSED_WRITE_PORT_ADDR);
    }

    configure_src_regs(&s_region, rca, src_reg_addrs);
    configure_fb_dst_regs(&s_region, rca, fb_dst_reg_addrs);
    configure_nfb_dst_regs(&s_region, rca, nfb_dst_reg_addrs);
    configure_fb_ls_mask(&s_region, rca, ls_mask_fb);
    configure_nfb_ls_mask(&s_region, rca, ls_mask_nfb);

    write_config(&s_region, row_offset, row_offset + seq_profile->sub_grid->num_rows, rca);    
}

void disable_acc_att(rca_t rca){
    set_att_field(rca, ACC_ENABLE, 0);
}

void enable_acc_att(seq_profile_t* seq_profile, rca_t rca){
    set_att_field(rca, ACC_ENABLE, 0);

    set_att_field(rca, LOOP_START_ADDR, seq_profile->loop_start_addr);
    set_att_field(rca, SBB_ADDR, seq_profile->loop_start_addr + (seq_profile->num_instrs * 4));

    set_att_field(rca, ACC_ENABLE, 1);
}



bool add_accelerator(seq_profile_t* seq_profile){
    uint32_t next_free_row = get_next_free_row();

    if(next_free_row + seq_profile->sub_grid->num_rows > NUM_GRID_ROWS){
        return false;
    }

    if(num_accelerated_seqs == NUM_RCAS){
        return false;
    }

    apply_partial_region_cfg(seq_profile, next_free_row);
    apply_static_region_cfg(seq_profile, next_free_row, num_accelerated_seqs);
    enable_acc_att(seq_profile, num_accelerated_seqs);

    accelerated_sequences[num_accelerated_seqs] = seq_profile;

    num_accelerated_seqs++;
    return true;
}

bool disable_accelerator(rca_t rca){
    if(rca >= num_accelerated_seqs){
        return false;
    }

    disable_acc_att(rca);

    return true;
}

int32_t find_accelerator(seq_profile_t* seq_profile){
    for(int i = 0; i < num_accelerated_seqs; i++){
        if(seq_profile == accelerated_sequences[i]){
            return i;
        }
    }

    return -1;
}

bool wipe_accelerator(seq_profile_t* seq_profile){
    //find accelerator to wipe based on seq profile to remove
    uint32_t accelerator_to_wipe = find_accelerator(seq_profile);

    //get number of rows currently occupied
    uint32_t num_rows_occupied = get_next_free_row();

    if(accelerator_to_wipe == -1){
        printf("Couldn't find accelerator to wipe\n\r");
        return false;
    }

    //find the row which the accelerator to wipe starts at
    uint32_t next_free_row = 0;
    for(int i = 0; i < accelerator_to_wipe; i++){
        next_free_row += accelerated_sequences[i]->sub_grid->num_rows;
    }

    //move all accelerators after the accelerator to wipe up, filling the rows previously occupied by the accelerator to wipe
    for(int i = accelerator_to_wipe+1; i < num_accelerated_seqs; i++){
        apply_partial_region_cfg(accelerated_sequences[i], next_free_row);
        apply_static_region_cfg(accelerated_sequences[i], next_free_row, i-1);
        enable_acc_att(accelerated_sequences[i], i-1);
        disable_acc_att(i);
        accelerated_sequences[i-1] = accelerated_sequences[i];
        next_free_row += accelerated_sequences[i]->sub_grid->num_rows;
        accelerated_sequences[i] = NULL;
    }

    //clear all OUs in rows that were occupied but no longer are
    clear_ous_in_rows(next_free_row, num_rows_occupied);

    num_accelerated_seqs--;
    return true;
}

bool replace_accelerators(seq_profile_t* accs_to_replace, uint32_t num_accs_to_replace, seq_profile_t* replacing_acc){
    bool success = true;
    for(int i = 0; i < num_accs_to_replace; i++){
        success = wipe_accelerator(&accs_to_replace[i]);
        if(!success) return success;
    }

    success = add_accelerator(replacing_acc);
    return success;
}



