import riscv_types::*;
import taiga_types::*;

module rca_wrapper(
    input clk, 
    input rst,
    rca_cpu_interface.slave cpu
);

    unit_issue_interface issue;
    rca_inputs_t rca_inputs;
    rca_dec_inputs_r_t rca_dec_inputs_r;
    rca_cpu_reg_config_t rca_config_regs_op;
    rca_writeback_interface rca_wb_if();
    logic rca_config_locked;

    rca_lsu_interface rca_lsu_if();

    assign issue.possible_issue = cpu.possible_issue;
    assign issue.new_request = cpu.new_request;
    assign issue.new_request_r = cpu.new_request_r;
    assign issue.id = cpu.issue_id;
    assign cpu.ready = issue.ready;

    assign rca_inputs.rca_use_fb_instr_decode = cpu.rca_use_fb_instr_decode;
    assign rca_inputs.rca_fb_cpu_reg_config_instr = cpu.rca_fb_cpu_reg_config_instr;
    assign rca_inputs.rca_nfb_cpu_reg_config_instr = cpu.rca_nfb_cpu_reg_config_instr;
    assign rca_inputs.rca_result_mux_config_fb = cpu.rca_result_mux_config_fb;

    assign rca_inputs.rs1 = cpu.rs1;
    assign rca_inputs.rs2 = cpu.rs2;
    assign rca_inputs.rs3 = cpu.rs3;
    assign rca_inputs.rs4 = cpu.rs4;
    assign rca_inputs.rs5 = cpu.rs5;

    assign rca_inputs.rca_sel_decode = cpu.rca_sel_decode;
    assign rca_inputs.cpu_port_sel = cpu.cpu_port_sel;
    assign rca_inputs.cpu_src_dest_port = cpu.cpu_src_dest_port;
    assign rca_inputs.cpu_reg_addr = cpu.cpu_reg_addr;

    assign rca_inputs.grid_mux_addr = cpu.grid_mux_addr;
    assign rca_inputs.new_grid_mux_sel = cpu.new_grid_mux_sel;

    assign rca_inputs.io_mux_addr = cpu.io_mux_addr;
    assign rca_inputs.new_io_mux_sel = cpu.new_io_mux_sel;

    assign rca_inputs.rca_result_mux_addr = cpu.rca_result_mux_addr;
    assign rca_inputs.new_rca_result_mux_sel = cpu.new_rca_result_mux_sel;

    assign rca_inputs.new_rca_io_inp_map = cpu.new_rca_io_inp_map;

    assign rca_inputs.io_unit_addr = cpu.io_unit_addr;
    assign rca_inputs.new_input_constant = cpu.new_input_constant;

    assign rca_inputs.io_ls_mask_config_fb = cpu.io_ls_mask_config_fb;
    assign rca_inputs.new_io_ls_mask = cpu.new_io_ls_mask;

    assign rca_dec_inputs_r.rca_use_instr = cpu.rca_use_instr;
    assign rca_dec_inputs_r.rca_use_fb_instr = cpu.rca_use_fb_instr;
    assign rca_dec_inputs_r.rca_sel = cpu.rca_sel;
    assign rca_dec_inputs_r.rca_grid_mux_config_instr = cpu.rca_grid_mux_config_instr;
    assign rca_dec_inputs_r.rca_io_mux_config_instr = cpu.rca_io_mux_config_instr;
    assign rca_dec_inputs_r.rca_result_mux_config_instr = cpu.rca_result_mux_config_instr;
    assign rca_dec_inputs_r.rca_io_inp_map_config_instr = cpu.rca_io_inp_map_config_instr;
    assign rca_dec_inputs_r.rca_input_constants_config_instr = cpu.rca_input_constants_config_instr;
    assign rca_dec_inputs_r.rca_io_ls_mask_config_instr = cpu.rca_io_ls_mask_config_instr;

    assign cpu.rca_cpu_src_reg_addrs = rca_config_regs_op.rca_cpu_src_reg_addrs;
    assign cpu.rca_cpu_dest_reg_addrs = rca_config_regs_op.rca_cpu_dest_reg_addrs;

    assign rca_wb_if.ack = cpu.ack;
    assign cpu.id = rca_wb_if.id;
    assign cpu.done = rca_wb_if.done;
    assign cpu.rd = rca_wb_if.rd;
    assign cpu.rca_config_locked = rca_config_locked;

    assign cpu.ls_request_rs1 = rca_lsu_if.rs1;
    assign cpu.ls_request_rs2 = rca_lsu_if.rs2;
    assign cpu.ls_request_fn3 = rca_lsu_if.fn3;
    assign cpu.ls_request_load = rca_lsu_if.load;
    assign cpu.ls_request_store = rca_lsu_if.store;
    assign cpu.ls_request_id = rca_lsu_if.id;

    assign rca_lsu_if.load_complete = cpu.load_complete;
    assign rca_lsu_if.load_data = cpu.load_data;
    assign cpu.load_complete = rca_lsu_if.rca_lsu_lock;
    assign cpu.ls_new_request = rca_lsu_if.new_request;
    assign rca_lsu_if.lsu_ready = cpu.lsu_ready;

    rca_unit rca(
        .clk,
        .rst,
        .issue,
        .rca_inputs,
        .rca_dec_inputs_r,
        .rca_config_regs_op,
        .rca_wb(rca_wb_if),
        .rca_config_locked,
        .lsu(rca_lsu_if)
    );

endmodule