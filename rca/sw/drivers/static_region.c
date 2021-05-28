// For maintaining a model of the static region of PR grid

#include "static_region.h"

//reg_addrs points to array with 5 register addresses
uint32_t configure_src_regs(static_region_t* pstatic_config, rca_t rca, uint32_t reg_addrs[NUM_READ_PORTS]){
    for(int i = 0; i < NUM_READ_PORTS; i++){
        if(reg_addrs[i] > 31) return 1; //returns 1 if register addr is outside range
        pstatic_config->cpu_src_regs[rca][i] = reg_addrs[i];
    }
    return 0;
}

uint32_t configure_nfb_dst_regs(static_region_t* pstatic_config, rca_t rca, uint32_t reg_addrs[NUM_WRITE_PORTS]){
    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        if(reg_addrs[i] > 31) return 1; //returns 1 if register addr is outside range
        pstatic_config->cpu_nfb_dst_regs[rca][i] = reg_addrs[i];
    }
    return 0;
}

uint32_t configure_fb_dst_regs(static_region_t* pstatic_config, rca_t rca, uint32_t reg_addrs[NUM_WRITE_PORTS]){
    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        if(reg_addrs[i] > 31) return 1; //returns 1 if register addr is outside range
        pstatic_config->cpu_fb_dst_regs[rca][i] = reg_addrs[i];
    }
    return 0;
}

uint32_t configure_grid_mux(static_region_t* pstatic_config, uint32_t row, uint32_t col, grid_slot_inp_t inp, grid_mux_inp_addr_t inp_addr){
    if(row >= NUM_GRID_ROWS || col >= NUM_GRID_COLS) return 1;
    pstatic_config->grid_mux_sel[inp][row][col] = inp_addr;
    return 0;
}

uint32_t configure_io_unit_mux(static_region_t* pstatic_config, uint32_t io_unit_addr, io_mux_inp_addr_t io_mux_inp_addr){
    if(io_unit_addr >= NUM_IO_MUX_INPUTS) return 1;
    pstatic_config->io_mux_sel[io_unit_addr] = io_mux_inp_addr;
    return 0;
}
//is_input array must have NUM_IO_UNIT elements
uint32_t configure_io_unit_inp_mask(static_region_t* pstatic_config, rca_t rca, bool is_input[NUM_IO_UNITS]){
    for(int i = 0; i < NUM_IO_UNITS; i++){
        pstatic_config->io_unit_is_input[rca][i] = is_input[i];
    }
    return 0;
}

uint32_t configure_fb_result_mux(static_region_t* pstatic_config, rca_t rca, rd_t write_port, uint32_t io_unit_addr){
    if(io_unit_addr > NUM_IO_UNITS) return 1;
    pstatic_config->result_mux_sel_fb[rca][write_port] = io_unit_addr;
    return 0;
}

uint32_t configure_nfb_result_mux(static_region_t* pstatic_config, rca_t rca, rd_t write_port, uint32_t io_unit_addr){
    if(io_unit_addr > NUM_IO_UNITS) return 1;
    pstatic_config->result_mux_sel_nfb[rca][write_port] = io_unit_addr;
    return 0;
}

uint32_t configure_input_constant(static_region_t* pstatic_config, uint32_t io_unit_addr, uint32_t new_constant){
    if(io_unit_addr >= NUM_IO_UNITS) return 1;
    pstatic_config->input_constants[io_unit_addr] = new_constant;
    return 0;
}

//wait_for_ls_request array must have NUM_IO_UNITS elements
uint32_t configure_fb_ls_mask(static_region_t* pstatic_config, rca_t rca, bool wait_for_ls_request[NUM_IO_UNITS]){
    for(int i = 0; i < NUM_IO_UNITS; i++){
        pstatic_config->ls_mask_fb[rca][i] = wait_for_ls_request[i];
    }    
    return 0;
}

uint32_t configure_nfb_ls_mask(static_region_t* pstatic_config, rca_t rca, bool wait_for_ls_request[NUM_IO_UNITS]){
    for(int i = 0; i < NUM_IO_UNITS; i++){
        pstatic_config->ls_mask_nfb[rca][i] = wait_for_ls_request[i];
    }    
    return 0;
}



void write_config(static_region_t* pstatic_config, uint32_t row_start, uint32_t row_end, rca_t rca){

    static_region_t static_region = *pstatic_config;

    //source regs
    for(int i = 0; i < NUM_READ_PORTS; i++){
        // printf("Configuring SRC regs for RCA %u, read port %u, reg_addr %u \n", rca, i, static_region.cpu_src_regs[rca][i]);
        rca_config_cpu_reg(rca, int_to_reg_port(i+1), SRC_PORT, static_region.cpu_src_regs[rca][i]);
    }

    //non feedback dest regs
    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        rca_config_cpu_reg(rca, int_to_reg_port(i+1), NFB_DEST_PORT, static_region.cpu_nfb_dst_regs[rca][i]);
    }

    //feedback dest regs
    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        rca_config_cpu_reg(rca, int_to_reg_port(i+1), FB_DEST_PORT, static_region.cpu_fb_dst_regs[rca][i]);
    }

    //grid mux config
    //NOTE: less than or equal to
    uint32_t mux_addr;
    for(int i = row_start; i <= row_end; i++){
        for(int j = 0; j < NUM_GRID_COLS; j++){
            //input 1
            mux_addr = (i * NUM_GRID_COLS) + j;
            rca_config_grid_mux(mux_addr, static_region.grid_mux_sel[0][i][j]);
            //input 2 
            mux_addr += NUM_GRID_ROWS*NUM_GRID_COLS;
            rca_config_grid_mux(mux_addr, static_region.grid_mux_sel[1][i][j]);
        }
    }

    //io mux config - just write to all IO units
    for(int i = 0; i < NUM_IO_UNITS; i++){
        rca_config_io_mux(i, static_region.io_mux_sel[i]);
    }

    //result mux config - non-feedback
    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        rca_config_result_mux(rca, int_to_reg_port(i+1), static_region.result_mux_sel_nfb[rca][i], false);
    }

    //result mux config - feedback
    for(int i = 0; i < NUM_WRITE_PORTS; i++){
        rca_config_result_mux(rca, int_to_reg_port(i+1), static_region.result_mux_sel_fb[rca][i], true);
    }

    //io unit input map
    rca_config_inp_io_unit_map(rca, static_region.io_unit_is_input[rca]);

    //input constants - write all IO units
    for(int i = 0; i < NUM_IO_UNITS; i++){
        rca_config_input_constant(i, static_region.input_constants[i]);
    }

    //io unit ls masks
    rca_config_io_ls_mask(rca, static_region.ls_mask_fb[rca], true);
    rca_config_io_ls_mask(rca, static_region.ls_mask_nfb[rca], false);
}
