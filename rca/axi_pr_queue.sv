module axi_pr_queue
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
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

    output logic pr_request_pending
);

parameter ENQUEUE_W_ADDR = 2'b01;
parameter PEEK_R_ADDR = 2'b01;
parameter POP_R_ADDR = 2'b10;

typedef struct packed{
    logic [$clog2(GRID_NUM_COLS*GRID_NUM_ROWS)-1:0] grid_slot;
    logic [$clog2(NUM_OUS)-1:0] ou_id;
} pr_request_t;

fifo_interface #(.DATA_WIDTH($bits(pr_request_t))) pr_request_fifo_if ();

taiga_fifo #(.DATA_WIDTH($bits(pr_request_t)), .FIFO_DEPTH(MAX_PR_QUEUE_REQUESTS)) pr_request_fifo(
        .clk(s_axi_aclk),
        .rst(~s_axi_aresetn),
        .fifo(pr_request_fifo_if)
    );

logic push_pr_request;
logic pop_pr_request;
pr_request_t new_pr_request;
pr_request_t oldest_pr_request;

logic request_fifo_full;

assign oldest_pr_request = pr_request_fifo_if.data_out;
assign pr_request_pending = pr_request_fifo_if.valid;
assign request_fifo_full = pr_request_fifo_if.full;
assign pr_request_fifo_if.push = push_pr_request;
assign pr_request_fifo_if.potential_push = push_pr_request;
assign pr_request_fifo_if.data_in = new_pr_request;


// Bus write FSM
localparam [1:0]
    WSTATE_ADDR = 2'h0,
    WSTATE_DATA = 2'h1,
    WSTATE_WAIT = 2'h2,
    WSTATE_RESP = 2'h3;
reg [1:0] wstate;
reg [12:0] wcnt;
reg fill_i;

always @ (posedge s_axi_aclk) begin
    s_axi_awready <= 1'h0;
    s_axi_wready <= 1'h0;
    s_axi_bvalid <= 1'h0;
    
    push_pr_request <= 1'h0;
    if (s_axi_aresetn)
        case (wstate)
            WSTATE_ADDR:
                if (s_axi_awvalid && (s_axi_awaddr == ENQUEUE_W_ADDR) && ~request_fifo_full) begin //only allow writes if correct address is used and fifo isn't full
                    s_axi_awready <= 1'h1;
                    wstate <= WSTATE_DATA;
                end
            WSTATE_DATA:
                if (s_axi_wvalid) begin
                    wstate <= WSTATE_RESP;
                    s_axi_wready <= 1'h1;
                    push_pr_request <= 1'h1;
                    new_pr_request.grid_slot <= s_axi_wdata[$clog2(GRID_NUM_COLS*GRID_NUM_ROWS)-1:0];
                    new_pr_request.ou_id <= s_axi_wdata[$clog2(NUM_OUS)+$clog2(GRID_NUM_COLS*GRID_NUM_ROWS)-1:$clog2(GRID_NUM_COLS*GRID_NUM_ROWS)];
                end
            WSTATE_RESP:
                if (s_axi_bready)
                    wstate <= WSTATE_ADDR;
                else
                    s_axi_bvalid <= 1'h1;
            default:
                ;
        endcase
    else
        wstate <= WSTATE_ADDR;
end

// Bus read FSM
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