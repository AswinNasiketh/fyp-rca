module sw_ou
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;    
(
    input clk,
    input rst,

    input [XLEN-1:0] data_in1,
    input [XLEN-1:0] data_in2,

    input data_valid_in1,
    input data_valid_in2,

    output [XLEN-1:0] data_out,
    output data_valid_out,

    output data_in_ack1,
    output data_in_ack2,

    output uses_data_in1,
    output uses_data_in2,   

    //LSQ interface
    output [XLEN-1:0] addr, 
    output [XLEN-1:0] data,
    output [2:0] fn3,
    output load,
    output store,
    output new_request,
    input lsq_full,

    input [XLEN-1:0] load_data,
    input load_complete
);
    assign uses_data_in1 = 1'b1;
    assign uses_data_in2 = 1'b1;

    //Store Request Submission
    assign fn3 = LS_W_fn3;
    assign load = 1'b0;
    assign store = 1'b1;

    assign addr = data_in1;
    assign data = data_in2;
    assign new_request = data_valid_in1 && data_valid_in2 && !lsq_full;
    assign data_in_ack1 = data_valid_in1 && data_valid_in2 && !lsq_full;
    assign data_in_ack2 = data_valid_in1 && data_valid_in2 && !lsq_full;

    assign data_out = 0;
    assign data_valid_out = 0;
    
endmodule