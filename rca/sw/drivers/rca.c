// Driver for Reconfigurable Custom Accelerators

//Static Logic Configuration

#include "rca.h"

uint32_t grid_coord_to_slot(uint32_t row, uint32_t col){
    return (NUM_GRID_COLS*row) + col;
}

static void bad_config_trap(){
    printf("bad config");
    // while(1);
}

reg_port_t int_to_reg_port(uint32_t i){
    if (i > NUM_REG_PORTS || i < 1){
        bad_config_trap();
    } 

    switch (i)
    {
    case 1:
        return RX1;
    case 2:
        return RX2;
    case 3:
        return RX3;
    case 4:
        return RX4;
    case 5:
        return RX5;
    }
}

char rca_to_opcode_ext(rca_t rca){
    switch (rca){
        case RCA_A:
            return 'a';
        case RCA_B:
            return 'b';
        case RCA_C:
            return 'c';
        case RCA_D:
            return 'd';
    }
}

static inline void rca_a_config_cpu_reg(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;
    asm volatile("rcaccr.a %0, %1, %2;"
        : "=r"(dummy) //dummy output - instr does not write to rd
        : "r"(rs1_val), "r"(rs2_val) //inputs
        : // noclobbered registers
    );
}

static inline void rca_b_config_cpu_reg(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;
    asm volatile("rcaccr.b %0, %1, %2;"
        : "=r"(dummy) //dummy output - instr does not write to rd
        : "r"(rs1_val), "r"(rs2_val) //inputs
        : // noclobbered registers
    );
}

static inline void rca_c_config_cpu_reg(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;
    asm volatile("rcaccr.c %0, %1, %2;"
        : "=r"(dummy) //dummy output - instr does not write to rd
        : "r"(rs1_val), "r"(rs2_val) //inputs
        : // noclobbered registers
    );
}

static inline void rca_d_config_cpu_reg(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;
    asm volatile("rcaccr.d %0, %1, %2;"
        : "=r"(dummy) //dummy output - instr does not write to rd
        : "r"(rs1_val), "r"(rs2_val) //inputs
        : // noclobbered registers
    );
}

void rca_config_cpu_reg(rca_t rca, reg_port_t reg_port, reg_port_type_t reg_port_type, uint32_t reg_addr){

    if(reg_addr > NUM_CPU_REGS-1){
        bad_config_trap();
    }


    uint32_t rs1_val = (reg_port & REG_PORT_MASK) | 
        ((reg_port_type << 3) & REG_PORT_TYPE_MASK);
    
    uint32_t rs2_val = reg_addr;
    // printf("RS2: %u", rs2_val);

    switch (rca) {
        case RCA_A:
            rca_a_config_cpu_reg(rs1_val, rs2_val);
            break;
        case RCA_B:
            rca_b_config_cpu_reg(rs1_val, rs2_val);
            break;
        case RCA_C:
            rca_c_config_cpu_reg(rs1_val, rs2_val);
            break;
        case RCA_D:
            rca_d_config_cpu_reg(rs1_val, rs2_val);
            break;
    }

}

void rca_config_grid_mux(uint32_t mux_addr, uint32_t mux_sel){

    if(mux_addr > NUM_GRID_MUXES-1){
        bad_config_trap();
    }

    if(mux_sel > NUM_GRID_MUX_INPUTS-1){
        bad_config_trap();
    }

    uint32_t rs1_val = mux_addr;
    uint32_t rs2_val = mux_sel;
    uint32_t dummy;

    asm volatile("rcacgm %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

void rca_config_io_mux(uint32_t io_unit_addr, uint32_t io_mux_sel){
    if(io_unit_addr > NUM_IO_UNITS-1){
        bad_config_trap();
    }

    if(io_mux_sel > NUM_IO_MUX_INPUTS-1){
        bad_config_trap();
    }

    uint32_t rs1_val = io_unit_addr;
    uint32_t rs2_val = io_mux_sel;
    uint32_t dummy;

    asm volatile("rcacim %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_a_config_result_mux(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacrm.a %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_b_config_result_mux(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacrm.b %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_c_config_result_mux(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacrm.c %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_d_config_result_mux(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacrm.d %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

void rca_config_result_mux(rca_t rca, reg_port_t write_port, uint32_t io_unit_addr, bool fb_addr){
    if(io_unit_addr > NUM_IO_UNITS){
        bad_config_trap();
    }

    uint32_t rs1_val = write_port & REG_PORT_MASK;

    if (fb_addr){
        rs1_val |= 0x1 << 3;
    }

    uint32_t rs2_val = io_unit_addr;

    switch (rca) {
        case RCA_A:
            rca_a_config_result_mux(rs1_val, rs2_val);
            break;
        case RCA_B:
            rca_b_config_result_mux(rs1_val, rs2_val);
            break;
        case RCA_C:
            rca_c_config_result_mux(rs1_val, rs2_val);
            break;
        case RCA_D:
            rca_d_config_result_mux(rs1_val, rs2_val);
            break;
    }

}

static void inline rca_a_config_inp_io_unit_map(uint32_t rs1_val){
    uint32_t dummy_result;
    uint32_t dummy_input = 0;

    asm volatile("rcacinp.a %0, %1, %2;"
        :"=r"(dummy_result)
        :"r"(rs1_val), "r"(dummy_result)
        :
    );
}

static void inline rca_b_config_inp_io_unit_map(uint32_t rs1_val){
    uint32_t dummy_result;
    uint32_t dummy_input = 0;

    asm volatile("rcacinp.b %0, %1, %2;"
        :"=r"(dummy_result)
        :"r"(rs1_val), "r"(dummy_result)
        :
    );
}

static void inline rca_c_config_inp_io_unit_map(uint32_t rs1_val){
    uint32_t dummy_result;
    uint32_t dummy_input = 0;

    asm volatile("rcacinp.c %0, %1, %2;"
        :"=r"(dummy_result)
        :"r"(rs1_val), "r"(dummy_result)
        :
    );
}

static void inline rca_d_config_inp_io_unit_map(uint32_t rs1_val){
    uint32_t dummy_result;
    uint32_t dummy_input = 0;

    asm volatile("rcacinp.d %0, %1, %2;"
        :"=r"(dummy_result)
        :"r"(rs1_val), "r"(dummy_result)
        :
    );
}

//boolean array must be of size NUM_IO_UNITS
void rca_config_inp_io_unit_map(rca_t rca, bool* io_unit_is_input){
    uint32_t set_bit = 0x1;
    uint32_t io_inp_map = 0;

    for(int i = 0; i < NUM_IO_UNITS; i++){
        if(io_unit_is_input[i]){
            io_inp_map |= set_bit;
        }
        set_bit = set_bit << 1;
    }

    switch (rca)
    {
    case RCA_A:
        rca_a_config_inp_io_unit_map(io_inp_map);
        break;
    case RCA_B:
        rca_b_config_inp_io_unit_map(io_inp_map);
        break;
    case RCA_C:
        rca_c_config_inp_io_unit_map(io_inp_map);
        break;
    case RCA_D:
        rca_d_config_inp_io_unit_map(io_inp_map);
    }
}

void rca_config_input_constant(uint32_t io_unit_addr, uint32_t c){
    if(io_unit_addr > NUM_IO_UNITS-1){
        bad_config_trap();
    }

    uint32_t dummy;

    asm volatile("rcacic %0, %1, %2;"
        :"=r"(dummy)
        :"r"(io_unit_addr), "r"(c)
        :
    );
}

static void inline rca_a_config_io_ls_mask(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacilm.a %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_b_config_io_ls_mask(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacilm.b %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_c_config_io_ls_mask(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacilm.c %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

static void inline rca_d_config_io_ls_mask(uint32_t rs1_val, uint32_t rs2_val){
    uint32_t dummy;

    asm volatile("rcacilm.d %0, %1, %2;"
        :"=r"(dummy)
        :"r"(rs1_val), "r"(rs2_val)
        :
    );
}

//boolean array must be of size NUM_IO_UNITS
void rca_config_io_ls_mask(rca_t rca, bool* wait_for_ls_request, bool fb){
    uint32_t set_bit = 0x1;
    uint32_t io_ls_mask = 0;
    uint32_t rs1_val = (fb == true) ? 0x1 : 0x0;

    for(int i = 0; i < NUM_IO_UNITS; i++){
        if(wait_for_ls_request[i]){
            io_ls_mask |= set_bit;
        }
        set_bit = set_bit << 1;
    }

    switch (rca)
    {
    case RCA_A:
        rca_a_config_io_ls_mask(rs1_val, io_ls_mask);
        break;
    case RCA_B:
        rca_b_config_io_ls_mask(rs1_val, io_ls_mask);
        break;
    case RCA_C:
        rca_c_config_io_ls_mask(rs1_val, io_ls_mask);
        break;
    case RCA_D:
        rca_d_config_io_ls_mask(rs1_val, io_ls_mask);
    }
}

void rca_a_use(){
    uint32_t dummy;

    asm volatile("rcaufb.a %0, %0, %0; rcaunfb.a %0, %0, %0"
        :"+r"(dummy)
        :
        :
    );
}

void rca_b_use(){
    uint32_t dummy;
    // uint32_t rs1_val = (1 & REG_PORT_MASK) | 
    //     ((SRC_PORT << 3) & REG_PORT_TYPE_MASK);
    // uint32_t rs2_val = 5;

    asm volatile("rcaufb.b %0, %0, %0; rcaunfb.b %0, %0, %0;"
        :"+r"(dummy)
        :
        :
    );    

    //test config lock
    // asm volatile("rcaccr.b %0, %1, %2;"
    //     : "=r"(dummy) //dummy output - instr does not write to rd
    //     : "r"(rs1_val), "r"(rs2_val) //inputs
    //     : // noclobbered registers
    // );
}

void rca_c_use(){
    uint32_t dummy;

    asm volatile("rcaufb.c %0, %0, %0; rcaunfb.c %0, %0, %0"
        :"+r"(dummy)
        :
        :
    );
}

void rca_d_use(){
    uint32_t dummy;

    asm volatile("rcaufb.d %0, %0, %0; rcaunfb.d %0, %0, %0"
        :"+r"(dummy)
        :
        :
    );
}

void send_pr_request(ou_t ou, uint32_t grid_slot){
    uint32_t dummy;
    uint32_t rs2 = ou;
    asm volatile("rcapprq %0, %1, %2;"
        : "=r"(dummy)
        : "r"(grid_slot), "r"(rs2)
        :
    );
}