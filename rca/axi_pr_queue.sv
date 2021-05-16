module axi_pr_queue
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(   
    //AXI interface
    input wire [1:0] s_axi_awaddr,
	input wire s_axi_awvalid,
	output reg s_axi_awready,
	input wire [31:0] s_axi_wdata,
	input wire s_axi_wvalid,
	output reg s_axi_wready,
	output reg s_axi_bvalid,
	input wire s_axi_bready,
	input wire [1:0] s_axi_araddr,
	input wire s_axi_arvalid,
	output reg s_axi_arready,
	output reg [31:0] s_axi_rdata,
	output reg s_axi_rvalid,
	input wire s_axi_rready,
	input wire s_axi_aclk,
	input wire s_axi_aresetn,

    //Interrupt and RCA stall signal
    output logic pr_request_pending,

    //Taiga Interfaces
    unit_issue_interface.unit issue,
    input pr_queue_inputs_t pr_queue_inputs,
    unit_writeback_interface.unit wb
);

// parameter ENQUEUE_W_ADDR = 2'b01;
parameter PEEK_R_ADDR = 2'b01;
parameter POP_R_ADDR = 2'b10;

fifo_interface #(.DATA_WIDTH($bits(pr_queue_inputs_t))) pr_request_fifo_if ();

taiga_fifo #(.DATA_WIDTH($bits(pr_queue_inputs_t)), .FIFO_DEPTH(MAX_PR_QUEUE_REQUESTS)) pr_request_fifo(
        .clk(s_axi_aclk),
        .rst(~s_axi_aresetn),
        .fifo(pr_request_fifo_if)
    );

logic pop_pr_request;

pr_queue_inputs_t oldest_pr_request;

logic request_fifo_full;

assign oldest_pr_request = pr_request_fifo_if.data_out;
assign pr_request_pending = pr_request_fifo_if.valid;
assign request_fifo_full = pr_request_fifo_if.full;
assign pr_request_fifo_if.push = issue.new_request;
assign pr_request_fifo_if.potential_push = push_pr_request;
assign pr_request_fifo_if.data_in = pr_queue_inputs;

//No AXI write interface - Queue pushes are done through Taiga Issue interface
assign s_axi_awready = 1'h0;
assign s_axi_wready = 1'h0;
assign s_axi_bvalid = 1'h0;

always @(posedge clk) 
    if(~waiting_for_ack) begin
        wb.done <= issue.new_request;
        wb.id <= issue.id;
    end


assign wb.rd = 0;
logic waiting_for_ack; 

set_clr_reg_with_rst #(.SET_OVER_CLR(1), .WIDTH(1), .RST_VALUE(0)) wb_ack_wait (
      .clk, .rst,
      .set(issue.new_request),
      .clr(wb.ack),
      .result(waiting_for_ack)
    );

assign issue.ready = !waiting_for_ack && !request_fifo_full;
    

// Bus read FSM - Taken from James Davis' URAM Checkerboard
localparam [1:0]
    RSTATE_ADDR = 2'h0,
    RSTATE_WAIT = 2'h1,
    RSTATE_CAPTURE = 2'h2,
    RSTATE_DATA = 2'h3;
reg [1:0] rstate;

logic pop;
always @ (posedge s_axi_aclk) begin
    s_axi_arready <= 1'h0;
    s_axi_rvalid <= 1'h0;

    pop_pr_request <= 1'h0;  
    if (s_axi_aresetn)
        case (rstate)
            RSTATE_ADDR:
                if (s_axi_arvalid && pr_request_pending) begin
                    rstate <= RSTATE_CAPTURE;
                    s_axi_arready <= 1'h1;
                    pop <= (s_axi_araddr == POP_R_ADDR); //just determine whether its a pop or a peek
                end
            RSTATE_CAPTURE: begin
                rstate <= RSTATE_DATA;
                s_axi_rdata <= {oldest_pr_request.ou_id, oldest_pr_request.grid_slot};
                s_axi_rvalid <= 1'h1;
                pop_pr_request <= pop;
            end
            RSTATE_DATA: begin
                if (s_axi_rready)
                    rstate <= RSTATE_ADDR;
                else
                    s_axi_rvalid <= 1'h1;
            end
            default:
                ;
        endcase
    else
        rstate <= RSTATE_ADDR;
end    

endmodule