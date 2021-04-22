module rca_pr_grid 
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,

    //Data
    input [XLEN-1:0] rs_vals [NUM_READ_PORTS],
    input [NUM_IO_UNITS-1:0] rs_data_valid,
    output [XLEN-1:0] io_unit_data_out [NUM_IO_UNITS],
    output io_unit_data_valid_out [NUM_IO_UNITS],

    //Config & Control - IO units
    input [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sels [NUM_IO_UNITS],
    input io_unit_output_mode [NUM_IO_UNITS],
    input io_units_rst,
    input io_fifo_pop [NUM_IO_UNITS],
    input [XLEN-1:0] input_constants [NUM_IO_UNITS],

    //Config & Control - PR slots
    input [$clog2(GRID_MUX_INPUTS)-1:0] grid_mux_sel [NUM_GRID_MUXES*2]
);

genvar i, j;

//Implementation - IO Units
typedef logic [XLEN-1:0] io_mux_data_t [IO_UNIT_MUX_INPUTS];
io_mux_data_t  io_mux_data_in [NUM_IO_UNITS];

typedef logic io_mux_data_valid_t [IO_UNIT_MUX_INPUTS];
io_mux_data_valid_t io_mux_data_valid_in [NUM_IO_UNITS];

logic [XLEN-1:0] io_mux_data_out [NUM_IO_UNITS];
logic io_mux_data_valid_out [NUM_IO_UNITS];

always_comb begin
    for (int k = 0; k < NUM_IO_UNITS; k++) begin

        for (int i = 0; i < NUM_READ_PORTS; i++)
            io_mux_data_in[k][i] = rs_vals[i];

        for (int j = NUM_READ_PORTS; j < NUM_READ_PORTS + GRID_NUM_COLS; j++)
            if (k < 1) io_mux_data_in[k][j] = 0; //first row doesn't have any preceding outputs
            else io_mux_data_in[k][j] = pr_unit_data_out[k-1][j - NUM_READ_PORTS];  

        io_mux_data_in[k][NUM_READ_PORTS + GRID_NUM_COLS] = input_constants[k];
    end
end

always_comb begin
    for (int k = 0; k < NUM_IO_UNITS; k++) begin

        for (int i = 0; i < NUM_READ_PORTS; i++)
            io_mux_data_valid_in[k][i] = rs_data_valid[k];

        for (int j = NUM_READ_PORTS; j < NUM_READ_PORTS + GRID_NUM_COLS; j++)
            if (k < 1) io_mux_data_valid_in[k][j] = 0; //first row doesn't have any preceding outputs
            else io_mux_data_valid_in[k][j] = pr_unit_data_valid_out[k-1][j - NUM_READ_PORTS];   
        
        io_mux_data_valid_in[k][NUM_READ_PORTS + GRID_NUM_COLS] = rs_data_valid[k];
    end
end

generate for (i = 0; i < NUM_IO_UNITS; i++) begin : io_unit_muxes
    grid_xbar_mux #(.NUM_INPUTS(IO_UNIT_MUX_INPUTS)) io_mux(
        .data_in(io_mux_data_in[i]),
        .data_valid_in(io_mux_data_valid_in[i]),
        .data_sel(curr_io_mux_sels[i]),
        .data_out(io_mux_data_out[i]),
        .data_valid_out(io_mux_data_valid_out[i])
    );
end endgenerate

generate for (i = 0; i < NUM_IO_UNITS; i++) begin : io_units
    grid_io_block io_unit(
        .clk, .rst,
        .data_valid_in(io_mux_data_valid_out[i]),
        .data_in(io_mux_data_out[i]),
        .data_valid_out(io_unit_data_valid_out[i]),
        .data_out(io_unit_data_out[i]),
        .output_mode(io_unit_output_mode[i]),
        .fifo_rst(io_units_rst),
        .fifo_pop(io_fifo_pop[i])
    );
end endgenerate

typedef logic [XLEN-1:0] pr_slot_mux_data_t [GRID_MUX_INPUTS];
typedef pr_slot_mux_data_t pr_row_mux_data_t [GRID_NUM_COLS];


typedef logic pr_slot_mux_data_valid_t [GRID_MUX_INPUTS];
typedef pr_slot_mux_data_valid_t pr_row_mux_data_valid_t [GRID_NUM_COLS];

pr_row_mux_data_t pr_slot_mux_data_in1 [GRID_NUM_ROWS];
pr_row_mux_data_valid_t pr_slot_mux_data_valid_in1 [GRID_NUM_ROWS];

typedef logic [XLEN-1:0] pr_mux_data_out_t [GRID_NUM_COLS];
typedef logic pr_mux_data_valid_out_t [GRID_NUM_COLS];

pr_mux_data_out_t pr_mux_data_out1 [GRID_NUM_ROWS];
pr_mux_data_valid_out_t pr_mux_data_valid_out1 [GRID_NUM_ROWS];

pr_row_mux_data_t pr_slot_mux_data_in2 [GRID_NUM_ROWS];// second data input to pr units
pr_row_mux_data_valid_t pr_slot_mux_data_valid_in2 [GRID_NUM_ROWS];

pr_mux_data_out_t pr_mux_data_out2 [GRID_NUM_ROWS];
pr_mux_data_valid_out_t pr_mux_data_valid_out2 [GRID_NUM_ROWS];

typedef logic [XLEN-1:0] pr_unit_data_out_t [GRID_NUM_COLS];
typedef logic pr_unit_data_valid_out_t [GRID_NUM_COLS];
pr_unit_data_out_t pr_unit_data_out [GRID_NUM_ROWS];
pr_unit_data_valid_out_t pr_unit_data_valid_out [GRID_NUM_ROWS];

always_comb begin
    for(int i = 0; i < GRID_NUM_ROWS; i++) begin
        for(int j = 0; j < GRID_NUM_COLS; j++) begin
            //crossbars between PR slots from row above into current row
            for(int k = 0; k < GRID_NUM_COLS; k++) begin
                if (i == 0) begin
                    pr_slot_mux_data_in1[i][j][k] = 0;
                    pr_slot_mux_data_valid_in1[i][j][k] = 0;
                end
                else begin
                    pr_slot_mux_data_in1[i][j][k] = pr_unit_data_out[i-1][k];
                    pr_slot_mux_data_valid_in1[i][j][k] = pr_unit_data_valid_out[i-1][k];
                end
            end
            //mux input for data from io unit above
            pr_slot_mux_data_in1[i][j][GRID_MUX_INPUTS-2] = io_unit_data_out[i];
            pr_slot_mux_data_valid_in1[i][j][GRID_MUX_INPUTS-2] = io_unit_data_valid_out[i];

            //mux input for data from pr slot on left
            if (j == 0) begin 
                pr_slot_mux_data_in1[i][j][GRID_MUX_INPUTS-1] = 0;
                pr_slot_mux_data_valid_in1[i][j][GRID_MUX_INPUTS-1] = 0;
            end
            else begin
                pr_slot_mux_data_in1[i][j][GRID_MUX_INPUTS-1] = pr_unit_data_out[i][j-1];
                pr_slot_mux_data_valid_in1[i][j][GRID_MUX_INPUTS-1] = pr_unit_data_valid_out[i][j-1];
            end

        end
    end
end

always_comb begin
    for(int i = 0; i < GRID_NUM_ROWS; i++) begin
        for(int j = 0; j < GRID_NUM_COLS; j++) begin
            //crossbars between PR slots from row above into current row
            for(int k = 0; k < GRID_NUM_COLS; k++) begin
                if (i == 0) begin
                    pr_slot_mux_data_in2[i][j][k] = 0;
                    pr_slot_mux_data_valid_in2[i][j][k] = 0;
                end
                else begin
                    pr_slot_mux_data_in2[i][j][k] = pr_unit_data_out[i-1][k];
                    pr_slot_mux_data_valid_in2[i][j][k] = pr_unit_data_valid_out[i-1][k];
                end
            end
            //mux input for data from io unit above
            pr_slot_mux_data_in2[i][j][GRID_MUX_INPUTS-2] = io_unit_data_out[i];
            pr_slot_mux_data_valid_in2[i][j][GRID_MUX_INPUTS-2] = io_unit_data_valid_out[i];

            //mux input for data from pr slot on left
            if (j == 0) begin 
                pr_slot_mux_data_in2[i][j][GRID_MUX_INPUTS-1] = 0;
                pr_slot_mux_data_valid_in2[i][j][GRID_MUX_INPUTS-1] = 0;
            end
            else begin
                pr_slot_mux_data_in2[i][j][GRID_MUX_INPUTS-1] = pr_unit_data_out[i][j-1];
                pr_slot_mux_data_valid_in2[i][j][GRID_MUX_INPUTS-1] = pr_unit_data_valid_out[i][j-1];
            end

        end

    end
end

generate 
    for (i = 0; i < GRID_NUM_ROWS; i++) begin : pr_slot_muxes1_row
        for (j = 0; j < GRID_NUM_COLS; j++) begin : pr_slot_muxes1_col
            grid_xbar_mux #(.NUM_INPUTS(GRID_MUX_INPUTS)) grid_mux(
                .data_in(pr_slot_mux_data_in1[i][j]),
                .data_valid_in(pr_slot_mux_data_valid_in1[i][j]),
                .data_sel(grid_mux_sel[(i*GRID_NUM_COLS) + j]),
                .data_out(pr_mux_data_out1[i][j]),
                .data_valid_out(pr_mux_data_valid_out1[i][j])
            );
        end
end endgenerate

generate 
    for (i = 0; i < GRID_NUM_ROWS; i++) begin : pr_slot_muxes2_row
        for (j = 0; j < GRID_NUM_COLS; j++) begin : pr_slot_muxes2_col
            grid_xbar_mux #(.NUM_INPUTS(GRID_MUX_INPUTS)) grid_mux(
                .data_in(pr_slot_mux_data_in2[i][j]),
                .data_valid_in(pr_slot_mux_data_valid_in2[i][j]),
                .data_sel(grid_mux_sel[(i*GRID_NUM_COLS) + j + NUM_GRID_MUXES]),
                .data_out(pr_mux_data_out2[i][j]),
                .data_valid_out(pr_mux_data_valid_out2[i][j])
            );
        end
end endgenerate

generate 
    for (i = 0; i < GRID_NUM_ROWS; i++) begin : pr_slots_row
        for (j = 0; j < GRID_NUM_COLS; j++) begin : pr_slots_col
            grid_pr_slot grid_slot(
                .clk,
                .rst,
                .data_in1(pr_mux_data_out1[i][j]),
                .data_valid_in1(pr_mux_data_valid_out1[i][j]),
                .data_in2(pr_mux_data_out2[i][j]),
                .data_valid_in2(pr_mux_data_valid_out2[i][j]),
                .data_out(pr_unit_data_out[i][j]),
                .data_valid_out(pr_unit_data_valid_out[i][j])
            );
        end
end endgenerate

//LSUs
    
endmodule