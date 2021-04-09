module grid_pr_slot
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,
    
    input [XLEN-1:0] data_in,
    input data_valid_in,

    output [XLEN-1:0] data_out,
    output data_valid_out
);
    
endmodule