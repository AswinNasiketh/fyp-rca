//Partial Reconfiguration request driver

#ifndef PR_H
#define PR_H

#include "rca.h"

typedef enum{
    UNUSED = 0,
    PASSTHROUGH = 1,
    ADD = 2,
    AND = 3,
    AUIPC = 4,
    LB = 5,
    LBU = 6,
    LH = 7,
    LHU = 8,
    LUI = 9,
    LW = 10,
    OR = 11,
    SB = 12,
    SH = 13,
    SLL = 14,
    SLT = 15,
    SLTU = 16,
    SRA = 17,
    SRL = 18,
    SUB = 19,
    SW = 20,
    XOR = 21
}ou_t;

#define NUM_OUS 22


void send_pr_request(ou_t ou, uint32_t grid_slot){
    uint32_t dummy;
    uint32_t rs2 = ou;
    asm volatile("rcapprq.d %0, %1, %2;"
        : "=r"(dummy)
        : "r"(grid_slot), "r"(rs2)
        :
    );
}


#endif //PR_H