#include "seqselect.h"

int32_t get_branch_offset(uint32_t branch_instr){
    uint32_t imm11 = (branch_instr & (0x00000001 << 7)) << 3;
    uint32_t imm4_1 = (branch_instr & (0x0000000F << 8)) >> 8;
    uint32_t imm10_5 = (branch_instr & (0x0000003F << 25)) >> 21;
    uint32_t imm12 = (branch_instr & (0x00000001 << 31)) >> 20;

    int32_t imm = imm4_1 | imm10_5 | imm11 | imm12;
    imm = (imm << 20) >> 20; //conversion to signed if signed using arithmetic shift
    
    return imm << 1; //offset is encoded in multiple of 2 bytes
}

void handle_profiler_exception(){
    toggle_profiler_lock();
    profiler_entry_t profiler_entries[NUM_PROFILER_ENTRIES];

    for(int i = 0; i < NUM_PROFILER_ENTRIES; i++){
        profiler_entries[i] = get_profiler_entry(i);
    }

    uint32_t branch_inst = *((uint32_t*) profiler_entries[0].branch_addr);
    printf("Branch Instruction at %x: %x \n\r", profiler_entries[0].branch_addr, branch_inst);
    int32_t offset = get_branch_offset(branch_inst);
    printf("Branch offset: %d \n\r", offset);
    toggle_profiler_lock();
}