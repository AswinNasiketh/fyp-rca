module rca_profiler
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,
    
    //Signals from branch unit
    input branch_instr_issue,
    input [XLEN-1:0] branch_instr_pc,
    input [20:0] branch_pc_offset,
    input branch_taken,

    output profiler_exception,
    input profiler_inputs_t profiler_inputs,
    unit_issue_interface.unit issue,
    unit_writeback_interface.unit wb,
);

    //Short backward branch detection
    logic sbb;
    logic cache_operation; //to know whether its a branch instruction which is taken
    logic profiler_lock; //To lock profile cache when sequence selection routines are running - TODO:set using custom instr

    assign sbb = (signed'(branch_pc_offset) < 20'd0) && (signed'(branch_pc_offset) > SBB_MAX_OFFSET);

    assign cache_operation = branch_instr_issue && branch_taken && ~profiler_lock;

    //Profiler Data Structure
    typedef struct packed{
        logic [XLEN-1:0] branch_instr_addr;
        logic entry_valid;

        logic [$clog2(MAX_TAKEN_COUNT)-1:0] taken_count;
    }profiler_entry_t;

    profiler_entry_t profiler_data[NUM_PROFILER_ENTRIES];

    //Taken Count Increment Mechanism
    logic [NUM_PROFILER_ENTRIES-1:0] addr_match;
    logic [NUM_PROFILER_ENTRIES-1:0] max_reached; 
    logic shift_required;
    logic [$clog2(MAX_TAKEN_COUNT)-1:0] next_taken_count [NUM_PROFILER_ENTRIES];

    always_comb
        for(int i = 0; i < NUM_PROFILER_ENTRIES; i++)
            addr_match[i] = cache_operation && profiler_data[i].valid && (profiler_data[i].branch_instr_addr == branch_instr_pc);

    always_comb
        for(int i = 0; i < NUM_PROFILER_ENTRIES; i++)
            max_reached[i] = &profiler_data[i].taken_count;

    assign shift_required = |(addr_match & max_reached);

    always_comb
        if(shift_required)
            for(int i = 0; i < NUM_PROFILER_ENTRIES; i++)
                next_taken_count[j] = profiler_data[i].taken_count >> 1;
        
        for(int j = 0; j < NUM_PROFILER_ENTRIES; i++)
            if(addr_match[j])
                next_taken_count[j] = profiler_data[j].taken_count + 1;
    end

    //Profile Cache Replacement Mechanism
    logic new_cache_entry;
    logic [$clog2(NUM_PROFILER_ENTRIES)-1:0] entry_lowest_hits;
    logic [$clog2(NUM_PROFILER_ENTRIES)-1:0] next_invalid_entry;
    logic any_invalid_entries;
    logic [$clog2(NUM_PROFILER_ENTRIES)-1:0] next_entry_to_replace;
    logic [$clog2(MAX_TAKEN_COUNT)-1:0] lowest_taken_count;

    assign new_cache_entry = cache_operation && sbb && (~(|addr_match)); 

    always_comb begin
        entry_lowest_hits = 0;
        lowest_taken_count = '1;
        for(int i = 0; i < NUM_PROFILER_ENTRIES; i++)
            if(profiler_data[i].taken_count < lowest_taken_count) begin
                lowest_taken_count = profiler_data[i].taken_count;
                entry_lowest_hits = ($clog2(NUM_PROFILER_ENTRIES))'(i);
            end
    end

    always_comb begin
        next_invalid_entry = 0;
        any_invalid_entries = 0;
        while(!any_invalid_entries && next_invalid_entry < ($clog2(NUM_PROFILER_ENTRIES))'(NUM_PROFILER_ENTRIES)) begin
            any_invalid_entries = profiler_data[next_invalid_entry].entry_valid;
            next_invalid_entry = next_invalid_entry + ($clog2(NUM_PROFILER_ENTRIES))'(~any_invalid_entries);
        end
    end

    assign next_entry_to_replace = any_invalid_entries ? next_invalid_entry : entry_lowest_hits;

    //Updating Profile Cache
    always_ff @(posedge clk) begin
        for (int i = 0; i < NUM_PROFILER_ENTRIES; i++)
            profiler_data[i].taken_count = next_taken_count[i];
        
        if(new_cache_entry) begin
            profiler_data[next_entry_to_replace].branch_instr_addr = branch_instr_pc;
            profiler_data[next_entry_to_replace].taken_count = 1;
            profiler_data[next_entry_to_replace].valid = 1;
        end
    end

    //CPU interface

    always_ff @(posedge clk)
        if(issue.new_request && profiler_inputs.toggle_lock)
            profiler_lock <= !profiler_lock;

    logic waiting_for_ack; 

    set_clr_reg_with_rst #(.SET_OVER_CLR(1), .WIDTH(1), .RST_VALUE(0)) wb_ack_wait (
        .clk, .rst,
        .set(issue.new_request),
        .clr(wb.ack),
        .result(waiting_for_ack)
        );

    assign wb.done = waiting_for_ack;
    assign issue.ready = !waiting_for_ack;

    localparam FIELD_BRANCH_ADDR = 32'd0;
    localparam FIELD_ENTRY_VALID = 32'd1;
    localparam FIELD_TAKEN_COUNT = 32'd2;

    always_ff @(posedge clk) begin
        case(profiler_inputs.field_id)
            FIELD_BRANCH_ADDR: wb.rd <= 32'(profiler_data[profiler_inputs.entry_index].branch_instr_addr);
            FIELD_ENTRY_VALID: wb.rd <= 32'(profiler_data[profiler_inputs.entry_index].entry_valid);
            FIELD_TAKEN_COUNT: wb.rd <= 32'(profiler_data[profiler_inputs.entry_index].taken_count);
        endcase
    end

    always_ff @(posedge clk)
        if(~waiting_for_ack)
            wb.id <= issue.id;

    //Exception Generation
    logic [NUM_PROFILER_ENTRIES-1:0] threshold_reached;
    logic [NUM_PROFILER_ENTRIES-1:0] threshold_reached_r;

    always_comb
        for(int i = 0; i < NUM_PROFILER_ENTRIES; i++)
            threshold_reached[i] = profiler_entry[i].taken_count >= ($clog2(MAX_TAKEN_COUNT))'(TAKEN_COUNT_THRESHOLD);
    
    always_ff @(posedge clk)
        threshold_reached_r[i] <= threshold_reached[i];

    always_ff @(posedge clk)
        profiler_exception <= |(threshold_reached & (~threshold_reached_r)) & (~profiler_exception); //only generate exception when a taken count crosses the threshold and we haven't already generated an exception last cycle
endmodule