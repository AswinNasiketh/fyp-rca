module grid_control
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,

    unit_issue_interface.unit issue,
    input rca_inputs_t rca_inputs,

    input wb_committing,
    output id_t wb_id,
    output wb_fb_instr,
    output fifo_populated,

    output buf_data_valid, //if 1, on next cycle, data can be taken from buffer
    output clear_fifos, //if 1, means we are switching from one accelerator to another => fifos and load store counters should be cleared
    output [$clog2(NUM_RCAS)-1:0] currently_running_rca,

    output [XLEN-1:0] buf_rs_data [NUM_READ_PORTS],
    output [$clog2(NUM_RCAS)-1:0] rca_sel_buf
);

    localparam ACCEPTING_ISSUE_STATE = 1'b0;
    localparam WAIT_FOR_FIFO_EMPTY_STATE = 1'b1;

    /////////////////Issue Side Control////////////////////

    //Buffer stage to determine whether instruction targets new RCA to one that is running
    rca_inputs_t rca_inputs_buf;
    id_t id_buf;

    always_ff @(posedge clk) if (issue.new_request) rca_inputs_buf <= rca_inputs;
    always_ff @(posedge clk) if (issue.new_request) id_buf <= issue.id;
    always_ff @(posedge clk) if (buf_data_valid) currently_running_rca <= rca_inputs_buf.rca_sel;

    logic rca_match = fifo_populated ? (currently_running_rca == rca_inputs.rca_sel) : 1'b1; //Note: rca_sel is aligned with new_request    

    logic current_state;
    logic next_state;
    always_ff @(posedge clk) current_state <= next_state;
    
    //next state logic
    always_comb begin
        case (current_state):
            ACCEPTING_ISSUE_STATE: begin
                next_state = issue.new_request && rca_inputs.rca_use_instr && fifo_populated && ~rca_match;
            end
            WAIT_FOR_FIFO_EMPTY_STATE: begin
                next_state = fifo_populated;
            end
    end

    //state machine output signals
    assign issue.ready = (next_state == ACCEPTING_ISSUE_STATE && current_state == ACCEPTING_ISSUE_STATE); //only in ACCEPTING_ISSUE_STATE because the checks on whether the current request (if any) is using the RCA which is active (if any are active) only happen in ACCEPTING_ISSUE_STATE

    assign buf_data_valid = ((next_state == ACCEPTING_ISSUE_STATE) && (current_state == WAIT_FOR_FIFO_EMPTY_STATE)) || (current_state == ACCEPTING_ISSUE_STATE && issue.new_request_r && rca_inputs_buf.rca_use_instr); //Buffer data is valid for issue to grid when we are moving out of WAIT_FOR_FIFO_EMPTY state since this data has been waiting in the buffer for the FIFO to empty. It is also valid when we have had a new request and haven't moved into WAIT_FOR_FIFO_EMPTY state (i.e. there was a new request on the previous cycle and we are still in ACCEPTING_ISSUE_STATE)

    assign clear_fifos = ((next_state == ACCEPTING_ISSUE_STATE) && (current_state == WAIT_FOR_FIFO_EMPTY_STATE)) || (current_state == ACCEPTING_ISSUE_STATE && issue.new_request_r && rca_inputs_buf.rca_use_instr && ~fifo_populated) //We must clear IO unit FIFOs whenever we move to using a new accelerator since the IO FIFOs for those accelerators may have erratic values accumulated over time. This cleary happens when we move out of WAIT_FOR_FIFO_EMPTY state and it also happens whenever we're in ACCEPTING_ISSUE_STATE and a request has been buffered (therefore we haven't moved into WAIT_FOR_FIFO_EMPTY), and the reason for not moving into WAIT_FOR_FIFO_EMPTY is because the ID FIFO was empty

    //FIFO to keep track of IDs of each RCA instruction in pipeline
    fifo_interface #(.DATA_WIDTH($bits(id_t))) rca_ids_fifo_if ();

    //FIFO to store ordering of IDs
    taiga_fifo #(.DATA_WIDTH($bits(id_t)), .FIFO_DEPTH(MAX_IDS)) rca_ids_fifo (
        .clk, .rst,
        .fifo(rca_ids_fifo_if)
    );
    
    assign rca_ids_fifo_if.pop = wb_committing;
    assign rca_ids_fifo_if.data_in = id_buf;
    assign rca_ids_fifo_if.potential_push = buf_data_valid;
    always_ff @(posedge clk) rca_ids_fifo_if.push <= rca_ids_fifo_if.potential_push;
    assign wb_id = rca_ids_fifo_if.data_out;
    assign fifo_populated = rca_ids_fifo_if.valid;

    //Registers to store whether its a feedback or non-feedback use instruction
    logic is_fb_instr [MAX_IDS];
    always_ff @(posedge clk) if (buf_data_valid) is_fb_instr[id_buf] <= rca_inputs_buf.rca_use_fb_instr;
    assign wb_fb_instr = is_fb_instr[wb_id];

    //RS data to buffered read data output
    assign buf_rs_data[0] = rca_inputs_buf.rs1;
    assign buf_rs_data[1] = rca_inputs_buf.rs2;
    assign buf_rs_data[2] = rca_inputs_buf.rs3;
    assign buf_rs_data[3] = rca_inputs_buf.rs4;
    assign buf_rs_data[4] = rca_inputs_buf.rs5;

    assign rca_sel_buf = rca_inputs_buf.rca_sel;

endmodule