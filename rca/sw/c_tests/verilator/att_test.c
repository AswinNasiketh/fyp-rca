#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "static_region.h"



extern void trap_entry(void); //defined in trap_entry.S


int main(void) {

    set_att_field(RCA_A, SBB_ADDR, 0x80000060);
    set_att_field(RCA_A, LOOP_START_ADDR, 0x80000050);
    set_att_field(RCA_A, ACC_ENABLE, 1);

        for(int i = 0; i < 10; i++){
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
    uint32_t num_triggers = set_att_field(RCA_A, ACC_ENABLE, 1);
    printf("Num ATT triggers: %u\n\r", num_triggers);
}