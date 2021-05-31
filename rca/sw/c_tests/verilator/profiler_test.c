#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "static_region.h"



extern void trap_entry(void); //defined in trap_entry.S


int main(void) {

    void (*fn_pointer)() = &trap_entry;
    platform_init((uint32_t) trap_entry);

    printf("Test fn is at %x\n\r", (uint32_t)fn_pointer);

    uint32_t test_num = 2000;

    for(int i = 0; i < 200; i++){
        test_num++;
        asm("");
    }

    for(int i = 0; i < 205; i++){
        test_num -= i;
    }

    printf("Final result %u\n\r", test_num);
}