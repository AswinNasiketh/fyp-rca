#ifndef RCA_H
#define RCA_H

#include "board_support.h"
#include <stdio.h>

#include "stdbool.h"
#include "stdint.h"
#include "string.h"

#define REG_PORT_MASK           0x00000007
#define REG_PORT_TYPE_MASK      0x00000018
#define NUM_CPU_REGS            32
#define NUM_REG_PORTS           5

#define NUM_GRID_ROWS           5
#define NUM_GRID_COLS           6
#define NUM_GRID_MUXES          60 //(NUM_GRID_ROWS * NUM_GRID_COLS * 2)
#define NUM_GRID_MUX_INPUTS     8 //(NUM_GRID_COLS + 1 (IO) + 1 (LSI))
#define LSI_GRID_MUX_ADDR       7 //LSI - Left side input

#define NUM_IO_UNITS            6 //(NUM_GRID_ROWS + 1)
#define NUM_IO_MUX_INPUTS       12 //(NUM_READ_PORTS + NUM_GRID_COLS + 1)
#define IO_UNIT_GRID_MUX_ADDR   6

#define NUM_RCAS                4
#define NUM_READ_PORTS          5
#define NUM_WRITE_PORTS         5

#define UNUSED_WRITE_PORT_ADDR  6 //(NUM_IO_UNITS)

#define NUM_OUS 22

#define NUM_PROFILER_ENTRIES    4

#define PT_SLOT_INPUT           1

#define LOG2_NUM_RCAS           2
#define RCA_NUM_MASK            0x00000003

typedef enum {
    RX1 = 0,
    RX2 = 1,
    RX3 = 2,
    RX4 = 3,
    RX5 = 4
} reg_port_t;

typedef enum {
    SRC_PORT = 0b10,
    NFB_DEST_PORT = 0b01,
    FB_DEST_PORT = 0b11
} reg_port_type_t;

typedef enum {
    RCA_A = 0,
    RCA_B = 1,
    RCA_C = 2,
    RCA_D = 3
} rca_t;

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

typedef enum{
    BRANCH_ADDR = 0,
    ENTRY_VALID = 1,
    TAKEN_COUNT = 2
}profiler_field_t;

typedef struct{
    uint32_t branch_addr;
    bool entry_valid;
    uint32_t taken_count; 
}profiler_entry_t;

typedef enum{
    SBB_ADDR = 0,
    LOOP_START_ADDR = 1,
    ACC_ENABLE = 2
}att_field_t;

char rca_to_opcode_ext(rca_t rca);
reg_port_t int_to_reg_port(uint32_t i);
uint32_t grid_coord_to_slot(uint32_t row, uint32_t col);
uint32_t grid_slot_to_coord(uint32_t slot, uint32_t* row, uint32_t* col);

//Static Region Configuration
void rca_config_cpu_reg(rca_t rca, reg_port_t reg_port, reg_port_type_t reg_port_type, uint32_t reg_addr);
void rca_config_grid_mux(uint32_t mux_addr, uint32_t mux_sel);
void rca_config_io_mux(uint32_t io_unit_addr, uint32_t io_mux_sel);
void rca_config_result_mux(rca_t rca, reg_port_t write_port, uint32_t io_unit_addr, bool fb_addr);
void rca_config_inp_io_unit_map(rca_t rca, bool* io_unit_is_input);
void rca_config_input_constant(uint32_t io_unit_addr, uint32_t c);
void rca_config_io_ls_mask(rca_t rca, bool* wait_for_ls_request, bool fb);

//RCA Use wrappers with FB instr followed by NFB instr
void rca_a_use();
void rca_b_use();
void rca_c_use();
void rca_d_use();

//Function wrapper for PR request function
void send_pr_request(ou_t ou, uint32_t grid_slot);

//Function wrapper for profiler operations
void toggle_profiler_lock();
profiler_entry_t get_profiler_entry(uint32_t entry_num);

//Function wrapper for ATT configuration
void set_att_field(rca_t rca, att_field_t field, uint32_t field_value);

#endif //RCA_H