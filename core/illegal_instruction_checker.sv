/*
 * Copyright © 2020 Eric Matthews,  Lesley Shannon
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

module  illegal_instruction_checker
    import taiga_config::*;
    (
        input logic [31:0] instruction,
        output logic illegal_instruction
    );

    ////////////////////////////////////////////////////
    //Instruction Patterns for Illegal Instruction Checking

    //Base ISA
    localparam [31:0] BEQ = 32'b?????????????????000?????1100011;
    localparam [31:0] BNE = 32'b?????????????????001?????1100011;
    localparam [31:0] BLT = 32'b?????????????????100?????1100011;
    localparam [31:0] BGE = 32'b?????????????????101?????1100011;
    localparam [31:0] BLTU = 32'b?????????????????110?????1100011;
    localparam [31:0] BGEU = 32'b?????????????????111?????1100011;
    localparam [31:0] JALR = 32'b?????????????????000?????1100111;
    localparam [31:0] JAL = 32'b?????????????????????????1101111;
    localparam [31:0] LUI = 32'b?????????????????????????0110111;
    localparam [31:0] AUIPC = 32'b?????????????????????????0010111;
    localparam [31:0] ADDI = 32'b?????????????????000?????0010011;
    localparam [31:0] SLLI = 32'b000000???????????001?????0010011;
    localparam [31:0] SLTI = 32'b?????????????????010?????0010011;
    localparam [31:0] SLTIU = 32'b?????????????????011?????0010011;
    localparam [31:0] XORI = 32'b?????????????????100?????0010011;
    localparam [31:0] SRLI = 32'b000000???????????101?????0010011;
    localparam [31:0] SRAI = 32'b010000???????????101?????0010011;
    localparam [31:0] ORI = 32'b?????????????????110?????0010011;
    localparam [31:0] ANDI = 32'b?????????????????111?????0010011;
    localparam [31:0] ADD = 32'b0000000??????????000?????0110011;
    localparam [31:0] SUB = 32'b0100000??????????000?????0110011;
    localparam [31:0] SLL = 32'b0000000??????????001?????0110011;
    localparam [31:0] SLT = 32'b0000000??????????010?????0110011;
    localparam [31:0] SLTU = 32'b0000000??????????011?????0110011;
    localparam [31:0] XOR = 32'b0000000??????????100?????0110011;
    localparam [31:0] SRL = 32'b0000000??????????101?????0110011;
    localparam [31:0] SRA = 32'b0100000??????????101?????0110011;
    localparam [31:0] OR = 32'b0000000??????????110?????0110011;
    localparam [31:0] AND = 32'b0000000??????????111?????0110011;
    localparam [31:0] LB = 32'b?????????????????000?????0000011;
    localparam [31:0] LH = 32'b?????????????????001?????0000011;
    localparam [31:0] LW = 32'b?????????????????010?????0000011;
    localparam [31:0] LBU = 32'b?????????????????100?????0000011;
    localparam [31:0] LHU = 32'b?????????????????101?????0000011;
    localparam [31:0] SB = 32'b?????????????????000?????0100011;
    localparam [31:0] SH = 32'b?????????????????001?????0100011;
    localparam [31:0] SW = 32'b?????????????????010?????0100011;
    localparam [31:0] FENCE = 32'b?????????????????000?????0001111;
    localparam [31:0] FENCE_I = 32'b?????????????????001?????0001111;
    localparam [31:0] ECALL = 32'b00000000000000000000000001110011;
    localparam [31:0] EBREAK = 32'b00000000000100000000000001110011;

    localparam [31:0] CSRRW = 32'b?????????????????001?????1110011;
    localparam [31:0] CSRRS = 32'b?????????????????010?????1110011;
    localparam [31:0] CSRRC = 32'b?????????????????011?????1110011;
    localparam [31:0] CSRRWI = 32'b?????????????????101?????1110011;
    localparam [31:0] CSRRSI = 32'b?????????????????110?????1110011;
    localparam [31:0] CSRRCI = 32'b?????????????????111?????1110011;

    //Mul
    localparam [31:0] MUL = 32'b0000001??????????000?????0110011;
    localparam [31:0] MULH = 32'b0000001??????????001?????0110011;
    localparam [31:0] MULHSU = 32'b0000001??????????010?????0110011;
    localparam [31:0] MULHU = 32'b0000001??????????011?????0110011;
    //Div
    localparam [31:0] DIV = 32'b0000001??????????100?????0110011;
    localparam [31:0] DIVU = 32'b0000001??????????101?????0110011;
    localparam [31:0] REM = 32'b0000001??????????110?????0110011;
    localparam [31:0] REMU = 32'b0000001??????????111?????0110011;

    //AMO
    localparam [31:0] AMO_ADD = 32'b00000????????????010?????0101111;
    localparam [31:0] AMO_XOR = 32'b00100????????????010?????0101111;
    localparam [31:0] AMO_OR = 32'b01000????????????010?????0101111;
    localparam [31:0] AMO_AND = 32'b01100????????????010?????0101111;
    localparam [31:0] AMO_MIN = 32'b10000????????????010?????0101111;
    localparam [31:0] AMO_MAX = 32'b10100????????????010?????0101111;
    localparam [31:0] AMO_MINU = 32'b11000????????????010?????0101111;
    localparam [31:0] AMO_MAXU = 32'b11100????????????010?????0101111;
    localparam [31:0] AMO_SWAP = 32'b00001????????????010?????0101111;
    localparam [31:0] LR = 32'b00010??00000?????010?????0101111;
    localparam [31:0] SC = 32'b00011????????????010?????0101111;

    //Machine/Supervisor
    localparam [31:0] SRET = 32'b00010000001000000000000001110011;
    localparam [31:0] MRET = 32'b00110000001000000000000001110011;
    localparam [31:0] SFENCE_VMA = 32'b0001001??????????000000001110011;
    localparam [31:0] WFI = 32'b00010000010100000000000001110011;

    //RCA
    localparam [31:0] RCA_USE_FB = 32'b0000000??????????0???????0101011;
    localparam [31:0] RCA_USE_NFB = 32'b0000001??????????0???????0101011;
    localparam [31:0] RCA_CPU_REG_CONFIG = 32'b0000010??????????0???????0101011;
    localparam [31:0] RCA_GRID_MUX_CONFIG = 32'b0000011??????????0???????0101011;
    localparam [31:0] RCA_IO_MUX_CONFIG = 32'b0000100??????????0???????0101011;
    localparam [31:0] RCA_RESULT_MUX_CONFIG = 32'b0000101??????????0???????0101011;
    localparam [31:0] RCA_IO_INP_MAP_CONFIG = 32'b0000110??????????0???????0101011;
    localparam [31:0] RCA_INP_CONSTANT_CONFIG = 32'b0000111??????????0???????0101011;
    localparam [31:0] RCA_LS_MASK_CONFIG = 32'b0001000??????????0???????0101011;
    localparam [31:0] PR_QUEUE_PUSH = 32'b0001001??????????0???????0101011;
    localparam [31:0] TOGGLE_PROFILER_LOCK  = 32'b0001010??????????0???????0101011;
    localparam [31:0] GET_PROFILER_DATA = 32'b0001011??????????0???????0101011;
    localparam [31:0] ATT_CONFIGURE = 32'b0001100??????????0???????0101011;


    logic base_legal;
    logic mul_legal;
    logic div_legal;
    logic amo_legal;
    logic machine_legal;
    logic supervisor_legal;
    logic rca_legal;
    ////////////////////////////////////////////////////
    //Implementation

    assign base_legal = instruction inside {
        BEQ, BNE, BLT, BGE, BLTU, BGEU, JALR, JAL, LUI, AUIPC,
        ADDI, SLLI, SLTI, SLTIU, XORI, SRLI, SRAI, ORI, ANDI,
        ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
        LB, LH, LW, LBU, LHU, SB, SH, SW,
        FENCE, FENCE_I,
        CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI
    };

    assign mul_legal = instruction inside {
        MUL, MULH, MULHSU, MULHU
    };

    assign div_legal = instruction inside {
        DIV, DIVU, REM, REMU
    };

    assign amo_legal = instruction inside {
        AMO_ADD, AMO_XOR, AMO_OR, AMO_AND, AMO_MIN, AMO_MAX, AMO_MINU, AMO_MAXU, AMO_SWAP,
        LR, SC
    };

    assign machine_legal = instruction inside {
        MRET, ECALL, EBREAK
    };

    assign supervisor_legal = instruction inside {
        SRET, SFENCE_VMA, WFI
    };

    assign rca_legal = instruction inside {
        RCA_USE_FB, RCA_USE_NFB, RCA_CPU_REG_CONFIG, RCA_GRID_MUX_CONFIG, RCA_IO_MUX_CONFIG, RCA_RESULT_MUX_CONFIG, RCA_IO_INP_MAP_CONFIG, RCA_INP_CONSTANT_CONFIG, RCA_LS_MASK_CONFIG, PR_QUEUE_PUSH, TOGGLE_PROFILER_LOCK, GET_PROFILER_DATA, ATT_CONFIGURE
    };

    assign illegal_instruction = ~(
        base_legal |
        (USE_MUL & mul_legal) |
        (USE_DIV & div_legal) |
        (USE_AMO & amo_legal) |
        (ENABLE_M_MODE & machine_legal) |
        (ENABLE_S_MODE & supervisor_legal)|
        (USE_RCA & rca_legal)
    );

endmodule
