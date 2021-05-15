// For maintaining a model of the static region of PR grid

#include "static_region.h"

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

