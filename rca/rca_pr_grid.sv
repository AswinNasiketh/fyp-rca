module rca_pr_grid 
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
#(
    parameters
) (
    //Data
    input [XLEN-1:0] rs_vals [NUM_READ_PORTS],
    output [XLEN-1:0] rd_vals [NUM_WRITE_PORTS],
    input [GRID_NUM_ROWS] data_valid,

    //Config - IO units
    input [$clog2(IO_UNIT_MUX_INPUTS)-1:0] curr_io_mux_sel [GRID_NUM_ROWS]
);

genvar i, j;

logic row_data_valid [GRID_NUM_COLS][GRID_NUM_ROWS];
logic [XLEN-1:0] row_data [GRID_NUM_COLS][GRID_NUM_ROWS];


//Implementation - IO Units
logic [XLEN-1:0] io_mux_input_data [IO_UNIT_MUX_INPUTS][GRID_NUM_ROWS];
logic io_mux_data_valid [IO_UNIT_MUX_INPUTS][GRID_NUM_ROWS];
logic [XLEN-1:0] io_mux_data_out [GRID_NUM_ROWS];
logic io_mux_data_valid_out [GRID_NUM_ROWS];

logic [XLEN-1:0] io_unit_data_out [GRID_NUM_ROWS];
logic io_unit_data_valid_out [GRID_NUM_ROWS];

always_comb begin
    for (int k = 0; k < GRID_NUM_ROWS; k++) begin

        for (int i = 0; i < NUM_READ_PORTS; i++)
            io_mux_input_data[i] = rs_vals[i];

        for (int j = NUM_READ_PORTS; j < IO_UNIT_MUX_INPUTS; j++)
            if (k == 0) io_mux_input_data[j] = 0; //first row doesn't have any preceding outputs
            else io_mux_input_data[j] = row_data[k-1][j - NUM_READ_PORTS];            
    end
end

always_comb begin
    for (int k = 0; k < GRID_NUM_ROWS; k++) begin

        for (int i = 0; i < NUM_READ_PORTS; i++)
            io_mux_data_valid[i] = data_valid[k];

        for (int j = NUM_READ_PORTS; j < IO_UNIT_MUX_INPUTS; j++)
            if (k == 0) io_mux_data_valid[j] = 0; //first row doesn't have any preceding outputs
            else io_mux_data_valid[j] = row_data_valid[k-1][j - NUM_READ_PORTS];   
    end
end

generate for (i = 0; i < GRID_NUM_ROWS; i++) begin : io_unit_muxes
    grid_xbar_mux #(.NUM_INPUTS(IO_UNIT_MUX_INPUTS)) io_mux(
        .data_in(io_mux_input_data[i]),
        .data_valid_in(io_mux_data_valid[i]),
        .data_sel(curr_io_mux_sel[i]),
        .data_out(io_mux_data_out[i]),
        .data_valid_out(io_mux_data_valid_out[i])
    )
end endgenerate

generate for (i = 0; i < GRID_NUM_ROWS; i++) begin : io_units
    grid_io_block io_unit(
        .data_valid_in(io_mux_data_valid_out[i]),
        .data_in(io_mux_data_out[i]),
        .data_valid_out(io_unit_data_valid_out[i]),
        .data_out(io_unit_data_out[i])
    )
end endgenerate

//PR Slots
//LSUs
//WB MUX + FIFOs


//N Write Ports *2 (feedback and normal) MUXes/Xbars per RCA?
//Need to modify destination registers and add more custom instrs?no
    
endmodule