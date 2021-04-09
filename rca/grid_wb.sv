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

    logic [NUM_WRITE_PORTS-1:0] port_ready_for_commit;

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            port_ready_for_commit[i] = io_unit_sels_valid && io_unit_output_data_valid[io_unit_sels[i]];
    end

    assign wb_committing = &port_ready_for_commit;

    always_comb begin
        for (int i = 0; i < NUM_WRITE_PORTS; i++)
            output_data[i] = io_unit_output_data[io_unit_sels[i]];
    end
    
endmodule