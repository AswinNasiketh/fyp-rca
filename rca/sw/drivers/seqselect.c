#include "seqselect.h"
#include "encodings.h"

int32_t get_branch_offset(uint32_t branch_instr){
    uint32_t imm11 = (branch_instr & (0x00000001 << 7)) << 3;
    uint32_t imm4_1 = (branch_instr & (0x0000000F << 8)) >> 8;
    uint32_t imm10_5 = (branch_instr & (0x0000003F << 25)) >> 21;
    uint32_t imm12 = (branch_instr & (0x00000001 << 31)) >> 20;

    int32_t imm = imm4_1 | imm10_5 | imm11 | imm12;
    imm = (imm << 20) >> 20; //conversion to signed if signed using arithmetic shift
    
    return imm << 1; //offset is encoded in multiple of 2 bytes
}

void print_instr_seq(instr_seq_t seq){
    printf("Sequence at %x, with %u instructions, ending at %x \n\r", seq.loop_start_addr, seq.num_instrs, seq.loop_branch_addr);
    if(seq.seq_supported){
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
    }else{
        printf("Sequence not supported\n\r");
    }
    
}

void handle_profiler_exception(){
    toggle_profiler_lock();
    profiler_entry_t profiler_entries[NUM_PROFILER_ENTRIES];

    for(int i = 0; i < NUM_PROFILER_ENTRIES; i++){
        profiler_entries[i] = get_profiler_entry(i);
    }

    instr_seq_t prof_seqs[NUM_PROFILER_ENTRIES];

    uint32_t branch_inst;
    int32_t offset;

    for(int i = 0; i < NUM_PROFILER_ENTRIES; i++){
        if(profiler_entries[i].taken_count >= SEQ_PROFILE_THRESH){

            branch_inst = *((uint32_t*) profiler_entries[i].branch_addr);
            offset = get_branch_offset(branch_inst);

            prof_seqs[i].loop_branch_addr = profiler_entries[i].branch_addr;
            prof_seqs[i].seq_supported = analyse_instr_seq(profiler_entries[i].branch_addr, offset, &prof_seqs[i]);
            print_instr_seq(prof_seqs[i]);
        }
    }



    toggle_profiler_lock();
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

