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
    logic id_pushed;
    logic id_popped;

    logic [GRID_NUM_ROWS-1:0] grid_new_request; //packed version of interface signal

    initial next_packet_id = 0;
    always_ff @(posedge clk)
        if(increment_packet_id) next_packet_id <= next_packet_id + ($bits(id_t))'d1;

    toggle_memory id_pushed_tm(
        .clk,
        .rst,
        .toggle(packet_id_fifo_if.push),
        .toggle_id(next_packet_id),
        .read_id(next_packet_id),
        .read_data(id_pushed)
    );

    toggle_memory id_popped_tm(
        .clk,
        .rst,
        .toggle(packet_id_fifo.pop),
        .toggle_id(curr_lsq_packet_id),
        .read_id(next_packet_id),
        .read_data(id_popped)
    );

    always_comb
        for(int i = 0; i < GRID_NUM_ROWS; i++)
            grid_new_request[i] = grid.new_request[i];

    assign increment_packet_id = ~(id_pushed ^ id_popped) && (|grid_new_request); //also serves as fifo push signal. No point in pushing into FIFO an empty packet

    //FIFO Data

    //RCA LSQ FIFO Packet structure
    typedef struct {
        logic [XLEN-1:0] addr [GRID_NUM_ROWS];
        logic [XLEN-1:0] data [GRID_NUM_ROWS];
        logic [2:0] fn3 [GRID_NUM_ROWS];
        logic load [GRID_NUM_ROWS];
        logic store [GRID_NUM_ROWS];
        logic new_request [GRID_NUM_ROWS];
    } rca_lsq_packet_t;

    rca_lsq_packet_t lsq_packets [MAX_IDS];
    rca_lsq_packet_t curr_packet;

    always_ff @(posedge clk) begin
        if(increment_packet_id) begin
            for(int i = 0; i < GRID_NUM_ROWS; i++) begin
                lsq_packets[next_packet_id].addr[i] <= grid.addr[i];
                lsq_packets[next_packet_id].data[i] <= grid.data[i];
                lsq_packets[next_packet_id].fn3[i] <= grid.fn3[i];
                lsq_packets[next_packet_id].load[i] <= grid.load[i];
                lsq_packets[next_packet_id].store[i] <= grid.store[i];
                lsq_packets[next_packet_id].new_request[i] <= grid.new_request[i];
            end
        end
    end

    always_comb begin
        for(int i = 0; i < GRID_NUM_ROWS; i++) begin
            curr_packet.addr[i] = lsq_packets[curr_lsq_packet_id].addr[i];
            curr_packet.data[i] = lsq_packets[curr_lsq_packet_id].data[i];
            curr_packet.fn3[i] = lsq_packets[curr_lsq_packet_id].fn3[i];
            curr_packet.load[i] = lsq_packets[curr_lsq_packet_id].load[i];
            curr_packet.store[i] = lsq_packets[curr_lsq_packet_id].store[i];
            curr_packet.new_request[i] = lsq_packets[curr_lsq_packet_id].new_request[i];
        end
    end    

    //Deqeuing Mechanism (Priority Encoder)

    logic [$clog2(GRID_NUM_ROWS-1):0] next_request;
    logic next_request_valid;
    logic [GRID_NUM_ROWS-1:0] request_completed; 

    initial request_completed = 0;
    always_ff @(posedge clk)
        if(packet_id_fifo_if.pop || rst) request_completed <= 0;
        else if(next_request_valid && lsu.lsu_ready) request_completed[next_request] <= 1;

    always_comb begin
        next_request = 0;
        while(!(curr_packet.new_request[next_request] && ~request_completed[next_request]) && next_request < (GRID_NUM_ROWS-1)) begin
            next_request = next_request + 1;
        end
        next_request_valid = curr_packet.new_request[next_request] && ~request_completed[next_request] && packet_id_fifo_valid;
    end

    // Next Request Inputs
    assign lsu.rs1 = curr_packet.addr[next_request];
    assign lsu.rs2 = curr_packet.data[next_request];
    assign lsu.fn3 = curr_packet.fn3[next_request];
    assign lsu.load = curr_packet.load[next_request];
    assign lsu.store = curr_packet.store[next_request];
    assign lsu.new_request = lsu.lsu_ready && next_request_valid;

    assign lsu.rca_lsu_lock = packet_id_fifo_valid || rca_fifo_populated; //lock lsu as long as the packet id fifo contains elements or an RCA is running

    //Load tracking FIFO
    logic [$clog2(GRID_NUM_ROWS-1):0] next_load_destination;
    logic next_load_destination_valid;

    fifo_interface #(.DATA_WIDTH($clog2(GRID_NUM_ROWS-1))) load_tracking_fifo_if ();

    taiga_fifo #(.DATA_WIDTH($clog2(GRID_NUM_ROWS-1)), .FIFO_DEPTH(MAX_IDS)) load_tracking_fifo(
        .clk,
        .rst,
        .fifo(load_tracking_fifo_if)
    );

    assign load_tracking_fifo_if.push = lsu.new_request && lsu.load && ~lsu.store;
    assign load_tracking_fifo_if.potential_push = lsu.new_request && lsu.load && ~lsu.store;
    assign load_tracking_fifo_if.pop = // grid slot ready needed here
    
    assign load_tracking_fifo_if.data_in = next_request;
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
    assign load_result_fifo_if.pop = // grid_slot_ready
    assign load_result_fifo_if.data_in = lsu.load_data;
    assign next_load_data = load_result_fifo_if.data_out;
    assign next_load_data_valid = load_result_fifo_if.valid;

endmodule