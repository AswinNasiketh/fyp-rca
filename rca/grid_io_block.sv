module grid_io_block
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,
    input gci_data_valid_in,
    input [XLEN-1:0] gci_data_in,

    input row_data_valid_in,
    input [XLEN-1:0] row_data_in,

    input output_mode, //use RCA result MUX sel regfile for this
    input fifo_rst, // control signal from RCA control unit
    input fifo_pop, // control signal from internal RCA writeback unit

    //for tracking LS request submission
    input new_ls_request,
    input ls_request_ack,
    output ls_requested,

    output gwb_data_valid_out,
    output [XLEN-1:0] gwb_data_out,
    output row_data_valid_out,
    output [XLEN-1:0] row_data_out,

    input pr_requests_incomplete
);

    //FIFO to store accelerator output data when in output mode
    fifo_interface #(.DATA_WIDTH(XLEN)) oldest_data ();

    taiga_fifo #(.DATA_WIDTH(XLEN), .FIFO_DEPTH(MAX_IDS)) data_fifo (
        .clk, .rst(fifo_rst | rst),
        .fifo(oldest_data)
    );

    assign oldest_data.pop = fifo_pop;
    assign oldest_data.data_in = row_data_in;
    assign oldest_data.potential_push = row_data_valid_in && output_mode && !pr_requests_incomplete;
    assign oldest_data.push = row_data_valid_in && output_mode && !pr_requests_incomplete;

    logic [$clog2(MAX_IDS):0] num_ls_requests;
    initial num_ls_requests = 0;
    always_ff @(posedge clk) begin
        if(rst || fifo_rst) num_ls_requests <= 0;
        else if(!pr_requests_incomplete) num_ls_requests <= num_ls_requests + ($clog2(MAX_IDS)+1)'(new_ls_request) - ($clog2(MAX_IDS)+1)'(ls_request_ack);
    end
    assign ls_requested = (num_ls_requests > 0);


    assign row_data_valid_out = gci_data_valid_in;
    assign row_data_out = gci_data_in;

    assign gwb_data_out = oldest_data.data_out;
    assign gwb_data_valid_out = oldest_data.valid;  
endmodule