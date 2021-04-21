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
} static_region_t;

void write_config(static_region_t* pstatic_config, uint32_t row_start, uint32_t row_end, rca_t rca);



//TODO: Add functions for changing configuration in a safe way - likely not needed as any bad configs are caught in rca.c

//Note:https://beginnersbook.com/2014/01/2d-arrays-in-c-example/