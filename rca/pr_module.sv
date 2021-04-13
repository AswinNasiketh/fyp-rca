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
    output data_in_ack //just output high when the number of data valid signal for the input(s) this module requires is asserted
);
    
endmodule