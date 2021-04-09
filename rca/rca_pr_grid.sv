module rca_pr_grid 
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    .clk,
    .rst,

    //Data
    input [XLEN-1:0] rs_vals [NUM_READ_PORTS],
    input [GRID_NUM_ROWS-1:0] rs_data_valid,
    output [XLEN-1:0] io_unit_data_out [GRID_NUM_ROWS],
    output io_unit_data_valid_out [GRID_NUM_ROWS],

    //Config & Control - IO units
    input [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sel [GRID_NUM_ROWS],
    input io_unit_output_mode [GRID_NUM_ROWS],
    input io_units_rst,
    input io_fifo_pop [GRID_NUM_ROWS],

    //Config & Control - PR slots
    input [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sel [NUM_GRID_MUXES]
);

genvar i, j;

logic row_data_valid [GRID_NUM_COLS][GRID_NUM_ROWS];
logic [XLEN-1:0] row_data [GRID_NUM_COLS][GRID_NUM_ROWS];


//Implementation - IO Units
logic [XLEN-1:0] io_mux_data_in [IO_UNIT_MUX_INPUTS][GRID_NUM_ROWS];
logic io_mux_data_valid_in [IO_UNIT_MUX_INPUTS][GRID_NUM_ROWS];
logic [XLEN-1:0] io_mux_data_out [GRID_NUM_ROWS];
logic io_mux_data_valid_out [GRID_NUM_ROWS];

always_comb begin
    for (int k = 0; k < GRID_NUM_ROWS; k++) begin

        for (int i = 0; i < NUM_READ_PORTS; i++)
            io_mux_data_in[k][i] = rs_vals[i];

        for (int j = NUM_READ_PORTS; j < IO_UNIT_MUX_INPUTS; j++)
            if (k == 0) io_mux_data_in[k][j] = 0; //first row doesn't have any preceding outputs
            else io_mux_data_in[k][j] = row_data[k-1][j - NUM_READ_PORTS];            
    end
end

always_comb begin
    for (int k = 0; k < GRID_NUM_ROWS; k++) begin

        for (int i = 0; i < NUM_READ_PORTS; i++)
            io_mux_data_valid_in[k][i] = rs_data_valid[k];

        for (int j = NUM_READ_PORTS; j < IO_UNIT_MUX_INPUTS; j++)
            if (k == 0) io_mux_data_valid_in[k][j] = 0; //first row doesn't have any preceding outputs
            else io_mux_data_valid_in[k][j] = row_data_valid[k-1][j - NUM_READ_PORTS];   
    end
end

generate for (i = 0; i < GRID_NUM_ROWS; i++) begin : io_unit_muxes
    grid_xbar_mux #(.NUM_INPUTS(IO_UNIT_MUX_INPUTS)) io_mux(
        .data_in(io_mux_data_in[i]),
        .data_valid_in(io_mux_data_valid_in[i]),
        .data_sel(curr_io_mux_sel[i]),
        .data_out(io_mux_data_out[i]),
        .data_valid_out(io_mux_data_valid_out[i])
    );
end endgenerate

generate for (i = 0; i < GRID_NUM_ROWS; i++) begin : io_units
    grid_io_block io_unit(
        .clk, .rst,
        .data_valid_in(io_mux_data_valid_out[i]),
        .data_in(io_mux_data_out[i]),
        .data_valid_out(io_unit_data_valid_out[i]),
        .data_out(io_unit_data_out[i])
        .output_mode(io_unit_output_mode[i]),
        .fifo_rst(io_units_rst),
        .fifo_pop(io_fifo_pop[i])
    );
end endgenerate

logic [XLEN-1:0] pr_slot_mux_data_in [GRID_MUX_INPUTS][GRID_NUM_COLS][GRID_NUM_ROWS];
logic pr_slot_mux_data_valid_in [GRID_MUX_INPUTS][GRID_NUM_COLS][GRID_NUM_ROWS];

logic [XLEN-1:0] pr_mux_data_out [GRID_NUM_COLS][GRID_NUM_ROWS];
logic pr_mux_data_valid_out [GRID_NUM_COLS][GRID_NUM_ROWS];

logic [XLEN-1:0] pr_unit_data_out [GRID_NUM_COLS][GRID_NUM_ROWS];
logic pr_unit_data_valid_out [GRID_NUM_COLS][GRID_NUM_ROWS];

always_comb begin
    for(int i = 0; i < GRID_NUM_ROWS; i++) begin
        for(int j = 0; j < GRID_NUM_COLS; i++) begin
            //crossbars between PR slots from row above into current row
            for(int k = 0; k < GRID_NUM_COLS; k++) begin
                if (i == 0) begin
                    pr_slot_mux_data_in[i][j][k] = 0;
                    pr_slot_mux_data_valid_in[i][j][k] = 0;
                end
                else begin
                    pr_slot_mux_data_in[i][j][k] = pr_unit_data_out[i-1][k];
                    pr_slot_mux_data_valid_in[i][j][k] = pr_unit_data_valid_out[i-1][k];
                end
            end
            //mux input for data from io unit above
            if (i == 0) begin
                pr_slot_mux_data_in[i][j][GRID_MUX_INPUTS-2] = 0;
                pr_slot_mux_data_valid_in[i][j][GRID_MUX_INPUTS-2] = 0;
            end
            else begin
                pr_slot_mux_data_in[i][j][GRID_MUX_INPUTS-2] = io_unit_data_out[i-1];
                pr_slot_mux_data_valid_in[i][j][GRID_MUX_INPUTS-2] = io_unit_data_valid_out[i-1];
            end

            //mux input for data from pr slot on left
            if (j == 0) begin 
                pr_slot_mux_data_in[i][j][GRID_MUX_INPUTS-1] = 0;
                pr_slot_mux_data_valid_in[i][j][GRID_MUX_INPUTS-1] = 0;
            end
            else begin
                pr_slot_mux_data_in[i][j][GRID_MUX_INPUTS-1] = pr_unit_data_out[i][j-1];
                pr_slot_mux_data_valid_in[i][j][GRID_MUX_INPUTS-1] = pr_unit_data_valid_out[i][j-1];
            end

        end

    end
end

generate 
    for (i = 0; i < GRID_NUM_ROWS; i++) begin : pr_slot_muxes_row
        for (j = 0; j < GRID_NUM_COLS; j++) begin : pr_slot_muxes_col
            grid_xbar_mux #(.NUM_INPUTS(GRID_MUX_INPUTS)) grid_mux(
                .data_in(pr_slot_mux_data_in[i][j]),
                .data_valid_in(pr_slot_mux_data_valid_in[i][j]),
                .data_sel(grid_mux_sel[(i*GRID_NUM_COLS) + j]),
                .data_out(pr_mux_data_out[i][j]),
                .data_valid_out(pr_mux_data_valid_out[i][j])
            );
        end
end endgenerate

generate 
    for (i = 0; i < GRID_NUM_ROWS; i++) begin : pr_slots_row
        for (j = 0; j < GRID_NUM_COLS; j++) begin : pr_slots_col
            grid_pr_slot grid_slot(
                .clk,
                .rst,
                .data_in(pr_mux_data_out[i][j]),
                .data_valid_in(pr_mux_data_valid_out[i][j]),
                .data_out(pr_unit_data_out[i][j]),
                .data_valid_out(pr_unit_data_valid_out[i][j])
            );
        end
end endgenerate

//LSUs
//WB MUX + FIFOs


//N Write Ports *2 (feedback and normal) MUXes/Xbars per RCA?
//Need to modify destination registers and add more custom instrs?no
    
endmodule