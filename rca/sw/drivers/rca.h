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

#define NUM_GRID_ROWS           12
#define NUM_GRID_COLS           6
#define NUM_GRID_MUXES          144 //(NUM_GRID_ROWS * NUM_GRID_COLS * 2)
#define NUM_GRID_MUX_INPUTS     8 //(NUM_GRID_COLS + 1 (IO) + 1 (LSI))
#define LSI_GRID_MUX_ADDR       7 //LSI - Left side input

#define NUM_IO_UNITS            13 //(NUM_GRID_ROWS + 1)
#define NUM_IO_MUX_INPUTS       12 //(NUM_READ_PORTS + NUM_GRID_COLS + 1)
#define IO_UNIT_GRID_MUX_ADDR   6

#define NUM_RCAS                4
#define NUM_READ_PORTS          5
#define NUM_WRITE_PORTS         5

#define UNUSED_WRITE_PORT_ADDR  13 //(NUM_IO_UNITS)

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

char rca_to_opcode_ext(rca_t rca);
reg_port_t int_to_reg_port(uint32_t i);

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

#endif //RCA_H