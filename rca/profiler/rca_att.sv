//Accelerator Trigger Table - to be integrated with fetch unit
module rca_att
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,

    unit_issue_interface.unit issue,
    unit_writeback_interface.unit wb,
    input att_inputs_t att_inputs,

    att_fetch_interface.att fetch
);

    typedef struct packed{
        logic [XLEN-1:0] sbb_addr;
        logic [XLEN-1:0] loop_start_addr;
        logic valid;
    } att_entry_t;


    att_entry_t rca_triggers [NUM_RCAS];

    logic [NUM_RCAS-1:0] entry_hit; 
    logic [$clog2(NUM_RCAS)-1:0] hit_way;
    logic [$clog2(NUM_RCAS)-1:0] hit_way_r;
    always_comb
        for(int i = 0; i < NUM_RCAS; i++)
            entry_hit[i] = (fetch.if_pc == rca_triggers[i].loop_start_addr) && rca_triggers[i].valid; 

    //taken from Branch Predictor
    one_hot_to_integer #(NUM_RCAS) hit_way_conv (.*, .one_hot(entry_hit), .int_out(hit_way));

    always_ff @(posedge clk)
        hit_way_r <= hit_way;

    //RCA use instruction definitions
    localparam [XLEN-1:0] rca_fb_instrs [NUM_RCAS] = '{32'h0000002b, 32'h0000102b, 32'h0000202b, 32'h0000302b};
    localparam [XLEN-1:0] rca_nfb_instrs [NUM_RCAS] = '{32'h0200002b, 32'h0200102b, 32'h0200202b, 32'h0200302b};


    //state machine to insert 2 instructions
    localparam STATE_NO_TRIGGER = 1'd0;
    localparam STATE_INJECTED_FB_INSTR = 1'd1;

    logic curr_state;
    logic next_state;

    always_ff @(posedge clk)
        if(rst)
            curr_state <= STATE_NO_TRIGGER;
        else
            curr_state <= next_state;

    always_comb
        case (curr_state)
            STATE_NO_TRIGGER: next_state = (|entry_hit) ? STATE_INJECTED_FB_INSTR : STATE_NO_TRIGGER;
            STATE_INJECTED_FB_INSTR: next_state = STATE_NO_TRIGGER;
            default: next_state = STATE_NO_TRIGGER;
        endcase

    always_comb begin
        fetch.att_override = 0;
        fetch.override_instr = '0;
        fetch.next_pc_override = '0;
        case(curr_state)
            STATE_NO_TRIGGER: begin
                if(|entry_hit) begin
                    fetch.att_override = 1;
                    fetch.override_instr = rca_fb_instrs[hit_way];
                    fetch.next_pc_override = fetch.if_pc + 32'd4;
                end
            end
            STATE_INJECTED_FB_INSTR: begin
                fetch.att_override = 1;
                fetch.override_instr = rca_nfb_instrs[hit_way_r];
                fetch.next_pc_override = rca_triggers[hit_way_r].sbb_addr;
            end
        endcase
    end

    //CPU Control Interface
    localparam FIELD_SBB_ADDR = 0;
    localparam FIELD_LOOP_START_ADDR = 1;
    localparam FIELD_VALID = 2;

    always_ff @(posedge clk)
        if(issue.new_request)
            case(att_inputs.field_id)
                FIELD_SBB_ADDR: rca_triggers[att_inputs.rca_addr].sbb_addr <= att_inputs.field_value;
                FIELD_LOOP_START_ADDR: rca_triggers[att_inputs.rca_addr].loop_start_addr <= att_inputs.field_value;
                FIELD_VALID: rca_triggers[att_inputs.rca_addr].valid <= att_inputs.field_value[0];
            endcase
        else if(rst)
            for(int i = 0 ; i < NUM_RCAS; i++) begin
                rca_triggers[i].sbb_addr <= '0;
                rca_triggers[i].loop_start_addr <= '0;
                rca_triggers[i].valid <= '0;
            end

    logic waiting_for_ack; 

    set_clr_reg_with_rst #(.SET_OVER_CLR(1), .WIDTH(1), .RST_VALUE(0)) wb_ack_wait (
        .clk, .rst,
        .set(issue.new_request),
        .clr(wb.ack),
        .result(waiting_for_ack)
        );

    //RCA Use counter for Testing purposes
    logic [XLEN-1:0] use_counter;
    always_ff @(posedge clk)
        if(rst)
            use_counter <= '0;
        else
            if((|entry_hit) == 1'b1)
                use_counter <= use_counter + 'd1;

    assign wb.done = waiting_for_ack;
    assign issue.ready = !waiting_for_ack;
    assign wb.rd = use_counter;

    always_ff @(posedge clk)
        if(~waiting_for_ack)
            wb.id <= issue.id;

endmodule