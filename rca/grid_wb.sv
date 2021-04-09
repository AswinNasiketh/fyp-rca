module grid_wb
    import taiga_config::*;
    import riscv_types::*;
    import taiga_types::*;
    import rca_config::*;
(
    input [XLEN-1:0] io_unit_output_data [GRID_NUM_ROWS],
    input io_unit_output_data_valid [GRID_NUM_ROWS],

    input [$clog2(GRID_NUM_ROWS)-1:0] io_unit_sels [NUM_WRITE_PORTS],
    input io_unit_sels_valid,

    output [XLEN-1:0] output_data [NUM_WRITE_PORTS],
    output wb_committing
);

    logic port_ready_for_commit [NUM_WRITE_PORTS];

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            port_ready_for_commit[i] = io_unit_sels_valid && io_unit_output_data_valid[io_unit_sels[i]];
    end

    always_comb begin
        wb_committing = 1;
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            wb_committing = wb_committing && port_ready_for_commit[i]; 
    end

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            output_data[i] = io_unit_output_data[io_unit_sels[i]];
    end
    
endmodule