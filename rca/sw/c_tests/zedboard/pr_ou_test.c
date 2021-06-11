#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "static_region.h"

extern void trap_entry(void); //defined in trap_entry.S

void test_add(static_region_t *pstatic_config){
    printf("Testing Add OU\n\r");
    uint32_t src_reg_addrs[NUM_READ_PORTS] = {5,28,0,0,0};
    uint32_t dst_fb_reg_addrs[NUM_WRITE_PORTS] = {7,0,0,0,0};
    uint32_t dst_nfb_reg_addrs[NUM_WRITE_PORTS] = {0,0,0,0,0};

    configure_src_regs(pstatic_config, RCA_A, src_reg_addrs);
    configure_fb_dst_regs(pstatic_config, RCA_A, dst_fb_reg_addrs);
    configure_nfb_dst_regs(pstatic_config, RCA_A, dst_nfb_reg_addrs);

    //passthrough
    // configure_grid_mux(pstatic_config, 0, 0, INP2, IO_UNIT);
    //add unit
    configure_grid_mux(pstatic_config, 1, 0, INP1, IO_UNIT);
    configure_grid_mux(pstatic_config, 1, 0, INP2, IO_UNIT);


    configure_io_unit_mux(pstatic_config, 0, RS1);
    configure_io_unit_mux(pstatic_config, 1, RS1);
    configure_io_unit_mux(pstatic_config, 2, CONST);

    bool io_inp_mask[NUM_IO_UNITS] = {false};
    io_inp_mask[0] = true;
    io_inp_mask[1] = true;

    configure_io_unit_inp_mask(pstatic_config, RCA_A, io_inp_mask);
    configure_fb_result_mux(pstatic_config, RCA_A, RD1, 2);
    configure_fb_result_mux(pstatic_config, RCA_A, RD2, UNUSED_WRITE_PORT_ADDR);
    configure_fb_result_mux(pstatic_config, RCA_A, RD3, UNUSED_WRITE_PORT_ADDR);
    configure_fb_result_mux(pstatic_config, RCA_A, RD4, UNUSED_WRITE_PORT_ADDR);
    configure_fb_result_mux(pstatic_config, RCA_A, RD5, UNUSED_WRITE_PORT_ADDR);

    configure_nfb_result_mux(pstatic_config, RCA_A, RD1, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD2, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD3, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD4, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD5, UNUSED_WRITE_PORT_ADDR);

    bool ls_mask[NUM_IO_UNITS] = {false};
    configure_fb_ls_mask(pstatic_config, RCA_A, ls_mask);
    configure_nfb_ls_mask(pstatic_config, RCA_A, ls_mask);

    printf("Writing static config with IO unit input diverted\n\r");
    write_config(pstatic_config, 0, 1, RCA_A);

    printf("Programming OU (not)\n\r");
    // send_pr_request(ADD, grid_coord_to_slot(1,0));
    // send_pr_request(PASSTHROUGH, grid_coord_to_slot(0,0));
    // char chr;
    // printf("Waiting for a character: \n\r");
    // scanf("%c", &chr);     

    // When %c is used, a character is displayed
    // printf("You entered %c.\n",chr);

    printf("Writing static config with IO unit input correct\n\r");
    configure_io_unit_mux(pstatic_config, 2, SLOT_0_OP);
    write_config(pstatic_config, 0, 1, RCA_A);

    printf("Using RCA\n\r");
    uint32_t dummy;
    uint32_t test;
    uint32_t adder;
    
    register int *t0Val asm("t0");
    register int *t1Val asm("t1");

    asm volatile("li %2, 0x80000005;"
    "li %3, 5678;"
    "rcaufb.a %3, %3, %3; rcaunfb.a %0, %0, %0;"
    "add %1, x7, x0;"
    "add %4, %3, x0"
    :"+r"(dummy), "=r"(test), "+r"(t0Val), "+r"(t1Val), "=r"(adder)
    :
    :
    );

    printf("Test: %u\n\r", test);
    // if(test == (1234+1234)){
    //     printf("Test passed!\n\r");
    // }else{
    //     printf("Test failed!\n\r");
    // }

}

bool test_comp_ou(static_region_t *pstatic_config, ou_t ou, uint32_t row, uint32_t col, int32_t inp_data, int32_t expected_output){
    printf("Testing OU %u in row %u col %u\n\r", ou, row, col);
    uint32_t src_reg_addrs[NUM_READ_PORTS] = {5,0,0,0,0};
    uint32_t dst_fb_reg_addrs[NUM_WRITE_PORTS] = {7,0,0,0,0};
    uint32_t dst_nfb_reg_addrs[NUM_WRITE_PORTS] = {0,0,0,0,0};

    configure_src_regs(pstatic_config, RCA_A, src_reg_addrs);
    configure_fb_dst_regs(pstatic_config, RCA_A, dst_fb_reg_addrs);
    configure_nfb_dst_regs(pstatic_config, RCA_A, dst_nfb_reg_addrs);


    configure_grid_mux(pstatic_config, row, col, INP1, IO_UNIT);
    configure_grid_mux(pstatic_config, row, col, INP2, IO_UNIT);


    configure_io_unit_mux(pstatic_config, row, RS1);
    configure_io_unit_mux(pstatic_config, row+1, CONST);

    bool io_inp_mask[NUM_IO_UNITS] = {false};
    io_inp_mask[row] = true;

    configure_io_unit_inp_mask(pstatic_config, RCA_A, io_inp_mask);
    configure_fb_result_mux(pstatic_config, RCA_A, RD1, row+1);
    configure_fb_result_mux(pstatic_config, RCA_A, RD2, UNUSED_WRITE_PORT_ADDR);
    configure_fb_result_mux(pstatic_config, RCA_A, RD3, UNUSED_WRITE_PORT_ADDR);
    configure_fb_result_mux(pstatic_config, RCA_A, RD4, UNUSED_WRITE_PORT_ADDR);
    configure_fb_result_mux(pstatic_config, RCA_A, RD5, UNUSED_WRITE_PORT_ADDR);

    configure_nfb_result_mux(pstatic_config, RCA_A, RD1, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD2, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD3, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD4, UNUSED_WRITE_PORT_ADDR);
    configure_nfb_result_mux(pstatic_config, RCA_A, RD5, UNUSED_WRITE_PORT_ADDR);

    bool ls_mask[NUM_IO_UNITS] = {false};
    configure_fb_ls_mask(pstatic_config, RCA_A, ls_mask);
    configure_nfb_ls_mask(pstatic_config, RCA_A, ls_mask);

    // printf("Writing static config with IO unit input diverted\n\r");
    write_config(pstatic_config, row, row, RCA_A);

    // printf("Programming OU\n\r");
    send_pr_request(ou, grid_coord_to_slot(row,col));

    // char chr;
    // printf("Waiting for a character: \n\r");
    // scanf("%c", &chr);     

    // When %c is used, a character is displayed
    // printf("You entered %c.\n",chr);

    // printf("Writing static config with output IO unit input correct\n\r");
    configure_io_unit_mux(pstatic_config, row+1, 5+col);
    write_config(pstatic_config, row, row, RCA_A);

    // printf("Using RCA\n\r");
    uint32_t dummy;
    int32_t test_result;
    

    asm volatile("mv x5, %2;"
    "rcaufb.a %0, %0, %0; rcaunfb.a %0, %0, %0;"
    "mv %1, x7;"
    :"+r"(dummy), "=r"(test_result)
    :"r"(inp_data)
    :
    );

    printf("Input Value: %d\n\r", inp_data);
    printf("Test Result: %d\n\r", test_result);
    printf("Expected Result: %d\n\r", expected_output);

    if(test_result == expected_output){
        printf("Test passed!\n\r");
    }else{
        printf("Test failed!\n\r");
    }

    // printf("Programming OU back to unused\n\r");
    send_pr_request(UNUSED, grid_coord_to_slot(row,col));
    return test_result == expected_output;
}

const ou_t ous_to_test[] = {
    ADD,
    AND,
    AUIPC,
    LUI,
    OR,
    SLL,
    SLT,
    SLTU,
    SRA,
    SRL,
    SUB,
    XOR
};

const int32_t test_inputs[] = {
    1234,
    5678,
    0x00012345,
    0x00012345,
    0x6789ABCD,
    24,
    25,
    8,
    0x80000005,
    0x80000006,
    777,
    0xEF123456
};

const int32_t expected_outputs[] = {
    2468,
    5678,
    0x12357345,
    0x12345000,
    0x6789ABCD,
    402653184,
    0,
    0,
    0xFC000000,
    0x02000000,
    0,
    0
};


int main(void) {
    void (*fn_pointer)() = &trap_entry;
    platform_init((uint32_t) trap_entry);

    // printf("Test fn is at %x\n\r", (uint32_t)fn_pointer);
    printf("Hello World!\n\r");
    static_region_t static_conf;
    // test_add(&static_conf);
    printf("Integer Computational OU Tests\n\r");
    bool all_tests_passed = true;
    for(int i = 0; i < 12; i++){
        for(int j = 0; j < NUM_GRID_ROWS; j++){
            for(int k = 0; k < NUM_GRID_COLS; k++){
                static_region_t static_conf;
                if(!test_comp_ou(&static_conf, ous_to_test[i], j, k, test_inputs[i], expected_outputs[i])) all_tests_passed = false;
            }
        }        
    }

    printf("All tests passed? : %u\n\r", all_tests_passed);    
}

