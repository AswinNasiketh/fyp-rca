import taiga_config::*;
import riscv_types::*;
import taiga_types::*;
import l2_config_and_types::*;
import rca_config::*;


interface rca_cpu_interface;
    //duplication of issue interface
        logic possible_issue;
        logic new_request;
        logic new_request_r;
        logic [$clog2(MAX_IDS)-1:0] issue_id;

        logic ready;

    //duplication of rca_inputs_t in taiga_types.sv
        logic rca_use_fb_instr_decode;         
        logic rca_fb_cpu_reg_config_instr;         
        logic rca_nfb_cpu_reg_config_instr;
        logic rca_result_mux_config_fb;        

        logic [XLEN-1:0] rs1;
        logic [XLEN-1:0] rs2;
        logic [XLEN-1:0] rs3;
        logic [XLEN-1:0] rs4;
        logic [XLEN-1:0] rs5;
        
        logic [$clog2(NUM_RCAS)-1:0] rca_sel_decode;
       
        logic [$clog2(NUM_READ_PORTS)-1:0] cpu_port_sel;
        logic cpu_src_dest_port;
        logic [4:0] cpu_reg_addr;
       
        logic [$clog2(NUM_GRID_MUXES*2)-1:0] grid_mux_addr;
        logic [$clog2(GRID_MUX_INPUTS)-1:0] new_grid_mux_sel;
       
        logic [$clog2(NUM_IO_UNITS)-1:0] io_mux_addr;
        logic [$clog2(IO_UNIT_MUX_INPUTS)-1:0] new_io_mux_sel;
        
        logic [$clog2(NUM_WRITE_PORTS)-1:0] rca_result_mux_addr;
        logic [$clog2(NUM_IO_UNITS+1)-1:0] new_rca_result_mux_sel;
        
        logic [NUM_IO_UNITS-1:0] new_rca_io_inp_map;

        logic [$clog2(NUM_IO_UNITS)-1:0] io_unit_addr;
        logic [XLEN-1:0] new_input_constant;

        logic io_ls_mask_config_fb;
        logic [NUM_IO_UNITS-1:0] new_io_ls_mask;

        //Duplication of rca_dec_inputs_r_t in taiga_types.sv

        logic rca_use_instr;
        logic rca_use_fb_instr; 
        logic [$clog2(NUM_RCAS)-1:0] rca_sel;
        logic rca_grid_mux_config_instr; 
        logic rca_io_mux_config_instr;
        logic rca_result_mux_config_instr;
        logic rca_io_inp_map_config_instr;
        logic rca_input_constants_config_instr;
        logic rca_io_ls_mask_config_instr;

        //Duplication of rca_cpu_reg_config_t in taiga_types.sv

        logic [4:0] [NUM_READ_PORTS-1:0] rca_cpu_src_reg_addrs;
        logic [4:0] [NUM_WRITE_PORTS-1:0] rca_cpu_dest_reg_addrs;

        //Duplication of rca_writeback_interface
        logic ack;

        logic[$clog2(MAX_IDS)-1:0] wb_id;
        logic done;
        logic [XLEN-1:0] rd1;
        logic [XLEN-1:0] rd2;
        logic [XLEN-1:0] rd3;
        logic [XLEN-1:0] rd4;
        logic [XLEN-1:0] rd5;

        logic rca_config_locked;

        //Duplication of rca_lsu_interface
        logic [XLEN-1:0] ls_request_rs1;
        logic [XLEN-1:0] ls_request_rs2;
        logic [2:0] ls_request_fn3;
        logic ls_request_load;
        logic ls_request_store;
        logic[$clog2(MAX_IDS)-1:0] ls_request_id;


        logic load_complete;
        logic [XLEN-1:0] load_data;

        //control signals
        logic rca_lsu_lock;
        logic ls_new_request;
        logic lsu_ready;

        modport master(input rca_cpu_src_reg_addrs, rca_cpu_dest_reg_addrs, wb_id, done, rd1, rd2, rd3, rd4, rd5, rca_config_locked, ls_request_rs1, ls_request_rs2, ls_request_fn3, ls_request_load, ls_request_store, rca_lsu_lock, ls_new_request, ls_request_id, ready,
        output rca_use_fb_instr_decode, rca_fb_cpu_reg_config_instr, rca_nfb_cpu_reg_config_instr, rca_result_mux_config_fb, rs1, rs2, rs3, rs4, rs5, rca_sel_decode, cpu_port_sel, cpu_src_dest_port, cpu_reg_addr, grid_mux_addr, new_grid_mux_sel, io_mux_addr, new_io_mux_sel, rca_result_mux_addr, new_rca_result_mux_sel, new_rca_io_inp_map, io_unit_addr, new_input_constant, io_ls_mask_config_fb, new_io_ls_mask, rca_use_instr, rca_use_fb_instr, rca_sel, rca_grid_mux_config_instr, rca_io_mux_config_instr, rca_result_mux_config_instr, rca_io_inp_map_config_instr, rca_input_constants_config_instr, rca_io_ls_mask_config_instr, ack, lsu_ready, load_complete, load_data, possible_issue, new_request, new_request_r, issue_id);

        modport slave(output rca_cpu_src_reg_addrs, rca_cpu_dest_reg_addrs, wb_id, done, rd1, rd2, rd3, rd4, rd5, rca_config_locked, ls_request_rs1, ls_request_rs2, ls_request_fn3, ls_request_load, ls_request_store, rca_lsu_lock, ls_new_request, ls_request_id, ready,
        input rca_use_fb_instr_decode, rca_fb_cpu_reg_config_instr, rca_nfb_cpu_reg_config_instr, rca_result_mux_config_fb, rs1, rs2, rs3, rs4, rs5, rca_sel_decode, cpu_port_sel, cpu_src_dest_port, cpu_reg_addr, grid_mux_addr, new_grid_mux_sel, io_mux_addr, new_io_mux_sel, rca_result_mux_addr, new_rca_result_mux_sel, new_rca_io_inp_map, io_unit_addr, new_input_constant, io_ls_mask_config_fb, new_io_ls_mask, rca_use_instr, rca_use_fb_instr, rca_sel, rca_grid_mux_config_instr, rca_io_mux_config_instr, rca_result_mux_config_instr, rca_io_inp_map_config_instr, rca_input_constants_config_instr, rca_io_ls_mask_config_instr, ack, lsu_ready, load_complete, load_data, possible_issue, new_request, new_request_r, issue_id);

endinterface