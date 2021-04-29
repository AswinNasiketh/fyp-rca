import taiga_config::*;
import riscv_types::*;
import taiga_types::*;
import l2_config_and_types::*;
import rca_config::*;

//This file was made to satisfy vivado's requirements that all modports must either be named master or slave
//CPU side ports are master
//RCA side ports are slave

interface rca_writeback_interface;
        logic ack;

        id_t id;
        logic done;
        logic [XLEN-1:0] rd [NUM_WRITE_PORTS];
        modport slave (
            input ack,
            output id, done, rd
        );
        modport master (
            output ack,
            input id, done, rd
        );
endinterface

interface rca_lsu_interface;
    //mimic load_store_inputs_t
    logic [XLEN-1:0] rs1;
    logic [XLEN-1:0] rs2;
    logic [2:0] fn3;
    logic load;
    logic store;
    id_t id;

    //Writeback interface
    logic load_complete;
    logic [XLEN-1:0] load_data;

    //control signals
    logic rca_lsu_lock;
    logic new_request;
    logic lsu_ready;

    modport slave (output rs1, rs2, fn3, load, store, rca_lsu_lock, new_request, id,
    input lsu_ready, load_complete, load_data);

    modport master (input rs1, rs2, fn3, load, store, rca_lsu_lock, new_request, id,
    output lsu_ready, load_complete, load_data);

endinterface

interface rca_decode_issue_interface
    logic possible_issue;
    logic new_request;
    logic new_request_r;
    id_t id;

    logic ready;

    rca_inputs_t rca_inputs;
    rca_dec_inputs_r_t rca_dec_inputs_r;
    rca_cpu_reg_config_t rca_config_regs_op;
    logic rca_config_locked;

    modport slave(input rca_inputs, rca_dec_inputs_r, possible_issue, new_request, new_request_r, id, 
    output rca_config_regs_op, rca_config_locked, ready);

    modport master(output rca_inputs, rca_dec_inputs_r, possible_issue, new_request, new_request_r, id, 
    input rca_config_regs_op, rca_config_locked, ready);

    modport decode(output rca_inputs, rca_dec_inputs_r,
    input rca_config_regs_op, rca_config_locked);
endinterface