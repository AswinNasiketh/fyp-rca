module grid_io_block
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input data_valid_in,
    input [XLEN-1:0] data_in,
    output data_valid_out,
    output [XLEN-1:0] data_out
);

//Note: Module will have more things related to control signals in the future

assign data_valid_out = data_valid_in;
assign data_out = data_valid_out;
    
endmodule