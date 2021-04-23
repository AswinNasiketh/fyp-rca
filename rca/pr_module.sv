module pr_module
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

    //LSQ interface
    output [XLEN-1:0] addr, 
    output [XLEN-1:0] data,
    output [2:0] fn3,
    output load,
    output store,
    output new_request,
    input lsq_full
);

    assign data_out = data_in1;
    assign data_valid_out = data_valid_in1;

    assign data_in_ack1 = data_valid_in1;
    assign data_in_ack2 = 0;    
endmodule