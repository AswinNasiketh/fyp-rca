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
    logic [$clog2(GRID_NUM_ROWS)-1:0] curr_fb_rca_result_mux_sel [NUM_WRITE_PORTS];
    logic [$clog2(GRID_NUM_ROWS)-1:0] curr_nfb_rca_result_mux_sel [NUM_WRITE_PORTS];
    logic [GRID_NUM_ROWS-1:0] curr_rca_io_inp_map;

    rca_config_regs rca_config_regfile(
        .*,
        .clk(clk),
        .rst(rst),
        .rca_sel_issue(rca_inputs.rca_sel), //for config writes
        .rca_sel_decode(rca_inputs.rca_sel_decode), //for CPU to read and write to correct registers when issuing use instrs
        .rca_sel_grid_wb(currently_running_rca), //for grid control to retrieve config information
        .rca_sel_buf(rca_sel_buf),

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
        .rca_fb_result_mux_wr_en(rca_inputs.rca_result_mux_config_instr && rca_inputs.rca_result_mux_config_fb && issue.new_request),
        .rca_nfb_result_mux_wr_en(rca_inputs.rca_result_mux_config_instr && ~rca_inputs.rca_result_mux_config_fb && issue.new_request),
        .new_rca_result_mux_sel(rca_inputs.new_rca_result_mux_sel),

        .rca_io_inp_map_wr_en(rca_inputs.rca_io_use_config_instr && issue.new_request),
        .new_rca_io_inp_map(rca_inputs.new_rca_io_inp_use)
    );

    id_t wb_id;
    logic wb_fb_instr;
    logic fifo_populated;
    logic buf_data_valid;
    logic clear_fifos;
    logic [$clog2(NUM_RCAS)-1:0] currently_running_rca;
    logic [XLEN-1:0] buf_rs_data [NUM_READ_PORTS];
    logic [$clog2(NUM_RCAS)-1:0] rca_sel_buf;

    grid_control rca_grid_control(.*);

    logic [GRID_NUM_ROWS-1:0] rs_data_valid;
    logic [XLEN-1:0] io_unit_data_out [GRID_NUM_ROWS];
    logic io_unit_data_valid_out [GRID_NUM_ROWS];
    logic [NUM_WRITE_PORTS-1:0] io_unit_addr_match_fb_wb [GRID_NUM_ROWS];
    logic [NUM_WRITE_PORTS-1:0] io_unit_addr_match_nfb_wb [GRID_NUM_ROWS];
    logic io_unit_output_mode [GRID_NUM_ROWS];
    logic io_fifo_pop [GRID_NUM_ROWS];

    assign rs_data_valid = curr_rca_io_inp_map & {GRID_NUM_ROWS{buf_data_valid}};

    always_comb begin
        for(int i = 0; i < GRID_NUM_ROWS; i++) begin
            for(int j = 0; j < NUM_WRITE_PORTS; i++) begin
                io_unit_addr_match_fb_wb[i][j] = curr_fb_rca_result_mux_sel[j] == ($clog2(GRID_NUM_ROWS))'(i);
                io_unit_addr_match_nfb_wb[i][j] = curr_nfb_rca_result_mux_sel[j] == ($clog2(GRID_NUM_ROWS))'(i);
            end
        end
    end

    always_comb begin
        for(int i = 0; i < GRID_NUM_ROWS; i++) begin
            io_unit_output_mode[i] = (|io_unit_addr_match_fb_wb[i]) || (|io_unit_addr_match_nfb_wb[i]);
        end
    end

    always_comb begin
        for (int i = 0; i < GRID_NUM_ROWS; i++) begin
            io_fifo_pop[i] = wb_committing && (wb_fb_instr ? io_unit_addr_match_fb_wb[i]: io_unit_addr_match_nfb_wb[i]);
        end
    end

    rca_pr_grid pr_grid(
        .clk,
        .rst,
        .rs_vals(buf_rs_data),
        .rs_data_valid,
        .io_unit_data_out,
        .io_unit_data_valid_out,
        .curr_io_mux_sel,
        .io_unit_output_mode,
        .io_units_rst(clear_fifos),
        .io_fifo_pop,
        .grid_mux_sel(grid_mux_sel_out)
    );

    logic wb_committing;
    logic [$clog2(GRID_NUM_ROWS)-1:0] io_unit_sels [NUM_WRITE_PORTS];
    logic [XLEN-1:0] grid_output_data [NUM_WRITE_PORTS];

    always_comb begin
        for(int i = 0; i < NUM_WRITE_PORTS; i++)
            io_unit_sels[i] = wb_fb_instr ? curr_fb_rca_result_mux_sel[i] : curr_nfb_rca_result_mux_sel[i]; 
    end

    grid_wb rca_grid_wb(
        .io_unit_output_data(io_unit_data_out),
        .io_unit_output_data_valid(io_unit_data_valid_out),
        .io_unit_sels,
        .io_unit_sels_valid(fifo_populated),
        .output_data(grid_output_data),
        .wb_committing
    );

    //TODO: assign grid_output_data to rca_wb.rd
    

    //stub module 
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

            rca_wb.done <= 0;
            rca_wb.id <= 0;
            for(int i = 0; i < NUM_WRITE_PORTS; i++)
                rca_wb.rd[i] <= 0;
        end
    end
    
endmodule