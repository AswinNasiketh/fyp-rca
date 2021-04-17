module grid_pr_slot
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,

    //mostly dealing with up to 2 input instructions    
    input [XLEN-1:0] data_in1,
    input [XLEN-1:0] data_in2,

    input data_valid_in1,
    input data_valid_in2,

    output [XLEN-1:0] data_out,
    output data_valid_out
);

//It is anticipated that data at input 1 and input 2 will arrive at different rates => FIFOs are needed

logic input1_fifo_populated;
logic input2_fifo_populated;

//FIFO to queue data from input 1
    fifo_interface #(.DATA_WIDTH(XLEN)) input1_fifo_if ();

    taiga_fifo #(.DATA_WIDTH(XLEN), .FIFO_DEPTH(MAX_IDS)) input1_fifo (
        .clk, .rst,
        .fifo(input1_fifo_if)
    );

    assign input1_fifo_if.pop = ou_data_in_ack1;
    assign input1_fifo_if.data_in = data_in1;
    assign input1_fifo_if.potential_push = data_valid_in1;
    always_ff @(posedge clk) if (rst) input1_fifo_if.push <= 0;
        else input1_fifo_if.push <= input1_fifo_if.potential_push;

    assign input1_fifo_populated = input1_fifo_if.valid;

    //FIFO to queue data from input 2
    fifo_interface #(.DATA_WIDTH(XLEN)) input2_fifo_if ();

    taiga_fifo #(.DATA_WIDTH(XLEN), .FIFO_DEPTH(MAX_IDS)) input2_fifo (
        .clk, .rst,
        .fifo(input2_fifo_if)
    );

    assign input2_fifo_if.pop = ou_data_in_ack2;
    assign input2_fifo_if.data_in = data_in2;
    assign input2_fifo_if.potential_push = data_valid_in2;
    always_ff @(posedge clk) if (rst) input2_fifo_if.push <= 0;
        else input2_fifo_if.push <= input2_fifo_if.potential_push;

    assign input2_fifo_populated = input2_fifo_if.valid;

    logic ou_data_in_ack1;
    logic ou_data_in_ack2;

    pr_module ou(
        .clk,
        .rst,
        .data_in1(input1_fifo_if.data_out),
        .data_in2(input2_fifo_if.data_out),
        .data_valid_in1(input1_fifo_populated),
        .data_valid_in2(input2_fifo_populated),
        .data_out,
        .data_valid_out,
        .data_in_ack1(ou_data_in_ack1),
        .data_in_ack2(ou_data_in_ack2)
    );
    
endmodule