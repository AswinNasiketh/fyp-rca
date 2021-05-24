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

    for(int i = 0; i < dfg.num_nodes; i++){
        printf("Node ID: %u, is_input: %u, is_output: %u, is_reg: %u, value: %u, op: %u\n\r",
        dfg.nodes[i].node_id,
        dfg.nodes[i].is_input,
        dfg.nodes[i].is_output,
        dfg.nodes[i].reg,
        dfg.nodes[i].inp_value,
        dfg.nodes[i].op
        );

    }

    printf("Edges: \n\r");

    for(int i = 0; i < dfg.num_edges; i++){
        printf("Edge from: %u, to: %u, slot input: %u\n\r",
        dfg.edges[i].fromNode,
        dfg.edges[i].toNode,
        dfg.edges[i].slot_inp
        );
    }
}

void print_seq_profile(seq_profile_t seq_prof){
    printf("Printing Sequence Profile\n\r");
    printf("Loop start address: %x\n\r", seq_prof.loop_start_addr);
    printf("Num Instrs: %u\n\r", seq_prof.num_instrs);
    
    print_dfg(*(seq_prof.dfg));
    printf("CPU time per iteration: %u\n\r", seq_prof.cpu_time_per_iter);
    printf("Acc time per iteration: %u\n\r", seq_prof.acc_time_per_iter);

    printf("PR time: %u\n\r", seq_prof.pr_time);
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

            seq_profile_t seq_prof = profile_seq(&prof_seqs[i]);
            print_seq_profile(seq_prof);
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

                    reg_last_write[new_node->inp_value] = new_node->node_id;

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

        reg_last_write[new_node->inp_value] = new_node->node_id;

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

        reg_last_write[new_node->inp_value] = new_node->node_id;

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

        reg_last_write[new_node->inp_value] = new_node->node_id;

        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, new_node->node_id, curr_op_node->node_id, INP2);
        add_edge(*last_edge, new_edge);
        *last_edge = new_edge;
    }else{
        new_edge = malloc(sizeof(dfg_edge_t));
        init_new_edge(new_edge, reg_last_write[curr_instr.input_operands[1].value] , curr_op_node->node_id, INP1);
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

void lls_to_arrays(dfg_node_t* first_node, dfg_edge_t* first_edge, uint32_t* num_nodes, uint32_t* num_edges, dfg_node_t** node_arr_ptr, dfg_edge_t** edge_arr_ptr){
    *num_nodes = 0;
    *num_edges = 0;

    dfg_node_t* curr_node = first_node;
    while (curr_node != NULL)
    {
        (*num_nodes)++;
        curr_node = curr_node->next_node;
    }

    dfg_edge_t* curr_edge = first_edge;
    while (curr_edge != NULL)
    {
        (*num_edges)++;
        curr_edge = curr_edge->next_edge;
    }

    *node_arr_ptr = malloc((*num_nodes)*sizeof(dfg_node_t));
    *edge_arr_ptr = malloc((*num_edges)*sizeof(dfg_edge_t));

    // printf("Num Nodes: %u. Num Edges %u\n\r", *num_nodes, *num_edges);

    curr_node = first_node;
    for(int i = 0; i < *num_nodes; i++){
        (*node_arr_ptr)[i] = *curr_node;
        free(curr_node);
        curr_node = (*node_arr_ptr)[i].next_node;
    }

    curr_edge = first_edge;
    for(int i = 0; i < *num_edges; i++){
        (*edge_arr_ptr)[i] = *curr_edge;
        free(curr_edge);
        curr_edge = (*edge_arr_ptr)[i].next_edge;
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

    for(int j = 0; j < NUM_CPU_REGS; j++){
        if(reg_last_write[j] != -1){
            // printf("Reg last write %u\n\r", reg_last_write[j]);

            dfg_node_t* curr_node_ptr = &firstNode;
            dfg_node_t* output_node_ptr = NULL;
            while(output_node_ptr == NULL && curr_node_ptr != NULL){
                if(curr_node_ptr->node_id == reg_last_write[j]){
                    output_node_ptr = curr_node_ptr;
                }
                curr_node_ptr = curr_node_ptr->next_node;
            }


            if(!(output_node_ptr->is_input)){            
                int32_t reg_id = -1;
                //find node id of reg
        
                curr_node_ptr = &firstNode;

                while(reg_id == -1 && curr_node_ptr != NULL){
                    if(curr_node_ptr->is_input && curr_node_ptr->reg && curr_node_ptr->inp_value == j){
                        reg_id = curr_node_ptr->node_id;
                        curr_node_ptr->is_output = true;
                    }else{
                        curr_node_ptr = curr_node_ptr->next_node;
                    }
                }

                //if register doesn't already have a node, create one
                if(reg_id == -1){
                    newNode = malloc(sizeof(dfg_node_t));
                    // printf("Last node ID %u\n\r", reg_last_write[j]);
                    init_new_node_output(newNode, j, lastNode->node_id+1);
                    add_node(lastNode, newNode);
                    lastNode = newNode;

                    reg_id = newNode->node_id;
                }

                //create edge from output to register
                newEdge = malloc(sizeof(dfg_node_t));
                init_new_edge(newEdge, reg_last_write[j], reg_id, INP1); //INP doesn't matter for output nodes
                add_edge(lastEdge, newEdge);
                lastEdge = newEdge;
            }
        }
    }


    dfg_edge_t* edge_arr_ptr;
    dfg_node_t* node_arr_ptr;
    uint32_t num_nodes;
    uint32_t num_edges;

    lls_to_arrays(firstNode.next_node, firstEdge.next_edge, &num_nodes, &num_edges, &node_arr_ptr, &edge_arr_ptr);
    
    dfg.nodes = node_arr_ptr;
    dfg.edges = edge_arr_ptr;
    dfg.num_nodes = num_nodes;
    dfg.num_edges = num_edges;

    return dfg;
}

uint32_t find_node_depth_cpu(dfg_t dfg, uint32_t nodeID, bool recursive_call){
    if(dfg.nodes[nodeID-1].is_input && recursive_call){
        return 0;
    }

    int32_t node_inps[2] = {-1, -1}; //each node can have a maximum of 2 inputs
    uint32_t j = 0;
    for(int i = 0; i < dfg.num_edges; i++){
        if(dfg.edges[i].toNode == nodeID){
            node_inps[j] = dfg.edges[i].fromNode;
            j++;
            if(j == 2){
                break;
            }
        }
    }

    uint32_t inp_node_depths[2] = {0,0};
    for(int i = 0; i < 2; i++){
        if(node_inps[i] >=  0){
            inp_node_depths[i] = find_node_depth_cpu(dfg, node_inps[i], true);
        }
    }

    uint32_t largest_inp_depth = (inp_node_depths[1] > inp_node_depths[0]) ? inp_node_depths[1] : inp_node_depths[0];

    if(dfg.nodes[nodeID-1].op == LB || dfg.nodes[nodeID-1].op == LBU || dfg.nodes[nodeID-1].op == LH || dfg.nodes[nodeID-1].op == LHU || dfg.nodes[nodeID-1].op == LW){
        return largest_inp_depth + CPU_LOAD_LATENCY - 1; //-1 since an extra adder is added in DFG for calculating address with offset in accelerator
    }else if(dfg.nodes[nodeID-1].op == SB || dfg.nodes[nodeID-1].op == SH || dfg.nodes[nodeID-1].op == SW){
        return largest_inp_depth; //don't add anything for stores adder is added in DFG for calculating address with offset in accelerator
    }else{
        return largest_inp_depth + 1;
    }
}

uint32_t find_node_depth_acc(dfg_t dfg, uint32_t nodeID, uint32_t num_ls_ops, bool recursive_call){
    if(dfg.nodes[nodeID-1].is_input && recursive_call){
        return 0;
    }

    //all integer comp latencies are 1 for both cpu and acc
    //load latency accelerator is (number of L/S ops + 1) * LSQ  depth
    //this load latency is only to be added once - it is not PER LOAD
    //load latency cpu is 3 cycles (add 2 because of extra add node added for accelerator)
    //store latency accelerator is (number of L/S ops + 1) (max time taken for an RCA LSQ packet to become free) - only considering store submit
    //only one store needs to be considered

    //store latency cpu is 1 cycle - only considering store submit

    int32_t node_inps[2] = {-1, -1}; //each node can have a maximum of 2 inputs
    uint32_t j = 0;
    for(int i = 0; i < dfg.num_edges; i++){
        if(dfg.edges[i].toNode == nodeID){
            node_inps[j] = dfg.edges[i].fromNode;
            j++;
            if(j == 2){
                break;
            }
        }
    }

    uint32_t inp_node_depths[2] = {0,0};
    for(int i = 0; i < 2; i++){
        if(node_inps[i] >=  0){
            inp_node_depths[i] = find_node_depth_acc(dfg, node_inps[i], num_ls_ops, true);
        }
    }

    uint32_t largest_inp_depth = (inp_node_depths[1] > inp_node_depths[0]) ? inp_node_depths[1] : inp_node_depths[0];

    if(dfg.nodes[nodeID-1].op == LB || dfg.nodes[nodeID-1].op == LBU || dfg.nodes[nodeID-1].op == LH || dfg.nodes[nodeID-1].op == LHU || dfg.nodes[nodeID-1].op == LW){
        return largest_inp_depth + ((num_ls_ops + 1) * RCA_PACKET_LSQ_DEPTH);
    }else if(dfg.nodes[nodeID-1].op == SB || dfg.nodes[nodeID-1].op == SH || dfg.nodes[nodeID-1].op == SW){
        return largest_inp_depth; //don't add anything for stores - constant will be added on
    }else{
        return largest_inp_depth + 1;
    }
}

uint32_t calc_pr_time(dfg_t dfg){
    uint32_t num_op_nodes = 0;
    for(int i = 0; i < dfg.num_nodes; i++){
        if(!dfg.nodes[i].is_input && !dfg.nodes[i].is_output){
            num_op_nodes++;
        }
    }
    return (num_op_nodes * (FRAME_RECONF_TIME_MS * CPU_CLK_FREQ/1000));
}


seq_profile_t profile_seq(instr_seq_t* seq){
    dfg_t* dfg = malloc(sizeof(dfg_t));

    *dfg = create_dfg(seq);

    uint32_t cpu_loop_iteration_latency = 0;
    uint32_t output_node_latency;
    for(int i = 0; i < (*dfg).num_nodes; i++){
        if((*dfg).nodes[i].is_output || ((!(*dfg).nodes[i].is_input && !(*dfg).nodes[i].is_output) && ((*dfg).nodes[i].op == SW) || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SB)){
            // printf("Finding CPU Depth of Node %u\n\r", i);
            output_node_latency = find_node_depth_cpu((*dfg), (*dfg).nodes[i].node_id, false);
            if(output_node_latency > cpu_loop_iteration_latency){
                cpu_loop_iteration_latency = output_node_latency;
            }
        }
    }

    uint32_t acc_loop_iteration_latency;
    uint32_t num_ls_ops = 0;
    bool store_ops = false;

    for(int i = 0; i < (*dfg).num_nodes; i++){
        if((*dfg).nodes[i].op == LB || (*dfg).nodes[i].op == LBU|| (*dfg).nodes[i].op == LH || (*dfg).nodes[i].op == LHU || (*dfg).nodes[i].op == LW || (*dfg).nodes[i].op == SB || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SW){
            num_ls_ops++;
            if((*dfg).nodes[i].op == SB || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SW){
                store_ops = true;
            }
        }
    }

    for(int i = 0; i < (*dfg).num_nodes; i++){
        if(((*dfg).nodes[i].is_output && (*dfg).nodes[i].is_input) || ((!(*dfg).nodes[i].is_input && !(*dfg).nodes[i].is_output) && ((*dfg).nodes[i].op == SW) || (*dfg).nodes[i].op == SH || (*dfg).nodes[i].op == SB)){
            output_node_latency = find_node_depth_acc((*dfg), (*dfg).nodes[i].node_id, num_ls_ops, false);
            if(output_node_latency > acc_loop_iteration_latency){
                acc_loop_iteration_latency = output_node_latency;
            }
        }
    }

    if(store_ops){
        acc_loop_iteration_latency += num_ls_ops+1;
    }

    uint32_t pr_time = calc_pr_time((*dfg));

    seq_profile_t seq_prof;

    seq_prof.loop_start_addr = seq->loop_start_addr;
    seq_prof.num_instrs = seq->num_instrs;
    seq_prof.cpu_time_per_iter = cpu_loop_iteration_latency;
    seq_prof.acc_time_per_iter = acc_loop_iteration_latency;
    seq_prof.pr_time = pr_time;    
    seq_prof.dfg = dfg;  

    return seq_prof;
}
