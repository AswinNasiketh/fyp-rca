#include "board_support.h"
#include <stdio.h>

#include "static_region.h"
#include "rca.h"


static_region_t gen_test_static_config(){
    static_region_t static_region = {0};

    static_region.cpu_src_regs[1][0] = 6;
    static_region.cpu_src_regs[1][1] = 7;

    static_region.cpu_src_regs[1][2] = 0;
    static_region.cpu_src_regs[1][3] = 0;
    static_region.cpu_src_regs[1][4] = 0;

    static_region.cpu_fb_dst_regs[1][0] = 6;

    static_region.cpu_fb_dst_regs[1][1] = 0;
    static_region.cpu_fb_dst_regs[1][2] = 0;
    static_region.cpu_fb_dst_regs[1][3] = 0;
    static_region.cpu_fb_dst_regs[1][4] = 0;

    static_region.cpu_nfb_dst_regs[1][0] = 7;

    static_region.cpu_nfb_dst_regs[1][1] = 0;
    static_region.cpu_nfb_dst_regs[1][2] = 0;
    static_region.cpu_nfb_dst_regs[1][3] = 0;
    static_region.cpu_nfb_dst_regs[1][4] = 0;

    static_region.grid_mux_sel[0][0][2] = IO_UNIT_GRID_MUX_ADDR;
    static_region.grid_mux_sel[0][1][0] = 2;
    static_region.grid_mux_sel[0][1][1] = LSI_GRID_MUX_ADDR;
    static_region.grid_mux_sel[0][1][2] = IO_UNIT_GRID_MUX_ADDR;

    static_region.grid_mux_sel[0][2][1] = 2;
    static_region.grid_mux_sel[0][2][2] = 1;

    static_region.io_mux_sel[0] = 0;
    static_region.io_mux_sel[1] = 1;

    static_region.io_mux_sel[2] = 6;
    static_region.io_mux_sel[3] = 6;

    static_region.result_mux_sel_nfb[1][0] = 2;
    static_region.result_mux_sel_nfb[1][1] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_nfb[1][2] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_nfb[1][3] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_nfb[1][4] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_fb[1][0] = 3;
    static_region.result_mux_sel_fb[1][1] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_fb[1][2] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_fb[1][3] = UNUSED_WRITE_PORT_ADDR;
    static_region.result_mux_sel_fb[1][4] = UNUSED_WRITE_PORT_ADDR;

    static_region.io_unit_is_input[1][0] = true;
    static_region.io_unit_is_input[1][1] = true;

    static_region.input_constants[0] = 0x00000056;

    return static_region;
}


int main(void) {
    //Platform Initialization
    platform_init ();

    //Records cycle and instruction counts
    start_profiling ();
    printf("Hello World!\n\r");

    static_region_t s_region = gen_test_static_config();
    write_config(&s_region, 0, 2, RCA_B);

    rca_b_use();
    rca_b_use();


    // Test to see if value was propagated correctly
    uint32_t r6_val;
    asm volatile("add %0, x6, zero"
        :"=r"(r6_val)
        :
        :
    );

    printf("R6 Val: %u", r6_val);
    //Records cycle and instruction counts
    //Prints summary stats for the application
    end_profiling ();

    return 0;
}