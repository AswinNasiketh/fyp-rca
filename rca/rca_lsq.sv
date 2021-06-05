module rca_lsq
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input clk,
    input rst,
    input rca_fifo_populated,

    rca_lsq_grid_interface.lsq grid,
    rca_lsu_interface.lsq lsu
);

    //Packet ID FIFO
    logic packet_id_fifo_valid;
    id_t curr_lsq_packet_id;

    fifo_interface #(.DATA_WIDTH($bits(id_t))) packet_id_fifo_if ();

    taiga_fifo #(.DATA_WIDTH($bits(id_t)), .FIFO_DEPTH(MAX_IDS)) packet_id_fifo(
        .clk,
        .rst,
        .fifo(packet_id_fifo_if)
    );

    assign packet_id_fifo_if.pop = ~next_request_valid && packet_id_fifo_valid;
    assign packet_id_fifo_if.data_in = next_packet_id;
    assign packet_id_fifo_if.potential_push = increment_packet_id;
    assign packet_id_fifo_if.push = increment_packet_id;
    assign packet_id_fifo_valid = packet_id_fifo_if.valid;
    assign curr_lsq_packet_id = packet_id_fifo_if.data_out;
    assign grid.fifo_full = packet_id_fifo_if.full;

    //Packet ID generator
    id_t next_packet_id;
    logic increment_packet_id;

    logic [GRID_NUM_ROWS-1:0] grid_new_request; //packed version of interface signal

    initial next_packet_id = 0;
    always_ff @(posedge clk)
        if(increment_packet_id) next_packet_id <= next_packet_id + 1;

    always_comb
        for(int i = 0; i < GRID_NUM_ROWS; i++)
            grid_new_request[i] = grid.new_request[i];

    assign increment_packet_id = ~packet_id_fifo_if.full && (|grid_new_request) && rca_fifo_populated; //also serves as fifo push signal. No point in pushing into FIFO an empty packet. only allow new requests when rca fifo is populated to prevent requests from being set off by PR

    //FIFO Data

    //RCA LSQ FIFO Packet structure
    typedef struct packed{
        logic [XLEN-1:0] addr;
        logic [XLEN-1:0] data;
        logic [2:0] fn3;
        logic load;
        logic store;
        logic new_request;
    } rca_lsq_attributes_t;

    typedef rca_lsq_attributes_t rca_lsq_packet_t [GRID_NUM_ROWS];
    rca_lsq_packet_t lsq_packets [MAX_IDS];
    rca_lsq_packet_t curr_packet;

    always_ff @(posedge clk) begin
        if(increment_packet_id) begin
            for(int i = 0; i < GRID_NUM_ROWS; i++) begin
                lsq_packets[next_packet_id][i].addr <= grid.addr[i];
                lsq_packets[next_packet_id][i].data <= grid.data[i];
                lsq_packets[next_packet_id][i].fn3 <= grid.fn3[i];
                lsq_packets[next_packet_id][i].load <= grid.load[i];
                lsq_packets[next_packet_id][i].store <= grid.store[i];
                lsq_packets[next_packet_id][i].new_request <= grid.new_request[i];
            end
        end
    end

    always_comb begin
        for(int i = 0; i < GRID_NUM_ROWS; i++) begin
            curr_packet[i].addr = lsq_packets[curr_lsq_packet_id][i].addr;
            curr_packet[i].data = lsq_packets[curr_lsq_packet_id][i].data;
            curr_packet[i].fn3 = lsq_packets[curr_lsq_packet_id][i].fn3;
            curr_packet[i].load = lsq_packets[curr_lsq_packet_id][i].load;
            curr_packet[i].store = lsq_packets[curr_lsq_packet_id][i].store;
            curr_packet[i].new_request = lsq_packets[curr_lsq_packet_id][i].new_request;
        end
    end    

    //Deqeuing Mechanism (Priority Encoder)

    logic [$clog2(GRID_NUM_ROWS)-1:0] next_request;
    logic next_request_valid;
    logic [GRID_NUM_ROWS-1:0] request_completed; 

    initial request_completed = 0;
    always_ff @(posedge clk)
        if(packet_id_fifo_if.pop || rst) request_completed <= 0;
        else if(next_request_valid && lsu.lsu_ready) request_completed[next_request] <= 1;

    always_comb begin
        next_request = 0;
        while(!(curr_packet[next_request].new_request && ~request_completed[next_request]) && next_request < (GRID_NUM_ROWS-1)) begin
            next_request = next_request + 1;
        end
        next_request_valid = curr_packet[next_request].new_request && ~request_completed[next_request] && packet_id_fifo_valid;
    end

    //CPU Side LSU ID generator - not important for anything except to give CPU LSQ unique IDs to store LS attributes with
    id_t next_request_id;
    logic increment_request_id;

    initial next_request_id = 0;
    always_ff @(posedge clk)
        if(increment_request_id) next_request_id <= next_request_id + 1;

    assign increment_request_id = lsu.lsu_ready && next_request_valid; // same as new request

    // Next LSU Request Inputs
    assign lsu.rs1 = curr_packet[next_request].addr;
    assign lsu.rs2 = curr_packet[next_request].data;
    assign lsu.fn3 = curr_packet[next_request].fn3;
    assign lsu.load = curr_packet[next_request].load;
    assign lsu.store = curr_packet[next_request].store;
    assign lsu.id = next_request_id;
    assign lsu.new_request = lsu.lsu_ready && next_request_valid;

    assign lsu.rca_lsu_lock = packet_id_fifo_valid || rca_fifo_populated; //lock lsu as long as the packet id fifo contains elements or an RCA is running

    //Load tracking FIFOs - for returning load results to grid
    //Both FIFOs will be popped at the simultaneously
    logic [$clog2(GRID_NUM_ROWS)-1:0] next_load_destination;
    logic next_load_destination_valid;

    fifo_interface #(.DATA_WIDTH($clog2(GRID_NUM_ROWS))) load_tracking_fifo_if ();

    taiga_fifo #(.DATA_WIDTH($clog2(GRID_NUM_ROWS)), .FIFO_DEPTH(MAX_IDS)) load_tracking_fifo(
        .clk,
        .rst,
        .fifo(load_tracking_fifo_if)
    );

    assign load_tracking_fifo_if.push = lsu.new_request && lsu.load && ~lsu.store; //only push loads
    assign load_tracking_fifo_if.potential_push = lsu.new_request && lsu.load && ~lsu.store;   
    assign load_tracking_fifo_if.data_in = next_request;
    assign load_tracking_fifo_if.pop = next_load_data_valid && next_load_destination_valid;//always pop when both FIFOs have data valid - load OUs should be able to output a new result every cycle (due to waiting for Loads and Stores to be at least submitted to queue before committing iteration) 
    assign next_load_destination = load_tracking_fifo_if.data_out;
    assign next_load_destination_valid = load_tracking_fifo_if.valid;

    //Load result storage FIFO
    logic [XLEN-1:0] next_load_data;
    logic next_load_data_valid;

    fifo_interface #(.DATA_WIDTH(XLEN)) load_result_fifo_if ();

    taiga_fifo #(.DATA_WIDTH(XLEN), .FIFO_DEPTH(MAX_IDS)) load_result_fifo(
        .clk,
        .rst,
        .fifo(load_result_fifo_if)
    );

    assign load_result_fifo_if.push = lsu.load_complete;
    assign load_result_fifo_if.potential_push = lsu.load_complete;
    assign load_result_fifo_if.data_in = lsu.load_data;
    assign load_result_fifo_if.pop = next_load_data_valid && next_load_destination_valid;
    assign next_load_data = load_result_fifo_if.data_out;
    assign next_load_data_valid = load_result_fifo_if.valid;

    //Returning loaded data to grid
    assign grid.load_data = next_load_data;
    always_comb begin
        //Default
        for(int i = 0; i < GRID_NUM_ROWS; i++) begin
            grid.load_complete[i] = 0;
        end
        grid.load_complete[next_load_destination] = next_load_data_valid && next_load_destination_valid;
    end
endmodule