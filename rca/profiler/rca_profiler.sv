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


endmodule