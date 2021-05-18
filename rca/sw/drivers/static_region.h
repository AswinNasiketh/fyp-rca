// For maintaining a model of the static region of PR grid

#include "rca.h"

typedef struct {
    uint32_t cpu_src_regs [NUM_RCAS][NUM_READ_PORTS];
    uint32_t cpu_fb_dst_regs [NUM_RCAS][NUM_WRITE_PORTS];
    uint32_t cpu_nfb_dst_regs [NUM_RCAS][NUM_WRITE_PORTS];

    uint32_t grid_mux_sel [2][NUM_GRID_ROWS][NUM_GRID_COLS];
    uint32_t io_mux_sel [NUM_IO_UNITS];
    uint32_t result_mux_sel_fb [NUM_RCAS][NUM_WRITE_PORTS];
    uint32_t result_mux_sel_nfb [NUM_RCAS][NUM_WRITE_PORTS];

    bool io_unit_is_input [NUM_RCAS][NUM_IO_UNITS];
    
    uint32_t input_constants [NUM_IO_UNITS];

    bool ls_mask_fb [NUM_RCAS][NUM_IO_UNITS];
    bool ls_mask_nfb [NUM_RCAS][NUM_IO_UNITS];
} static_region_t;

typedef enum{
    SLOT_0 = 0,
    SLOT_1 = 1,
    SLOT_2 = 2,
    SLOT_3 = 3,
    SLOT_4 = 4,
    SLOT_5 = 5,
    IO_UNIT = IO_UNIT_GRID_MUX_ADDR,
    LSI = LSI_GRID_MUX_ADDR
} grid_mux_inp_addr_t;

typedef enum{
    INP1 = 0,
    INP2 = 1
} grid_slot_inp_t;

typedef enum{
    RS1 = 0,
    RS2 = 1,
    RS3 = 2,
    RS4 = 3,
    RS5 = 4,
    SLOT_0 = 5,
    SLOT_1 = 6,
    SLOT_2 = 7,
    SLOT_3 = 8,
    SLOT_4 = 9,
    SLOT_5 = 10,
    CONST = 11
} io_mux_inp_addr_t;

typedef enum{
    RD1 = 0,
    RD2 = 1,
    RD3 = 2,
    RD4 = 3,
    RD5 = 4
} rd_t;

void write_config(static_region_t* pstatic_config, uint32_t row_start, uint32_t row_end, rca_t rca);

uint32_t configure_src_regs(static_region_t* pstatic_config, rca_t rca, uint32_t* reg_addrs);
uint32_t configure_nfb_dst_regs(static_region_t* pstatic_config, rca_t rca, uint32_t* reg_addrs);
uint32_t configure_fb_dst_regs(static_region_t* pstatic_config, rca_t rca, uint32_t* reg_addrs);
uint32_t configure_grid_mux(static_region_t* pstatic_config, uint32_t row, uint32_t col, grid_slot_inp_t inp, grid_mux_inp_addr_t inp_addr);
uint32_t configure_fb_result_mux(static_region_t* pstatic_config, rca_t rca, rd_t write_port, uint32_t io_unit_addr);
uint32_t configure_nfb_result_mux(static_region_t* pstatic_config, rca_t rca, rd_t write_port, uint32_t io_unit_addr);
uint32_t configure_input_constant(static_region_t* pstatic_config, uint32_t io_unit_addr, uint32_t new_constant);
uint32_t configure_fb_ls_mask(static_region_t* pstatic_config, rca_t rca, uint32_t io_unit_addr, bool wait_for_ls_request);
uint32_t configure_nfb_ls_mask(static_region_t* pstatic_config, rca_t rca, uint32_t io_unit_addr, bool wait_for_ls_request);

//Note:https://beginnersbook.com/2014/01/2d-arrays-in-c-example/