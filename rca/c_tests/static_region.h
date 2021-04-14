// For maintaining a model of the static region of PR grid

#include "rca.h"

typedef struct {
    uint32_t cpu_src_regs [NUM_READ_PORTS][NUM_RCAS];
    uint32_t cpu_fb_dst_regs [NUM_WRITE_PORTS][NUM_RCAS];
    uint32_t cpu_nfb_dst_regs [NUM_WRITE_PORTS][NUM_RCAS];

    uint32_t grid_mux_sel [NUM_GRID_COLS][NUM_GRID_ROWS];
    uint32_t io_mux_sel [NUM_IO_UNITS];
    uint32_t result_mux_sel_fb [NUM_WRITE_PORTS][NUM_RCAS];
    uint32_t result_mux_sel_nfb [NUM_WRITE_PORTS][NUM_RCAS];

    bool io_unit_is_input [NUM_IO_UNITS][NUM_RCAS];
    
    uint32_t input_constants [NUM_IO_UNITS];
} static_region_t;

void write_config(static_region_t* pstatic_config, uint32_t row_start, uint32_t row_end, rca_t rca);

//TODO: Add functions for changing configuration in a safe way - likely not needed as any bad configs are caught in rca.c