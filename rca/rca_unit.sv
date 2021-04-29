import riscv_types::*;
import taiga_types::*;

module rca_unit(
    input clk,
    input rst,
    rca_decode_issue_interface.slave cpu,    
    rca_writeback_interface.slave rca_wb,
    rca_lsu_interface.slave lsu
);

    logic [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sel_out [NUM_GRID_MUXES*2];
    logic [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sels [NUM_IO_UNITS];
    logic [$clog2(NUM_IO_UNITS+1)-1:0] curr_fb_rca_result_mux_sel [NUM_WRITE_PORTS];
    logic [$clog2(NUM_IO_UNITS+1)-1:0] curr_nfb_rca_result_mux_sel [NUM_WRITE_PORTS];
    logic [NUM_IO_UNITS-1:0] curr_rca_io_inp_map;
    logic [XLEN-1:0] input_constants_out [NUM_IO_UNITS];
    logic [NUM_IO_UNITS-1:0] curr_io_ls_mask_fb;
    logic [NUM_IO_UNITS-1:0] curr_io_ls_mask_nfb;

    rca_config_regs rca_config_regfile(
        .*, //Read ports
        .clk(clk),
        .rst(rst),
        .rca_sel_issue(cpu.rca_dec_inputs_r.rca_sel), //for config writes
        .rca_sel_decode(cpu.rca_inputs.rca_sel_decode), //for CPU to read and write to correct registers when issuing use instrs
        .rca_sel_grid_wb(currently_running_rca), //for grid control to retrieve config information
        .rca_sel_buf(rca_sel_buf),

        .rca_cpu_src_reg_addrs_decode(cpu.rca_config_regs_op.rca_cpu_src_reg_addrs),
        .rca_cpu_dest_reg_addrs_decode(cpu.rca_config_regs_op.rca_cpu_dest_reg_addrs),
        .rca_use_fb_instr_decode(cpu.rca_inputs.rca_use_fb_instr_decode),

        .cpu_fb_reg_addr_wr_en(cpu.rca_inputs.rca_fb_cpu_reg_config_instr && cpu.new_request),
        .cpu_nfb_reg_addr_wr_en(cpu.rca_inputs.rca_nfb_cpu_reg_config_instr && cpu.new_request),
        .cpu_port_sel(cpu.rca_inputs.cpu_port_sel),
        .cpu_src_dest_port(cpu.rca_inputs.cpu_src_dest_port),
        .cpu_reg_addr(cpu.rca_inputs.cpu_reg_addr),

        .grid_mux_wr_addr(cpu.rca_inputs.grid_mux_addr),
        .grid_mux_wr_en(cpu.rca_dec_inputs_r.rca_grid_mux_config_instr && cpu.new_request),
        .new_grid_mux_sel(cpu.rca_inputs.new_grid_mux_sel),

        .io_mux_addr(cpu.rca_inputs.io_mux_addr),
        .io_mux_wr_en(cpu.rca_dec_inputs_r.rca_io_mux_config_instr && cpu.new_request),
        .new_io_mux_sel(cpu.rca_inputs.new_io_mux_sel),

        .rca_result_mux_addr(cpu.rca_inputs.rca_result_mux_addr),
        .rca_fb_result_mux_wr_en(cpu.rca_dec_inputs_r.rca_result_mux_config_instr && cpu.rca_inputs.rca_result_mux_config_fb && cpu.new_request),
        .rca_nfb_result_mux_wr_en(cpu.rca_dec_inputs_r.rca_result_mux_config_instr && ~cpu.rca_inputs.rca_result_mux_config_fb && cpu.new_request),
        .new_rca_result_mux_sel(cpu.rca_inputs.new_rca_result_mux_sel),

        .rca_io_inp_map_wr_en(cpu.rca_dec_inputs_r.rca_io_inp_map_config_instr && cpu.new_request),
        .new_rca_io_inp_map(cpu.rca_inputs.new_rca_io_inp_map),

        .rca_input_constants_wr_en(cpu.rca_dec_inputs_r.rca_input_constants_config_instr && cpu.new_request),
        .io_unit_addr(cpu.rca_inputs.io_unit_addr),
        .new_input_constant(cpu.rca_inputs.new_input_constant),

        .rca_io_ls_mask_wr_en(cpu.rca_dec_inputs_r.rca_io_ls_mask_config_instr && cpu.new_request),
        .rca_io_ls_mask_fb_wr_en(cpu.rca_dec_inputs_r.rca_io_ls_mask_config_instr && cpu.rca_inputs.io_ls_mask_config_fb && cpu.new_request),
        .new_io_ls_mask(cpu.rca_inputs.new_io_ls_mask)
    );

    id_t wb_id;
    logic wb_fb_instr;
    logic fifo_populated;
    logic buf_data_valid;
    logic buf_rs_data_valid;
    logic clear_fifos;
    logic [$clog2(NUM_RCAS)-1:0] currently_running_rca;
    logic [XLEN-1:0] buf_rs_data [NUM_READ_PORTS];
    logic [$clog2(NUM_RCAS)-1:0] rca_sel_buf;

    grid_control rca_grid_control(.*);

    logic [NUM_IO_UNITS-1:0] rs_data_valid;
    logic [XLEN-1:0] io_unit_data_out [NUM_IO_UNITS];
    logic io_unit_data_valid_out [NUM_IO_UNITS];
    logic [NUM_WRITE_PORTS-1:0] io_unit_addr_match_fb_wb [NUM_IO_UNITS];
    logic [NUM_WRITE_PORTS-1:0] io_unit_addr_match_nfb_wb [NUM_IO_UNITS];
    logic io_unit_output_mode [NUM_IO_UNITS];
    logic io_fifo_pop [NUM_IO_UNITS];
    logic io_unit_ls_requested [NUM_IO_UNITS];
    logic io_unit_ls_ack [NUM_IO_UNITS];
    logic [NUM_IO_UNITS-1:0] io_unit_ls_mask;

    assign rs_data_valid = curr_rca_io_inp_map & {NUM_IO_UNITS{buf_rs_data_valid}};

    always_comb begin
        for(int i = 0; i < NUM_IO_UNITS; i++) begin
            for(int j = 0; j < NUM_WRITE_PORTS; j++) begin
                io_unit_addr_match_fb_wb[i][j] = curr_fb_rca_result_mux_sel[j] == ($clog2(NUM_IO_UNITS+1))'(i);
                io_unit_addr_match_nfb_wb[i][j] = curr_nfb_rca_result_mux_sel[j] == ($clog2(NUM_IO_UNITS+1))'(i);
            end
        end
    end

    always_comb begin
        for(int i = 0; i < NUM_IO_UNITS; i++) begin
            io_unit_output_mode[i] = (|io_unit_addr_match_fb_wb[i]) || (|io_unit_addr_match_nfb_wb[i]);
        end
    end

    always_comb begin
        for (int i = 0; i < NUM_IO_UNITS; i++) begin
            io_fifo_pop[i] = wb_committing && (wb_fb_instr ? |io_unit_addr_match_fb_wb[i]: |io_unit_addr_match_nfb_wb[i]);
        end
    end

    always_comb begin
        for (int i = 0; i < NUM_IO_UNITS; i++) begin
            io_unit_ls_ack[i] = wb_committing && io_unit_ls_mask[i];
        end
    end

    always_comb io_unit_ls_mask = wb_fb_instr ? curr_io_ls_mask_fb : curr_io_ls_mask_nfb;

    rca_pr_grid pr_grid(
        .clk,
        .rst,
        .rs_vals(buf_rs_data),
        .rs_data_valid,
        .io_unit_data_out,
        .io_unit_data_valid_out,
        .curr_io_mux_sels,
        .io_unit_output_mode,
        .io_units_rst(clear_fifos),
        .io_fifo_pop,
        .grid_mux_sel(grid_mux_sel_out),
        .input_constants(input_constants_out),
        .io_unit_ls_requested,
        .io_unit_ls_ack,
        .lsq(rca_lsq_grid_if)
    );

    rca_lsq_grid_interface rca_lsq_grid_if();
    rca_lsq lsq(
        .clk, .rst,
        .lsu,
        .grid(rca_lsq_grid_if),
        .rca_fifo_populated(fifo_populated)
    );

    logic wb_committing;
    logic [$clog2(NUM_IO_UNITS+1)-1:0] io_unit_sels [NUM_WRITE_PORTS];
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
        .wb_committing,
        .io_unit_ls_requested,
        .io_unit_ls_mask
    );

    //rca_wb.rd 
    always_comb begin
        for(int i = 0; i < NUM_WRITE_PORTS; i++) begin
            if (wb_committing)
                rca_wb.rd[i] = grid_output_data[i];
            else
                rca_wb.rd[i] = 0; //only show grid outputs when using rca
        end
    end

    //rca_wb.id
    id_t id_prev;
    always_ff @(posedge clk) id_prev <= cpu.id;

    always_comb begin
        if (wb_committing)
            rca_wb.id = wb_id; //from grid control fifo
        else
            rca_wb.id = id_prev;
    end

    //rca_wb.done
    logic config_instr_issued;
    always_ff @(posedge clk) config_instr_issued <= cpu.new_request && ~cpu.rca_dec_inputs_r.rca_use_instr;

    assign rca_wb.done = wb_committing || config_instr_issued;

    assign rca.rca_config_locked = fifo_populated; // lock any reconfiguration if there are any RCAs being used
    
endmodule