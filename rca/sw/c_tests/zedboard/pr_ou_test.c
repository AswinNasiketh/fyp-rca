#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "pr.h"
#include "static_region.h"

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
    configure_io_unit_mux(pstatic_config, 2, SLOT_0_OP);

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

    printf("Writing static config\n\r");
    write_config(pstatic_config, 0, 1, RCA_A);

    printf("Programming OU\n\r");
    send_pr_request(ADD, grid_coord_to_slot(1,0));
    // send_pr_request(PASSTHROUGH, grid_coord_to_slot(0,0));

    printf("Using RCA\n\r");
    uint32_t dummy;
    uint32_t test;
    uint32_t adder;
    
    register int *t0Val asm("t0");
    register int *t1Val asm("t1");

    asm volatile("li %2, 1234;"
    "li %3, 5678;"
    "rcaufb.a %3, %3, %3; rcaunfb.a %0, %0, %0;"
    "add %1, x7, x0;"
    "add %4, %3, x0"
    :"+r"(dummy), "=r"(test), "+r"(t0Val), "+r"(t1Val), "=r"(adder)
    :
    :
    );


    if(test == (1234+1234)){
        printf("Test passed!\n\r");
    }else{
        printf("Test failed!\n\r");
    }

}

int main(void) {
    printf("Hello World!\n\r");
    printf("PR OU Tests\n\r");
    static_region_t static_conf;
    test_add(&static_conf);
}

