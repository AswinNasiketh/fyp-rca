#include "iid.h"

const uint32_t masks[NUM_SUPPORTED_INSTS] = {
    MASK_ADDI,
    MASK_SLTIU,
    MASK_SLTI,
    MASK_ANDI,
    MASK_ORI,
    MASK_XORI,
    MASK_SLLI,
    MASK_SRLI,
    MASK_SRAI,
    MASK_LUI,
    MASK_AUIPC,
    MASK_ADD,
    MASK_SLT,
    MASK_SLTU,
    MASK_SLL,
    MASK_SUB,
    MASK_AND,
    MASK_OR,
    MASK_XOR,
    MASK_SRL,
    MASK_SRA,

    MASK_LB,
    MASK_LH,
    MASK_LW,
    MASK_LBU,
    MASK_LHU,
    MASK_SB,
    MASK_SH,
    MASK_SW
};
const uint32_t matches[NUM_SUPPORTED_INSTS] = {
    MATCH_ADDI,
    MATCH_SLTIU,
    MATCH_SLTI,
    MATCH_ANDI,
    MATCH_ORI,
    MATCH_XORI,
    MATCH_SLLI,
    MATCH_SRLI,
    MATCH_SRAI,
    MATCH_LUI,
    MATCH_AUIPC,
    MATCH_ADD,
    MATCH_SLT,
    MATCH_SLTU,
    MATCH_SLL,
    MATCH_SUB,
    MATCH_AND,
    MATCH_OR,
    MATCH_XOR,
    MATCH_SRL,
    MATCH_SRA,

    MATCH_LB,
    MATCH_LH,
    MATCH_LW,
    MATCH_LBU,
    MATCH_LHU,
    MATCH_SB,
    MATCH_SH,
    MATCH_SW
};
const ou_t instr_ou[NUM_SUPPORTED_INSTS] = {
    ADD,
    SLTU,
    SLT,
    AND,
    OR,
    XOR,
    SLL,
    SRL,
    SRA,
    LUI,
    AUIPC,
    ADD,
    SLT,
    SLTU,
    SLL,
    SUB,
    AND,
    OR,
    XOR,
    SRL,
    SRA,

    LB,
    LH,
    LW,
    LBU,
    LHU,
    SB,
    SH,
    SW
};

void (*operand_func[NUM_SUPPORTED_INSTS]) (uint32_t raw_instr, instr_t* instr) = {
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_u,
    get_operands_u,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,
    get_operands_r,

    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_i,
    get_operands_s,
    get_operands_s,
    get_operands_s
};

void print_instr_seq(instr_seq_t seq){
    printf("Sequence at %x, with %u instructions, ending at %x \n\r", seq.loop_start_addr, seq.num_instrs, seq.loop_branch_addr);

    for(int i = 0; i < seq.num_instrs; i++){
        printf("Instr %u: OP: %u, IN1 reg?: %u, IN1 value: %d, IN2 reg?: %u, IN2 value: %d, RD: %d\n\r", 
        i, 
        seq.instr_arr[i].ou, 
        seq.instr_arr[i].input_operands[0].reg,
        seq.instr_arr[i].input_operands[0].value,
        seq.instr_arr[i].input_operands[1].reg,
        seq.instr_arr[i].input_operands[1].value,
        seq.instr_arr[i].rd_addr);
    }   
}


bool analyse_instr_seq(uint32_t branch_addr, int32_t branch_offset, instr_seq_t* instr_seq){
    uint32_t loop_start = branch_addr + branch_offset;
    
    uint32_t curr_inst;
    uint32_t num_instrs = -branch_offset/4;
    instr_t* instr_arr = malloc(num_instrs * sizeof(instr_t)); //TODO: needs freeing
    
    bool instr_supported;
    for(uint32_t i = loop_start, j = 0; i < branch_addr, j < num_instrs; i+= 4, j++){
        curr_inst = *((uint32_t*) i);
        instr_supported = analyse_instr(curr_inst, &instr_arr[j]);
        if(!instr_supported) return false;
    }

    instr_seq->instr_arr = instr_arr;
    instr_seq->num_instrs = num_instrs;
    instr_seq->loop_start_addr = loop_start;
    return true;
}

bool analyse_instr(uint32_t raw_instr, instr_t* instr){
    int32_t matched_index = -1; 
    for(int i = 0; i < NUM_SUPPORTED_INSTS; i++){
        if((raw_instr & masks[i]) == matches[i]){
            matched_index = i; 
            break;
        }
    }

    if(matched_index >= 0){
        instr->ou = instr_ou[matched_index];
        (*operand_func[matched_index]) (raw_instr, instr);
        return true;
    }

    return false;
}

//I-type Integer instructions - signed immediate
void get_operands_i(uint32_t raw_instr, instr_t* instr){
    uint32_t imm11_0 = (raw_instr & (0x00000FFF << 20));
    int32_t imm_sgd = imm11_0;
    imm_sgd = imm_sgd >> 20; //do right shift here for sign extension

    uint32_t rs1 = (raw_instr & (0x0000001F << 15)) >> 15;
    uint32_t rd = (raw_instr & (0x0000001F << 7)) >> 7;

    instr->rd_addr = rd;

    instr->input_operands[0].reg = true;
    instr->input_operands[0].value = rs1;

    instr->input_operands[1].reg = false;
    instr->input_operands[1].value = imm_sgd;
}

//U-type Integer instructions
void get_operands_u(uint32_t raw_instr, instr_t* instr){
    uint32_t imm31_12 = (raw_instr & (0x000FFFFF << 12)) >> 12;
    
    uint32_t rd = (raw_instr & (0x0000001F << 7)) >> 7;

    instr->rd_addr = rd;

    instr->input_operands[0].reg = false;
    instr->input_operands[0].value = imm31_12;
    
    instr->input_operands[1].reg = true;
    instr->input_operands[1].value = -1;
}

//R-type Integer instructions
void get_operands_r(uint32_t raw_instr, instr_t* instr){
    uint32_t rd = (raw_instr & (0x0000001F << 7)) >> 7;

    uint32_t rs1 = (raw_instr & (0x0000001F << 15)) >> 15;
    uint32_t rs2 = (raw_instr & (0x0000001F << 20)) >> 20;

    instr->rd_addr = rd;
    
    instr->input_operands[0].reg = true;
    instr->input_operands[0].value = rs1;

    instr->input_operands[1].reg = true;
    instr->input_operands[1].value = rs2;
}

//S type Integer instructions
void get_operands_s(uint32_t raw_instr, instr_t* instr){
    uint32_t imm4_0 = (raw_instr & (0x0000001F << 7)) >> 7;
    uint32_t imm11_5 = (raw_instr & (0x0000007F << 25)) >> 20;

    int32_t imm = imm4_0 | imm11_5;
    imm = (imm << 20) >> 20; //sign extension

    uint32_t rs1 = (raw_instr & (0x0000001F << 15)) >> 15;
    uint32_t rs2 = (raw_instr & (0x0000001F << 20)) >> 20;

    instr->rd_addr = rs1; //s-type instr so rd will be treated as input

    instr->input_operands[0].reg = false;
    instr->input_operands[0].value = imm;

    instr->input_operands[1].reg = true;
    instr->input_operands[1].value = rs2;
}

bool is_ls_op(ou_t op){
    const ou_t ls_ous[] = {
        LB,
        LBU,
        LH,
        LHU,
        LW,
        SB,
        SH,
        SW
    };

    for(int i = 0; i < 8; i++){
        if(op == ls_ous[i]){
            return true;
        }
    }

    return false;
}

int32_t get_branch_offset(uint32_t branch_instr){
    uint32_t imm11 = (branch_instr & (0x00000001 << 7)) << 3;
    uint32_t imm4_1 = (branch_instr & (0x0000000F << 8)) >> 8;
    uint32_t imm10_5 = (branch_instr & (0x0000003F << 25)) >> 21;
    uint32_t imm12 = (branch_instr & (0x00000001 << 31)) >> 20;

    int32_t imm = imm4_1 | imm10_5 | imm11 | imm12;
    imm = (imm << 20) >> 20; //conversion to signed if signed using arithmetic shift
    
    return imm << 1; //offset is encoded in multiple of 2 bytes
}