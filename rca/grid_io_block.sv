module grid_io_block
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input data_valid_in,
    input [XLEN-1:0] data_in,

    input output_mode, //use RCA result MUX sel regfile for this
    input fifo_rst, // control signal from RCA control unit
    input fifo_pop, // control signal from internal RCA writeback unit

    output data_valid_out,
    output [XLEN-1:0] data_out
);

    //FIFO to store accelerator output data when in output mode
    fifo_interface #(.DATA_WIDTH(XLEN) oldest_data ();

    taiga_fifo #(.DATA_WIDTH(XLEN), .FIFO_DEPTH(MAX_IDS)) data_fifo (
        .clk, .rst(fifo_rst),
        .fifo(oldest_data)
    );

    assign oldest_data.pop = fifo_pop;
    assign oldest_data.data_in = data_in;
    assign oldest_data.potential_push = data_valid_in && output_mode;
    always_ff @(posedge clk) oldest_data.push = oldest_data.potential_push;

    modport structure(input push, pop, data_in, potential_push, output data_out, valid, full);

    //if not in output mode, IO unit just acts as passthrough, otherwise IO unit acts as FIFO
    assign data_valid_out = output_mode ? oldest_data.data_out : data_valid_in;
    assign data_out = output_mode ? oldest_data.valid : data_valid_out;
    
endmodule