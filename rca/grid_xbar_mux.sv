module grid_xbar_mux
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
#(
    parameter NUM_INPUTS = GRID_MUX_INPUTS;
) (
    input [XLEN-1:0] data_in [NUM_INPUTS],
    input data_valid_in [NUM_INPUTS],
    input [$clog2(NUM_INPUTS)-1:0] data_sel,

    output [XLEN-1:0] data_out,
    output data_valid_out
);

    assign data_out = data_in[data_sel];
    assign data_valid_out = data_valid_in[data_sel];
    
endmodule