#include "dfg.h"


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
    new_node->is_literal_input_used = true;
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
        // printf("Malloc Address: %x\n\r", newNode);
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

    squash_literal_inputs(&dfg);

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
        return largest_inp_depth + ((num_ls_ops + 1) * RCA_PACKET_LSQ_DEPTH) + CPU_LSQ_DEPTH + CPU_MEMORY_LATENCY + LOAD_FORWARD_LATENCY;
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


void remap_edges(dfg_t* dfg, uint32_t node_id_to_replace, uint32_t new_node_id){
    dfg->nodes[node_id_to_replace-1].is_literal_input_used = false;

    for(int i = 0; i < dfg->num_edges; i++){
        if(dfg->edges[i].fromNode == node_id_to_replace){
            dfg->edges[i].fromNode = new_node_id;
        }

        if(dfg->edges[i].toNode == node_id_to_replace){
            dfg->edges[i].toNode = new_node_id;
        }
    }
}

void squash_literal_inputs(dfg_t* dfg){
    uint32_t* literal_inp_node_ids = malloc(dfg->num_nodes*sizeof(uint32_t));
    uint32_t next_literal_inp_node_id = 0;

    for(int i = 0; i < dfg->num_nodes; i++){
        if(dfg->nodes[i].is_input && !dfg->nodes->reg){
            literal_inp_node_ids[next_literal_inp_node_id] = dfg->nodes[i].node_id;
            next_literal_inp_node_id++;
        }
    }


    for(int i = 0; i < next_literal_inp_node_id; i++){
        uint32_t literal_node_id_i = literal_inp_node_ids[i];

        for(int j = i+1; j < next_literal_inp_node_id; j++){
            uint32_t literal_node_id_j = literal_inp_node_ids[j];

            if(dfg->nodes[literal_node_id_j-1].is_literal_input_used){
                if(dfg->nodes[literal_node_id_i-1].inp_value == dfg->nodes[literal_node_id_j-1].inp_value){
                    remap_edges(dfg, literal_node_id_j, literal_node_id_i);
                }
            }

        }
    }

    
    free(literal_inp_node_ids);
}

uint32_t get_num_op_nodes(dfg_t dfg){
    uint32_t num_op_nodes = 0;
    for(int i = 0; i < dfg.num_nodes; i++){
        if(!dfg.nodes[i].is_input && !dfg.nodes[i].is_output){
            num_op_nodes++;
        }
    }

    return num_op_nodes;
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