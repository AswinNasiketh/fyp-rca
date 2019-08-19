/*
 * Copyright © 2017-2019 Eric Matthews,  Lesley Shannon
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
import taiga_types::*;

module load_store_unit (
        input logic clk,
        input logic rst,
        input load_store_inputs_t ls_inputs,
        func_unit_ex_interface.unit ls_ex,

        input logic dcache_on,
        input logic clear_reservation,
        tlb_interface.mem tlb,

        input logic gc_issue_flush,

        l1_arbiter_request_interface.master l1_request,
        l1_arbiter_return_interface.master l1_response,
        input sc_complete,
        input sc_success,

        axi_interface.master m_axi,
        avalon_interface.master m_avalon,
        wishbone_interface.master m_wishbone,

        local_memory_interface.master data_bram,

        output logic store_committed,
        output instruction_id_t store_id,

        output logic load_store_FIFO_emptying,
        output exception_packet_t ls_exception,
        output logic ls_exception_valid,

        unit_writeback_interface.unit ls_wb
        );

    localparam NUM_SUB_UNITS = USE_D_SCRATCH_MEM+USE_BUS+USE_DCACHE;
    localparam NUM_SUB_UNITS_W = $clog2(NUM_SUB_UNITS);

    localparam BRAM_ID = 0;
    localparam BUS_ID = USE_D_SCRATCH_MEM;
    localparam DCACHE_ID = USE_D_SCRATCH_MEM+USE_BUS;

    //Should be equal to pipeline depth of longest load/store subunit
    localparam ATTRIBUTES_DEPTH = USE_DCACHE ? 2 : 1;

    data_access_shared_inputs_t shared_inputs;
    ls_sub_unit_interface #(.BASE_ADDR(SCRATCH_ADDR_L), .UPPER_BOUND(SCRATCH_ADDR_H), .BIT_CHECK(SCRATCH_BIT_CHECK)) bram();
    ls_sub_unit_interface #(.BASE_ADDR(BUS_ADDR_L), .UPPER_BOUND(BUS_ADDR_H), .BIT_CHECK(MEMORY_BIT_CHECK)) bus();
    ls_sub_unit_interface #(.BASE_ADDR(MEMORY_ADDR_L), .UPPER_BOUND(MEMORY_ADDR_H), .BIT_CHECK(BUS_BIT_CHECK)) cache();

    logic units_ready;
    logic issue_request;
    logic load_complete;

    logic [31:0] virtual_address;
    logic [3:0] be;

    logic [31:0] unit_muxed_load_data;
    logic [31:0] aligned_load_data;
    logic [31:0] final_load_data;

    logic [31:0] forwarded_data;

    logic [31:0] rd_bank [MAX_INFLIGHT_COUNT-1:0];
    logic [31:0] previous_load;
    logic [31:0] stage1_raw_data;

    logic [31:0] unit_data_array [NUM_SUB_UNITS-1:0];
    logic [NUM_SUB_UNITS-1:0] unit_ready;
    logic [NUM_SUB_UNITS-1:0] unit_data_valid;
    logic [NUM_SUB_UNITS-1:0] last_unit;
    logic [NUM_SUB_UNITS-1:0] current_unit;

    logic unaligned_addr;
    logic [NUM_SUB_UNITS-1:0] sub_unit_address_match;

    logic dcache_forward_data;
    logic [2:0] dcache_stage2_fn3;
    logic unit_stall;

    typedef struct packed{
        logic [2:0] fn3;
        logic [1:0] byte_addr;
        instruction_id_t instruction_id;
    } load_attributes_t;
    load_attributes_t  load_attributes_in, stage2_attr;
    load_store_inputs_t  stage1;

    //FIFOs
    fifo_interface #(.DATA_WIDTH($bits(load_store_inputs_t))) input_fifo();
    fifo_interface #(.DATA_WIDTH($bits(load_attributes_t))) load_attributes();

    ////////////////////////////////////////////////////
    //Implementation

    ////////////////////////////////////////////////////
    //Input FIFO
    taiga_fifo #(.DATA_WIDTH($bits(load_store_inputs_t)), .FIFO_DEPTH(LS_INPUT_BUFFER_DEPTH), .FIFO_TYPE(NON_MUXED_INPUT_FIFO)
        ) ls_input_fifo (.fifo(input_fifo), .*);

    assign input_fifo.data_in = ls_inputs;
    assign input_fifo.push = ls_ex.new_request_dec;
    assign ls_ex.ready = ~input_fifo.full;
    assign input_fifo.pop = issue_request | gc_issue_flush;
    assign load_store_FIFO_emptying = input_fifo.early_empty;
    assign stage1 = input_fifo.data_out;

    ////////////////////////////////////////////////////
    //Unit tracking
    assign current_unit = sub_unit_address_match;
    always_ff @ (posedge clk) begin
        if (rst)
            last_unit <= 0;
        else if (load_attributes.push)
            last_unit <= sub_unit_address_match;
    end

    ////////////////////////////////////////////////////
    //Store commit tracking
    always_ff @ (posedge clk) begin
        store_id <= stage1.instruction_id;
    end

    always_ff @ (posedge clk) begin
        if (rst)
            store_committed <= 0;
        else
            store_committed <= stage1.store & (issue_request | ls_exception_valid);
    end

    ////////////////////////////////////////////////////
    //Primary Control Signals
    assign units_ready = &unit_ready;
    assign load_complete = |unit_data_valid;

    //When switching units, ensure no outstanding loads so that there can be no timing collisions with results
    assign unit_stall = (current_unit != last_unit) && ~load_attributes.empty;
    logic store_bypass_stall;
    assign store_bypass_stall = stage1.store & stage1.load_store_forward & ~load_attributes.empty;
    assign issue_request = input_fifo.valid & units_ready & ~unit_stall & ~unaligned_addr & ~store_bypass_stall;

    ////////////////////////////////////////////////////
    //TLB interface
    assign virtual_address = stage1.virtual_address;// + 32'(signed'(stage1.offset));

    assign tlb.virtual_address = virtual_address;
    assign tlb.new_request = input_fifo.valid;
    assign tlb.execute = 0;
    assign tlb.rnw = stage1.load & ~stage1.store;

    ////////////////////////////////////////////////////
    //Alignment Exception
    always_comb begin
        case(stage1.fn3)
            LS_H_fn3 : unaligned_addr = virtual_address[0];
            LS_W_fn3 : unaligned_addr = |virtual_address[1:0];
            default : unaligned_addr = 0;
        endcase
    end

//    always_ff @ (posedge clk) begin
//        if (rst | gc_issue_flush)
//            ls_exception_valid <= 0;
//        else if (unaligned_addr & input_fifo.valid)
//            ls_exception_valid <= 0;
//    end
//    assign ls_exception.code = stage1.load ? LOAD_ADDR_MISSALIGNED : STORE_AMO_ADDR_MISSALIGNED;
//    assign ls_exception.pc = stage1.pc;
//    assign ls_exception.tval = stage1.virtual_address;
//    assign ls_exception.id = stage1.instruction_id;


    ////////////////////////////////////////////////////
    //Input Processing

    //Byte enable generation
    //Only set on store
    //  SW: all bytes
    //  SH: upper or lower half of bytes
    //  SB: specific byte
    always_comb begin
        foreach (be[i]) begin
            case(stage1.fn3[1:0])
                LS_B_fn3[1:0] : be[i] = (virtual_address[1:0] == i[1:0]);
                LS_H_fn3[1:0] : be[i] = (virtual_address[1] == i[1]);
                default : be[i] = '1; //LS_W_fn3[1:0]
            endcase
            be[i] &= stage1.store;
        end
    end

    ////////////////////////////////////////////////////
    //Unit Inputs
    assign shared_inputs.addr = tlb.physical_address;
    assign shared_inputs.load = stage1.load;
    assign shared_inputs.store = stage1.store;
    assign shared_inputs.be = be;
    assign shared_inputs.fn3 = stage1.fn3;

    logic forward_data;
    assign forward_data = stage1.load_store_forward | dcache_forward_data;
    assign stage1_raw_data =  forward_data ?  previous_load : stage1.rs2;

    //Input: ABCD
    //Assuming aligned requests,
    //Possible byte selections: (A/C/D, B/D, C/D, D)
    logic [1:0] data_in_mux;
    always_comb begin
        data_in_mux = dcache_forward_data ? dcache_stage2_fn3[1:0] : virtual_address[1:0];
        shared_inputs.data_in[7:0] = stage1_raw_data[7:0];
        shared_inputs.data_in[15:8] = (data_in_mux == 2'b01) ? stage1_raw_data[7:0] : stage1_raw_data[15:8];
        shared_inputs.data_in[23:16] = (data_in_mux == 2'b10) ? stage1_raw_data[7:0] : stage1_raw_data[23:16];
        case(data_in_mux)
            2'b10 : shared_inputs.data_in[31:24] = stage1_raw_data[15:8];
            2'b11 : shared_inputs.data_in[31:24] = stage1_raw_data[7:0];
            default : shared_inputs.data_in[31:24] = stage1_raw_data[31:24];
        endcase
    end

    ////////////////////////////////////////////////////
    //Load attributes FIFO
    taiga_fifo #(.DATA_WIDTH($bits(load_attributes_t)), .FIFO_DEPTH(ATTRIBUTES_DEPTH), .FIFO_TYPE(LUTRAM_FIFO)
        ) attributes_fifo (.fifo(load_attributes), .*);
    assign load_attributes_in.fn3 = stage1.fn3;
    assign load_attributes_in.byte_addr = virtual_address[1:0];
    assign load_attributes_in.instruction_id = stage1.instruction_id;

    assign load_attributes.data_in = load_attributes_in;

    assign load_attributes.push = issue_request & stage1.load;
    assign load_attributes.pop = load_complete;

    assign stage2_attr  = load_attributes.data_out;

    ////////////////////////////////////////////////////
    //Unit Instantiation
    generate if (USE_D_SCRATCH_MEM) begin
            assign sub_unit_address_match[BRAM_ID] = bram.address_range_check(tlb.physical_address);
            assign bram.new_request = sub_unit_address_match[BRAM_ID] & issue_request;

            assign unit_ready[BRAM_ID] = bram.ready;
            assign unit_data_valid[BRAM_ID] = bram.data_valid;

            dbram d_bram (.*, .ls_inputs(shared_inputs), .ls(bram), .data_out(unit_data_array[BRAM_ID]));
        end
    endgenerate

    generate if (USE_BUS) begin
            assign sub_unit_address_match[BUS_ID] = bus.address_range_check(tlb.physical_address);
            assign bus.new_request = sub_unit_address_match[BUS_ID] & issue_request;

            assign unit_ready[BUS_ID] = bus.ready;
            assign unit_data_valid[BUS_ID] = bus.data_valid;

            if(BUS_TYPE == AXI_BUS)
                axi_master axi_bus (.*, .ls_inputs(shared_inputs), .size({1'b0,stage1.fn3[1:0]}), .m_axi(m_axi), .ls(bus), .data_out(unit_data_array[BUS_ID])); //Lower two bits of fn3 match AXI specification for request size (byte/halfword/word)
            else if (BUS_TYPE == WISHBONE_BUS)
                wishbone_master wishbone_bus (.*, .ls_inputs(shared_inputs), .m_wishbone(m_wishbone), .ls(bus), .data_out(unit_data_array[BUS_ID]));
            else if (BUS_TYPE == AVALON_BUS)  begin
                avalon_master avalon_bus (.*, .ls_inputs(shared_inputs), .m_avalon(m_avalon), .ls(bus), .data_out(unit_data_array[BUS_ID]));
            end
        end
    endgenerate

    generate if (USE_DCACHE) begin
            assign sub_unit_address_match[DCACHE_ID] = cache.address_range_check(tlb.physical_address);
            assign cache.new_request = sub_unit_address_match[DCACHE_ID] & issue_request;

            assign unit_ready[DCACHE_ID] = cache.ready;
            assign unit_data_valid[DCACHE_ID] = cache.data_valid;

            dcache data_cache (.*, .ls_inputs(shared_inputs), .ls(cache), .amo(stage1.amo), .use_forwarded_data(stage1.load_store_forward), .data_out(unit_data_array[DCACHE_ID]));
        end
    endgenerate

    ////////////////////////////////////////////////////
    //Output Muxing

    //unit mux
    always_comb begin
        unit_muxed_load_data = 0;
        foreach (unit_data_array[i])
            unit_muxed_load_data |= unit_data_array[i];
    end

    //Byte/halfword select: assumes aligned operations
    always_comb begin
        aligned_load_data[31:16] = unit_muxed_load_data[31:16];
        aligned_load_data[15:0] = stage2_attr.byte_addr[1] ? unit_muxed_load_data[31:16] : unit_muxed_load_data[15:0];
        //select halfword first then byte
        aligned_load_data[7:0] = stage2_attr.byte_addr[0] ? aligned_load_data[15:8] : aligned_load_data[7:0];
    end

    //Sign extending
    always_comb begin
        case(stage2_attr.fn3)
            LS_B_fn3 : final_load_data = 32'(signed'(aligned_load_data[7:0]));
            LS_H_fn3 : final_load_data = 32'(signed'(aligned_load_data[15:0]));
            LS_W_fn3 : final_load_data = aligned_load_data;
                //unused 011
            L_BU_fn3 : final_load_data = 32'(unsigned'(aligned_load_data[7:0]));
            L_HU_fn3 : final_load_data = 32'(unsigned'(aligned_load_data[15:0]));
                //unused 110
                //unused 111
            default : final_load_data = aligned_load_data;
        endcase
    end

    ////////////////////////////////////////////////////
    //Output bank
     always_ff @ (posedge clk) begin
         if (load_complete)
             rd_bank[stage2_attr.instruction_id] <= final_load_data;
     end

    always_ff @ (posedge clk) begin
        if (load_complete)
            previous_load <= final_load_data;
    end

    assign ls_wb.rd = rd_bank[ls_wb.writeback_instruction_id];

    logic exception_complete;
    always_ff @ (posedge clk) begin
        exception_complete <= (input_fifo.valid & ls_exception_valid & stage1.load);
    end

    assign ls_wb.done_next_cycle = load_complete | exception_complete;
    assign ls_wb.instruction_id = stage2_attr.instruction_id;

    ////////////////////////////////////////////////////
    //End of Implementation
    ////////////////////////////////////////////////////

    ////////////////////////////////////////////////////
    //Assertions
    always_ff @ (posedge clk) begin
        assert ((issue_request & |sub_unit_address_match) || (!issue_request)) else $error("invalid L/S address");
    end

endmodule
