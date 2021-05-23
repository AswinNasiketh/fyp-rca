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

void print_dfg(dfg_t dfg){
    printf("Printing DFG \n\r");
    printf("Nodes: \n\r");

    dfg_node_t* curr_node = dfg.first_node;
    while(curr_node != NULL){
        printf("Node ID: %u, is_input: %u, is_output: %u, is_reg: %u, value: %u, op: %u\n\r",
        curr_node->node_id,
        curr_node->is_input,
        curr_node->is_output,
        curr_node->reg,
        curr_node->inp_value,
        curr_node->op
        );
        curr_node = curr_node->next_node;
    }

    printf("Edges: \n\r");
    dfg_edge_t* curr_edge = dfg.first_edge;
    while(curr_edge != NULL){
        printf("Edge from: %u, to: %u, slot input: %u\n\r",
        curr_edge->fromNode,
        curr_edge->toNode,
        curr_edge->slot_inp
        );
        curr_edge = curr_edge->next_edge;
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

            dfg_t dfg = create_dfg(&prof_seqs[i]);
            print_dfg(dfg);
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

void add_node(dfg_node_t* node_lst, dfg_node_t* node){
    node_lst->next_node = node;
}

void init_node(dfg_node_t* node){
    node->next_node = NULL;
}

void init_edge(dfg_edge_t* edge){
    edge->next_edge = NULL;
}

void add_edge(dfg_edge_t* last_edge, dfg_edge_t* new_edge){
    last_edge->next_edge = new_edge;
}

void init_new_node_reg(dfg_node_t* new_node, uint32_t reg_addr, uint32_t node_id){
    init_node(new_node);
    new_node->is_input = true;
    new_node->reg = true;
    new_node->inp_value = reg_addr;
    new_node->node_id = node_id;
    new_node->is_output = false;
}

void init_new_node_lit(dfg_node_t* new_node, uint32_t lit_val, uint32_t node_id){
    init_node(new_node);
    new_node->is_input = true;
    new_node->reg = false;
    new_node->inp_value = lit_val;
    new_node->node_id = node_id;
    new_node->is_output = false;
}

void init_new_node_op(dfg_node_t* new_node, ou_t op, uint32_t node_id){
    init_node(new_node);
    new_node->is_input = false;
    new_node->op = op;
    new_node->node_id = node_id;
    new_node->is_output = false;
}

void init_new_edge(dfg_edge_t* new_edge, uint32_t fromNode, uint32_t toNode, grid_slot_inp_t slot_inp){
    init_edge(new_edge);
    new_edge->fromNode = fromNode;
    new_edge->toNode = toNode;
    new_edge->slot_inp = slot_inp;
    // printf("New Edge - from %u, to %u\n\r", fromNode, toNode);
}

void init_new_node_output(dfg_node_t* new_node, uint32_t reg_addr, uint32_t node_id){
    init_node(new_node);
    new_node->is_input = false;
    new_node->is_output = true;
    new_node->reg = true; //always true for outputs
    new_node->inp_value = reg_addr;
    new_node->node_id = node_id;
}

void process_instr_inputs_normal(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge){
    dfg_node_t* new_node;
    dfg_edge_t* new_edge;
    dfg_node_t* curr_op_node = *last_node;
    // printf("Normal process instr inputs\n\r");
    for(int i = 0; i < 2; i++){
        if(curr_instr.input_operands[i].reg){
            if(curr_instr.input_operands[i].value >= 0){
                // printf("Reg Addr: %u\n\r", curr_instr.input_operands[i].value);
                // printf("Reg Last Write: %d\n\r", reg_last_write[curr_instr.input_operands[i].value]);
                if(reg_last_write[curr_instr.input_operands[i].value] == -1){                    
                    new_node = malloc(sizeof(dfg_node_t));
                    init_new_node_reg(new_node, curr_instr.input_operands[i].value, (*last_node)->node_id + 1);
                    add_node(*last_node, new_node);
                    *last_node = new_node;

                    new_edge = malloc(sizeof(dfg_edge_t));
                    init_new_edge(new_edge, new_node->node_id, curr_op_node->node_id, i);
                    add_edge(*last_edge, new_edge);
                    *last_edge = new_edge;
                }else{
                    uint32_t inp_node_id = reg_last_write[curr_instr.input_operands[i].value];

                    new_edge = malloc(sizeof(dfg_edge_t));
                    init_new_edge(new_edge, inp_node_id, curr_op_node->node_id, i);
                    
                    add_edge(*last_edge, new_edge);
                    *last_edge = new_edge;
                }
            }
        }else{
            new_node = malloc(sizeof(dfg_node_t));
            init_new_node_lit(new_node, curr_instr.input_operands[i].value, (*last_node)->node_id + 1);
            add_node(*last_node, new_node);
            *last_node = new_node;

            new_edge = malloc(sizeof(dfg_edge_t));
            init_new_edge(new_edge, new_node->node_id, curr_op_node->node_id, i);
            add_edge(*last_edge, new_edge);
            *last_edge = new_edge;
        }
    }
}

//NOTE: Whole AUIPC instruction could be replaced with a constant input
void process_instr_inputs_auipc(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge, uint32_t instr_addr){
    dfg_node_t* curr_op_node = *last_node;

    //create literal node for current PC
    dfg_node_t* new_node = malloc(sizeof(dfg_node_t));
    init_new_node_lit(new_node, instr_addr, (*last_node)->node_id+1);
    add_node(*last_node, new_node);
    *last_node = new_node;

    //add PC literal node as input 2 to current instr
    dfg_edge_t* new_edge = malloc(sizeof(dfg_edge_t));
    init_new_edge(new_edge, new_node->node_id, curr_op_node->node_id, INP2);
    add_edge(*last_edge, new_edge);
    *last_edge = new_edge;

    //create literal node for upper immediate
    new_node = malloc(sizeof(dfg_node_t));
    init_new_node_lit(new_node, curr_instr.input_operands[0].value, (*last_node)->node_id+1);
    add_node(*last_node, new_node);
    *last_node = new_node;
    
    //add upper immediate as input 1 to current instr
    new_edge = malloc(sizeof(dfg_edge_t));
    init_new_edge(new_edge, new_node->node_id, curr_op_node->node_id, INP1);
    add_edge(*last_edge, new_edge);
    *last_edge = new_edge;
}

void process_instr_inputs_load(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge){
    dfg_node_t* curr_op_node = *last_node;

    //create addition structure to perform offset calculation to address
    dfg_node_t* adder_node = malloc(sizeof(dfg_node_t));
    init_new_node_op(adder_node, ADD, (*last_node)->node_id+1);
    add_node(*last_node, adder_node);
    *last_node = adder_node;


    dfg_node_t* new_node;
    dfg_edge_t* new_edge;

    if(reg_last_write[curr_instr.input_operands[0].value] == -1){
        new_node = malloc(sizeof(dfg_node_t));
        init_new_node_reg(new_node, curr_instr.input_operands[0].value, (*last_node)->node_id+1);
        add_node(*last_node, new_node);
        *last_node = new_node;

        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, new_node->node_id, adder_node->node_id, INP1);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }else{
        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, reg_last_write[curr_instr.input_operands[0].value], adder_node->node_id, INP1);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }

    new_node = malloc(sizeof(dfg_node_t));
    init_new_node_lit(new_node, curr_instr.input_operands[1].value, (*last_node)->node_id+1);
    add_node(*last_node, new_node);
    *last_node = new_node;

    new_edge = malloc(sizeof(dfg_edge_t));
    init_new_edge(new_edge, new_node->node_id, adder_node->node_id, INP2);
    add_edge(*last_edge, new_edge);
    *last_edge = new_edge; 

    //route adder output to load store unit

    new_edge = malloc(sizeof(dfg_edge_t));
    init_new_edge(new_edge, adder_node->node_id, curr_op_node->node_id, INP1);
    add_edge(*last_edge, new_edge);
    *last_edge = new_edge; 
}

void process_instr_inputs_store(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge){
    dfg_node_t* curr_op_node = *last_node;

    //create addition structure to perform offset calculation to address
    dfg_node_t* adder_node = malloc(sizeof(dfg_node_t));
    init_new_node_op(adder_node, ADD, (*last_node)->node_id+1);
    add_node(*last_node, adder_node);
    *last_node = adder_node;


    dfg_node_t* new_node;
    dfg_edge_t* new_edge;

    if(reg_last_write[curr_instr.rd_addr] == -1){
        new_node = malloc(sizeof(dfg_node_t));
        init_new_node_reg(new_node, curr_instr.rd_addr, (*last_node)->node_id+1);
        add_node(*last_node, new_node);
        *last_node = new_node;

        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, new_node->node_id, adder_node->node_id, INP1);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }else{
        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, reg_last_write[curr_instr.rd_addr] , adder_node->node_id, INP1);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }

    new_node = malloc(sizeof(dfg_node_t));
    init_new_node_lit(new_node, curr_instr.input_operands[0].value, (*last_node)->node_id+1);
    add_node(*last_node, new_node);
    *last_node = new_node;

    new_edge = malloc(sizeof(dfg_edge_t));
    init_new_edge(new_edge, new_node->node_id, adder_node->node_id, INP2);
    add_edge(*last_edge, new_edge);
    *last_edge = new_edge;

     //route adder output to load store unit

    new_edge = malloc(sizeof(dfg_edge_t));
    init_new_edge(new_edge, adder_node->node_id, curr_op_node->node_id, INP1);
    add_edge(*last_edge, new_edge);
    *last_edge = new_edge; 


    //route data to store unit
    if(reg_last_write[curr_instr.input_operands[1].value] == -1){
        new_node = malloc(sizeof(dfg_node_t));
        init_new_node_reg(new_node,curr_instr.input_operands[1].value, (*last_node)->node_id+1);
        add_node(*last_node, new_node);
        *last_node = new_node;

        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, new_node->node_id, curr_op_node->node_id, INP2);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }else{
        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, reg_last_write[curr_instr.input_operands[1].value] , adder_node->node_id, INP1);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }
}


void process_instr_inputs(dfg_node_t** last_node, instr_t curr_instr, int32_t reg_last_write[NUM_CPU_REGS], dfg_edge_t** last_edge, uint32_t curr_instr_addr){
    bool is_load = false;
    bool is_store = false;

    const ou_t load_ous[] = {
        LB,
        LBU,
        LH,
        LHU,
        LW,
    };

    const ou_t store_ous[] = {
        SB,
        SH,
        SW
    };

    for(int i = 0; i < 5; i++){
        if(curr_instr.ou == load_ous[i]){
            is_load = true;
            break;
        }
    }

    for(int i = 0; i < 3; i++){
        if(curr_instr.ou == store_ous[i]){
            is_store = true;
            break;
        }
    }

    if(is_load){
        process_instr_inputs_load(last_node, curr_instr, reg_last_write, last_edge);
    }else if(is_store){
        process_instr_inputs_store(last_node, curr_instr, reg_last_write, last_edge);
    }else if(curr_instr.ou == AUIPC){
        process_instr_inputs_auipc(last_node, curr_instr, reg_last_write, last_edge, curr_instr_addr);
    }else{
        process_instr_inputs_normal(last_node, curr_instr, reg_last_write, last_edge);
    }    
}

dfg_t create_dfg(instr_seq_t* instr_seq){
    dfg_t dfg;
    int32_t reg_last_write[NUM_CPU_REGS];
    for(int i = 0; i < NUM_CPU_REGS; i++){
        reg_last_write[i] = -1;
    }

    instr_t curr_instr;

    dfg_node_t firstNode;
    dfg_edge_t firstEdge;
    firstNode.node_id = 0;

    dfg_node_t* lastNode = &firstNode;
    dfg_edge_t* lastEdge = &firstEdge;
    dfg_node_t* newNode;
    dfg_edge_t* newEdge;

    for(int i = 0, j = instr_seq->loop_start_addr; i < instr_seq->num_instrs, j < instr_seq->loop_branch_addr; i++, j+=4){
        newNode = malloc(sizeof(dfg_node_t));
        init_new_node_op(newNode, instr_seq->instr_arr[i].ou, lastNode->node_id+1);
        add_node(lastNode, newNode);
        lastNode = newNode;
        
        process_instr_inputs(&lastNode, instr_seq->instr_arr[i], reg_last_write, &lastEdge, j);
        if(instr_seq->instr_arr[i].ou != SW && instr_seq->instr_arr[i].ou != SH && instr_seq->instr_arr[i].ou != SB){
            reg_last_write[instr_seq->instr_arr[i].rd_addr] = newNode->node_id;
        }
    }


    uint32_t finalNodeID = lastNode->node_id;
    bool* hasOutputEdge = malloc(finalNodeID*sizeof(bool));

    //if there is no output edge for the node, and it is an op, and it has a corresponding entry in the reg_last_write table


    dfg_edge_t* curr_edge_ptr = firstEdge.next_edge;
    while(curr_edge_ptr != NULL){
        hasOutputEdge[curr_edge_ptr->fromNode] = true;
        curr_edge_ptr = curr_edge_ptr->next_edge;
    }
    for(int i = 0; i < finalNodeID; i++){
        if(hasOutputEdge[i]){
            continue;
        }else{
            int32_t unwrittenReg = -1;
            for(int j = 0; j < NUM_CPU_REGS; j++){
                if(reg_last_write[j] == i){
                    unwrittenReg = j;
                    break;
                }
            }

            if(unwrittenReg == -1){
                continue; //can assume this is a store instruction
            }else{

                int32_t reg_id = -1;
                //find node id of reg
            
                dfg_node_t* curr_node_ptr = &firstNode;

                while(reg_id == -1 && curr_node_ptr != NULL){
                    if(curr_node_ptr->is_input && curr_node_ptr->reg && curr_node_ptr->inp_value == unwrittenReg){
                        reg_id = curr_node_ptr->node_id;
                        curr_node_ptr->is_output = true;
                    }else{
                        curr_node_ptr = curr_node_ptr->next_node;
                    }
                }

                //if register doesn't already have a node, create one
                if(reg_id == -1){
                    newNode = malloc(sizeof(dfg_node_t));
                    init_new_node_output(newNode, i, lastNode->node_id+1);
                    add_node(lastNode, newNode);
                    lastNode = newNode;

                    reg_id = newNode->node_id;
                }

                //create edge from output to register
                newEdge = malloc(sizeof(dfg_node_t));
                init_new_edge(newEdge, i, reg_id, INP1); //INP doesn't matter for output nodes
                add_edge(lastEdge, newEdge);
                lastEdge = newEdge;
            }
        }
    }

    dfg.first_edge = firstEdge.next_edge;
    dfg.first_node = firstNode.next_node;

    free(hasOutputEdge);

    return dfg;
}
