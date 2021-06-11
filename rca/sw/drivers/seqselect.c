#include "seqselect.h"


void handle_profiler_exception(){
    toggle_profiler_lock();

    static seq_profile_t seq_profiles[MAX_STORED_SEQ_PROFS];
    static uint32_t next_seq_prof_index = 0;

    profiler_entry_t profiler_entry;
    uint32_t raw_branch_instr;
    uint32_t loop_start_addr;
    int32_t offset; 
    int32_t seq_prof_index;

    //Update/Create Sequence Profiles

    for(int i = 0; i < next_seq_prof_index; i++){
        seq_profiles[i].in_hw_profiler = false;
    }

    for(int i = 0; i < NUM_PROFILER_ENTRIES; i++){
        profiler_entry = get_profiler_entry(i);

        if(profiler_entry.taken_count >= SEQ_PROFILE_THRESH){
            raw_branch_instr = *((uint32_t*) profiler_entry.branch_addr);
            offset = get_branch_offset(raw_branch_instr);

            loop_start_addr = profiler_entry.branch_addr + offset;
            seq_prof_index = find_seq_profile(loop_start_addr, seq_profiles, next_seq_prof_index);

            if(seq_prof_index == -1){
                printf("New Sequence found, attempting to profile\n\r");
                if(next_seq_prof_index >= MAX_STORED_SEQ_PROFS){
                    printf("Sequence Profile Array Full. Could not add new entry\n\r");
                }else{
                    instr_seq_t instr_seq;
                    printf("Analysing instruction sequence\n\r");
                    bool seq_supported = analyse_instr_seq(profiler_entry.branch_addr, offset, &instr_seq);
                    // print_instr_seq(instr_seq);
                    if(seq_supported){
                        printf("Sequence is supported, profiling\n\r");
                        seq_profiles[next_seq_prof_index] = profile_seq(&instr_seq, &profiler_entry);
                    }

                    seq_profiles[next_seq_prof_index].seq_supported = seq_supported;
                    seq_profiles[next_seq_prof_index].loop_start_addr = loop_start_addr; //update anyway so address is set for unsupported sequences
                    seq_profiles[next_seq_prof_index].in_hw_profiler = true;

                    next_seq_prof_index++;
                }                
            }else{
                printf("Already profiled sequence in profiler, updating profiler values\n\r");
                seq_profiles[seq_prof_index].taken_count = profiler_entry.taken_count; //will overwrite previous taken count if sequence REENTERS hardware profiler
                seq_profiles[seq_prof_index].in_hw_profiler = true;            
            }
        }
    }

    //Choose accelerator
    select_accelerators(seq_profiles, next_seq_prof_index);

    toggle_profiler_lock();
}







