#include "seqselect.h"

void handle_profiler_exception(){
    toggle_profiler_lock();
    profiler_entry_t profiler_entries[NUM_PROFILER_ENTRIES];

    for(int i = 0; i < NUM_PROFILER_ENTRIES; i++){
        profiler_entries[i] = get_profiler_entry(i);
    }

    for(int i = 0 ; i < NUM_PROFILER_ENTRIES; i++){
        printf("Profiler entry %u \n\r", i);
        printf("Branch Address: %x \n\r", profiler_entries[i].branch_addr);
        printf("Valid: %u \n\r", (uint32_t) profiler_entries[i].entry_valid);
        printf("Taken Count: %u \n\r", profiler_entries[i].taken_count);
    }
    toggle_profiler_lock();
}