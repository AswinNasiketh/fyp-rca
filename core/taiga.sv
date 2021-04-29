/*
 * Copyright © 2017, 2018, 2019 Eric Matthews,  Lesley Shannon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Initial code developed under the supervision of Dr. Lesley Shannon,
 * Reconfigurable Computing Lab, Simon Fraser University.
 *
 * Author(s):
 *             Eric Matthews <ematthew@sfu.ca>
 */

import taiga_config::*;
import riscv_types::*;
import taiga_types::*;

module taiga (
        input logic clk,
        input logic rst,

        local_memory_interface.master instruction_bram,
        local_memory_interface.master data_bram,

        axi_interface.master m_axi,
        avalon_interface.master m_avalon,
        wishbone_interface.master m_wishbone,

        output trace_outputs_t tr,

        l2_requester_interface.master l2,

        input logic timer_interrupt,
        input logic interrupt,
        
        //RCA Unit interfaces
        unit_issue_interface.decode rca_issue_if,
        rca_decode_issue_interface.cpu rca_di_if,
        rca_writeback_interface.wb rca_wb_if,
        rca_lsu_interface.lsu rca_lsq_if
        );

    l1_arbiter_request_interface l1_request[L1_CONNECTIONS-1:0]();
    l1_arbiter_return_interface l1_response[L1_CONNECTIONS-1:0]();
    logic sc_complete;
    logic sc_success;

    branch_predictor_interface bp();
    branch_results_t br_results;
    logic branch_flush;
    logic potential_branch_exception;
    exception_packet_t br_exception;
    logic branch_exception_is_jump;

    ras_interface ras();

    issue_packet_t issue;
    logic [31:0] rs_data [REGFILE_READ_PORTS];


    alu_inputs_t alu_inputs;
    load_store_inputs_t ls_inputs;
    branch_inputs_t branch_inputs;
    mul_inputs_t mul_inputs;
    div_inputs_t div_inputs;
    gc_inputs_t gc_inputs;
    // rca_inputs_t rca_inputs;
    // rca_dec_inputs_r_t rca_dec_inputs_r;
    // rca_cpu_reg_config_t rca_config_regs_op;

    unit_issue_interface unit_issue [NUM_UNITS-1:0]();
    logic alu_issued;

    exception_packet_t  ls_exception;
    logic ls_exception_is_store;

    unit_writeback_interface unit_wb  [NUM_WB_UNITS]();

    mmu_interface immu();
    mmu_interface dmmu();

    tlb_interface itlb();
    tlb_interface dtlb();
    logic tlb_on;
    logic [ASIDLEN-1:0] asid;

    //Instruction ID/Metadata
        //ID issuing
    id_t pc_id;
    logic pc_id_available;
    logic pc_id_assigned;
    logic [31:0] if_pc;
        //Fetch stage
    id_t fetch_id;
    logic fetch_complete;
    logic [31:0] fetch_instruction;
    logic fetch_address_valid;
        //Decode stage
    logic decode_advance;
    decode_packet_t decode;
        //Issue stage
    id_t rs_id [REGFILE_READ_PORTS];
    logic rs_inuse [REGFILE_READ_PORTS];
    logic rs_id_inuse [REGFILE_READ_PORTS];
        //Branch predictor
    branch_metadata_t branch_metadata_if;
    branch_metadata_t branch_metadata_ex;
        //ID freeing
    logic store_complete;
    id_t store_id;
    logic branch_complete;
    id_t branch_id;
    logic system_op_or_exception_complete;
    logic exception_with_rd_complete;
    id_t system_op_or_exception_id;
    logic instruction_retired;
    logic [$clog2(MAX_COMPLETE_COUNT)-1:0] retire_inc;
        //Exception
    id_t exception_id;
    logic [31:0] exception_pc;

    //Global Control
    logic gc_init_clear;
    logic gc_fetch_hold;
    logic gc_issue_hold;
    logic gc_issue_flush;
    logic gc_fetch_flush;
    logic gc_fetch_pc_override;
    logic gc_supress_writeback;
    logic [31:0] gc_fetch_pc;

    logic[31:0] csr_rd;
    id_t csr_id;
    logic csr_done;
    logic ls_is_idle;

    //Decode Unit and Fetch Unit
    logic illegal_instruction;
    logic instruction_issued;
    logic gc_flush_required;

    //LS
    writeback_store_interface wb_store();

    //WB
    id_t ids_retiring [COMMIT_PORTS];
    logic retired [COMMIT_PORTS];
    logic [4:0] retired_rd_addr [COMMIT_PORTS];
    id_t id_for_rd [COMMIT_PORTS];
    //RCA WB
    id_t rca_id_retiring;
    logic rca_retired;

    logic [4:0] rca_retired_rd_addrs [NUM_WRITE_PORTS];
    id_t rca_id_for_rds [NUM_WRITE_PORTS];

    logic rca_config_locked;

    //Trace Interface Signals
    logic tr_operand_stall;
    logic tr_unit_stall;
    logic tr_no_id_stall;
    logic tr_no_instruction_stall;
    logic tr_other_stall;
    logic tr_branch_operand_stall;
    logic tr_alu_operand_stall;
    logic tr_ls_operand_stall;
    logic tr_div_operand_stall;

    logic tr_alu_op;
    logic tr_branch_or_jump_op;
    logic tr_load_op;
    logic tr_store_op;
    logic tr_mul_op;
    logic tr_div_op;
    logic tr_misc_op;

    logic tr_instruction_issued_dec;
    logic [31:0] tr_instruction_pc_dec;
    logic [31:0] tr_instruction_data_dec;

    logic tr_branch_correct;
    logic tr_branch_misspredict;
    logic tr_return_correct;
    logic tr_return_misspredict;

    logic tr_rs1_forwarding_needed;
    logic tr_rs2_forwarding_needed;
    logic tr_rs1_and_rs2_forwarding_needed;

    unit_id_t tr_num_instructions_completing;
    id_t tr_num_instructions_in_flight;
    id_t tr_num_of_instructions_pending_writeback;
    ////////////////////////////////////////////////////
    //Implementation


    ////////////////////////////////////////////////////
    // Memory Interface
    generate if (ENABLE_S_MODE || USE_ICACHE || USE_DCACHE)
            l1_arbiter arb(.*);
    endgenerate

    ////////////////////////////////////////////////////
    // ID support
    instruction_metadata_and_id_management id_block (.*);

    ////////////////////////////////////////////////////
    // Fetch
    fetch fetch_block (.*, .icache_on('1), .tlb(itlb), .l1_request(l1_request[L1_ICACHE_ID]), .l1_response(l1_response[L1_ICACHE_ID]), .exception(1'b0));
    branch_predictor bp_block (.*);
    ras ras_block(.*);
    generate if (ENABLE_S_MODE) begin
            tlb_lut_ram #(ITLB_WAYS, ITLB_DEPTH) i_tlb (.*, .tlb(itlb), .mmu(immu));
            mmu i_mmu (.*,  .mmu(immu) , .l1_request(l1_request[L1_IMMU_ID]), .l1_response(l1_response[L1_IMMU_ID]), .mmu_exception());
        end
        else begin
            assign itlb.complete = 1;
            assign itlb.physical_address = itlb.virtual_address;
        end
    endgenerate

    ////////////////////////////////////////////////////
    //Decode/Issue
    decode_and_issue decode_and_issue_block (.*, .rca(rca_di_if));

    ////////////////////////////////////////////////////
    //Register File and Writeback
    register_file_and_writeback register_file_and_writeback_block (.*, rca_wb(rca_wb_if));

    ////////////////////////////////////////////////////
    //Execution Units
    branch_unit branch_unit_block (.*, .issue(unit_issue[BRANCH_UNIT_ID]));
    alu_unit alu_unit_block (.*, .issue(unit_issue[ALU_UNIT_WB_ID]), .wb(unit_wb[ALU_UNIT_WB_ID]));
    load_store_unit load_store_unit_block (.*, .dcache_on(1'b1), .clear_reservation(1'b0), .tlb(dtlb), .issue(unit_issue[LS_UNIT_WB_ID]), .wb(unit_wb[LS_UNIT_WB_ID]), .l1_request(l1_request[L1_DCACHE_ID]), .l1_response(l1_response[L1_DCACHE_ID]), .rca_lsq(rca_lsq_if));
    generate if (ENABLE_S_MODE) begin
            tlb_lut_ram #(DTLB_WAYS, DTLB_DEPTH) d_tlb (.*, .tlb(dtlb), .mmu(dmmu));
            mmu d_mmu (.*, .mmu(dmmu), .l1_request(l1_request[L1_DMMU_ID]), .l1_response(l1_response[L1_DMMU_ID]), .mmu_exception());
        end
        else begin
            assign dtlb.complete = 1;
            assign dtlb.physical_address = dtlb.virtual_address;
        end
    endgenerate
    gc_unit gc_unit_block (.*, .issue(unit_issue[GC_UNIT_ID]));

    generate if (USE_MUL)
            mul_unit mul_unit_block (.*, .issue(unit_issue[MUL_UNIT_WB_ID]), .wb(unit_wb[MUL_UNIT_WB_ID]));
    endgenerate
    generate if (USE_DIV)
            div_unit div_unit_block (.*, .issue(unit_issue[DIV_UNIT_WB_ID]), .wb(unit_wb[DIV_UNIT_WB_ID]));
    endgenerate

    // generate if (USE_RCA)
    //     rca_unit rca (.*, .issue(unit_issue[RCA_UNIT_WB_ID]), .lsu(rca_ls));
    // endgenerate

    assign rca_issue_if.possible_issue = unit_issue[RCA_UNIT_WB_ID].possible_issue;
    assign rca_issue_if.new_request = unit_issue[RCA_UNIT_WB_ID].new_request;
    assign rca_issue_if.new_request_r = unit_issue[RCA_UNIT_WB_ID].new_request_r;
    assign rca_issue_if.id = unit_issue[RCA_UNIT_WB_ID].id;
    assign unit_issue[RCA_UNIT_WB_ID].ready = rca_issue_if.ready;


    ////////////////////////////////////////////////////
    //End of Implementation
    ////////////////////////////////////////////////////

    ////////////////////////////////////////////////////
    //Assertions
    //Ensure that reset is held for at least 32 cycles to clear shift regs
    // always_ff @ (posedge clk) begin
    //     assert property(@(posedge clk) $rose (rst) |=> rst[*32]) else $error("Reset not held for long enough!");
    // end

    ////////////////////////////////////////////////////
    //Assertions

    ////////////////////////////////////////////////////
    //Trace Interface
    generate if (ENABLE_TRACE_INTERFACE) begin
        always_ff @(posedge clk) begin
            tr.events.operand_stall <= tr_operand_stall;
            tr.events.unit_stall <= tr_unit_stall;
            tr.events.no_id_stall <= tr_no_id_stall;
            tr.events.no_instruction_stall <= tr_no_instruction_stall;
            tr.events.other_stall <= tr_other_stall;
            tr.events.instruction_issued_dec <= tr_instruction_issued_dec;
            tr.events.branch_operand_stall <= tr_branch_operand_stall;
            tr.events.alu_operand_stall <= tr_alu_operand_stall;
            tr.events.ls_operand_stall <= tr_ls_operand_stall;
            tr.events.div_operand_stall <= tr_div_operand_stall;
            tr.events.alu_op <= tr_alu_op;
            tr.events.branch_or_jump_op <= tr_branch_or_jump_op;
            tr.events.load_op <= tr_load_op;
            tr.events.store_op <= tr_store_op;
            tr.events.mul_op <= tr_mul_op;
            tr.events.div_op <= tr_div_op;
            tr.events.misc_op <= tr_misc_op;
            tr.events.branch_correct <= tr_branch_correct;
            tr.events.branch_misspredict <= tr_branch_misspredict;
            tr.events.return_correct <= tr_return_correct;
            tr.events.return_misspredict <= tr_return_misspredict;
            tr.events.rs1_forwarding_needed <= tr_rs1_forwarding_needed;
            tr.events.rs2_forwarding_needed <= tr_rs2_forwarding_needed;
            tr.events.rs1_and_rs2_forwarding_needed <= tr_rs1_and_rs2_forwarding_needed;
            tr.events.num_instructions_completing <= tr_num_instructions_completing;
            tr.events.num_instructions_in_flight <= tr_num_instructions_in_flight;
            tr.events.num_of_instructions_pending_writeback <= tr_num_of_instructions_pending_writeback;
            tr.instruction_pc_dec <= tr_instruction_pc_dec;
            tr.instruction_data_dec <= tr_instruction_data_dec;
        end
    end
    endgenerate

endmodule
