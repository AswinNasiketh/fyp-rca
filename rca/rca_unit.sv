import riscv_types::*;
import taiga_types::*;

module rca_unit(
    unit_issue_interface.unit issue,
    // unit_writeback_interface.unit wb,
    input clk,
    input rst,
    input rca_inputs_t rca_inputs,
    output rca_config_t rca_config_regs_op,
    rca_writeback_interface.unit rca_wb
);

    logic [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sel_out [NUM_GRID_MUXES];
    logic [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sel [GRID_NUM_ROWS];
    logic [$clog2(GRID_NUM_ROWS)-1:0] curr_rca_result_mux_sel [NUM_WRITE_PORTS];
    logic [GRID_NUM_ROWS-1:0] curr_rca_io_inp_use;

    rca_config_regs rca_config_regfile(
        .*,
        .clk(clk),
        .rst(rst),
        .rca_sel_issue(rca_inputs.rca_sel),
        .rca_sel_decode(rca_inputs.rca_sel_decode),
        //.rca_sel_grid_control() TODO

        .rca_cpu_src_reg_addrs_decode(rca_config_regs_op.rca_cpu_src_reg_addrs),
        .rca_cpu_dest_reg_addrs_decode(rca_config_regs_op.rca_cpu_dest_reg_addrs),
        .rca_use_fb_instr_decode(rca_inputs.rca_use_fb_instr_decode),

        .cpu_fb_reg_addr_wr_en(rca_inputs.rca_fb_cpu_reg_config_instr && issue.new_request),
        .cpu_nfb_reg_addr_wr_en(rca_inputs.rca_nfb_cpu_reg_config_instr && issue.new_request),
        .cpu_port_sel(rca_inputs.cpu_port_sel),
        .cpu_src_dest_port(rca_inputs.cpu_src_dest_port),
        .cpu_reg_addr(rca_inputs.cpu_reg_addr),

        .grid_mux_addr(rca_inputs.grid_mux_addr),
        .grid_mux_wr_en(rca_inputs.rca_grid_mux_config_instr && issue.new_request),
        .new_grid_mux_sel(rca_inputs.new_grid_mux_sel),

        .io_mux_addr(rca_inputs.io_mux_addr),
        .io_mux_wr_en(rca_inputs.rca_io_mux_config_instr && issue.new_request),
        .new_io_mux_sel(rca_inputs.new_io_mux_sel),

        .rca_result_mux_addr(rca_inputs.rca_result_mux_addr),
        .rca_result_mux_wr_en(rca_inputs.rca_result_mux_config_instr && issue.new_request),
        .new_rca_result_mux_sel(rca_inputs.new_rca_result_mux_sel),

        .rca_io_inp_use_wr_en(rca_inputs.rca_io_use_config_instr && issue.new_request),
        .new_rca_io_inp_use(rca_inputs.new_rca_io_inp_use)
    );

    //stub module for later implementation of RCAs
    assign issue.ready = 1'b1;
    
    always_ff @(posedge clk) begin
        if (issue.new_request && ~rca_inputs.rca_use_instr) begin

            rca_wb.id <= issue.id;
            rca_wb.done <= 1;
            for(int i = 0; i < NUM_WRITE_PORTS; i++)
                rca_wb.rd[i] <= 0;
        end
        else if (rca_inputs.rca_use_instr && issue.new_request) begin
            rca_wb.done <= 1;
            rca_wb.id <= issue.id;
            //Reverse input register order - just for testing
            rca_wb.rd[0] <= rca_inputs.rs5;
            rca_wb.rd[1] <= rca_inputs.rs4;
            rca_wb.rd[2] <= rca_inputs.rs3;
            rca_wb.rd[3] <= rca_inputs.rs2;
            rca_wb.rd[4] <= rca_inputs.rs1;
        end
        else begin 
            // wb.done <= 0;
            // wb.rd <= 0;
            // wb.id <= 0;

            rca_wb.done <= 0;
            rca_wb.id <= 0;
            for(int i = 0; i < NUM_WRITE_PORTS; i++)
                rca_wb.rd[i] <= 0;
        end
    end
    
endmodule