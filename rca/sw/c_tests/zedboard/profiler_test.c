#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "static_region.h"



extern void trap_entry(void); //defined in trap_entry.S

void print_entries(){
    profiler_entry_t profiler_entry;
    toggle_profiler_lock();
    for(int i = 0; i < NUM_PROFILER_ENTRIES; i++){                
        profiler_entry = get_profiler_entry(i);
        printf("Profiler entry %u, Branch Address:%x, Entry Valid: %u, Taken Count %u\n\r", i, profiler_entry.branch_addr, profiler_entry.entry_valid, profiler_entry.taken_count);
    }
    toggle_profiler_lock();
    return;
}

int main(void) {

    void (*fn_pointer)() = &trap_entry;
    platform_init((uint32_t) trap_entry);

    // printf("Test fn is at %x\n\r", (uint32_t)fn_pointer);

    for(int i = 0; i < 200; i++){
        // test_num++;

        // print_entries();   
    
        asm volatile("add t0, t1, t2;"
        "sub t2, t0, t1;"
        "sltu t0, t0, t2;"
        "xor t0, t1, t2;"
        :
        :
        :"t0", "t2"
        );
    }

    uint32_t num_triggers = set_att_field(RCA_A, ACC_ENABLE, 0);
    toggle_profiler_lock();
    printf("Num ATT triggers: %u\n\r", num_triggers);
}