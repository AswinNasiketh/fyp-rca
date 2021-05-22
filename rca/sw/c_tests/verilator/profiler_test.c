#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "static_region.h"



extern void trap_entry(void); //defined in trap_entry.S


int main(void) {

    void (*fn_pointer)() = &trap_entry;
    platform_init((uint32_t) trap_entry);

    printf("Test fn is at %x\n\r", (uint32_t)fn_pointer);

    volatile uint32_t test_num;

    for(int i = 0; i < 200; i++){
        test_num += i % 2;
    }

    for(int i = 0; i < 200; i++){
        test_num -= i % 2;
    }

    // printf("Final result ")
}